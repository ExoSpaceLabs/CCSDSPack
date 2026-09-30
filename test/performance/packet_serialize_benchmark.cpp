// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSPacket.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::atomic<std::size_t> g_allocations{0U};
std::atomic<std::size_t> g_allocated_bytes{0U};
thread_local bool g_track_allocations = false;
volatile std::size_t g_sink = 0U;

struct PerfSample {
  double ns_per_op{};
  double allocations_per_op{};
  double allocated_bytes_per_op{};
};

void recordAllocation(const std::size_t size) noexcept {
  if (!g_track_allocations) return;
  g_allocations.fetch_add(1U, std::memory_order_relaxed);
  g_allocated_bytes.fetch_add(size, std::memory_order_relaxed);
}

template <typename F>
PerfSample measure(const std::size_t iterations, F &&operation) {
  for (std::size_t i = 0U; i < 16U; ++i) operation();

  g_allocations.store(0U, std::memory_order_relaxed);
  g_allocated_bytes.store(0U, std::memory_order_relaxed);
  g_track_allocations = true;
  const auto start = std::chrono::steady_clock::now();
  for (std::size_t i = 0U; i < iterations; ++i) operation();
  const auto end = std::chrono::steady_clock::now();
  g_track_allocations = false;

  const auto elapsed =
    std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  return {
    static_cast<double>(elapsed) / static_cast<double>(iterations),
    static_cast<double>(g_allocations.load(std::memory_order_relaxed))
      / static_cast<double>(iterations),
    static_cast<double>(g_allocated_bytes.load(std::memory_order_relaxed))
      / static_cast<double>(iterations)
  };
}

std::size_t iterationsFor(const std::size_t packetSize) {
  if (packetSize <= 64U) return 15000U;
  if (packetSize <= 256U) return 8000U;
  if (packetSize <= 1024U) return 3000U;
  if (packetSize <= 8192U) return 750U;
  return 200U;
}

ccsds::Packet makePacket(const std::size_t packetSize, const bool crcEnabled) {
  const std::size_t pecSize = crcEnabled ? 2U : 0U;
  if (packetSize <= 6U + pecSize) throw std::runtime_error("packet too small");

  ccsds::Packet packet;
  packet.setDataFieldSize(65535U);
  packet.setPacketErrorControlMode(
    crcEnabled ? ccsds::PacketErrorControlMode::CRC16
               : ccsds::PacketErrorControlMode::None);

  std::vector<std::uint8_t> payload(packetSize - 6U - pecSize, 0U);
  for (std::size_t i = 0U; i < payload.size(); ++i) {
    payload[i] = static_cast<std::uint8_t>((i * 29U + packetSize) & 0xFFU);
  }
  const auto set = packet.setApplicationData(payload);
  if (!set) throw std::runtime_error("setApplicationData failed");
  const auto update = packet.update();
  if (!update) throw std::runtime_error("initial update failed");
  return packet;
}

void runCase(const std::string &label,
             const std::string &mode,
             const std::size_t packetSize,
             const bool crcEnabled) {
  auto packet = makePacket(packetSize, crcEnabled);
  const auto iterations = iterationsFor(packetSize);
  std::uint16_t sequence = 0U;

  const auto sample = measure(iterations, [&] {
    if (mode == "refinalize") {
      sequence = static_cast<std::uint16_t>((sequence + 1U) & 0x3FFFU);
      const auto set = packet.setSequenceCount(sequence);
      if (!set) std::abort();
    }
    auto encoded = packet.serialize();
    if (!encoded || encoded.value().size() != packetSize) std::abort();
    g_sink += encoded.value()[packetSize - 1U];
  });

  std::cout << label << ','
            << mode << ','
            << (crcEnabled ? "crc16" : "none") << ','
            << packetSize << ','
            << std::fixed << std::setprecision(2)
            << sample.ns_per_op << ','
            << sample.allocations_per_op << ','
            << sample.allocated_bytes_per_op << '\n';
}
} // namespace

void *operator new(const std::size_t size) {
  recordAllocation(size);
  if (void *ptr = std::malloc(size)) return ptr;
  throw std::bad_alloc{};
}
void *operator new[](const std::size_t size) {
  recordAllocation(size);
  if (void *ptr = std::malloc(size)) return ptr;
  throw std::bad_alloc{};
}
void operator delete(void *ptr) noexcept { std::free(ptr); }
void operator delete[](void *ptr) noexcept { std::free(ptr); }
void operator delete(void *ptr, std::size_t) noexcept { std::free(ptr); }
void operator delete[](void *ptr, std::size_t) noexcept { std::free(ptr); }

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: serialization-benchmark <label>\n";
    return 2;
  }
  constexpr std::size_t sizes[] = {64U, 256U, 1024U, 8192U, 32768U};
  std::cout << "label,mode,pec,packet_bytes,ns_per_op,allocations_per_op,allocated_bytes_per_op\n";
  try {
    for (const std::string mode : {"cached", "refinalize"}) {
      for (const bool crc : {false, true}) {
        for (const auto size : sizes) runCase(argv[1], mode, size, crc);
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "serialization benchmark failed: " << e.what() << '\n';
    return 1;
  }
  return g_sink == static_cast<std::size_t>(-1) ? 3 : 0;
}
