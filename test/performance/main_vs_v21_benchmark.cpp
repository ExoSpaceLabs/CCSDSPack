// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSBuffer.h>

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

std::uint16_t crc16(const std::uint8_t *data, const std::size_t size) {
  std::uint16_t crc = 0xFFFFU;
  for (std::size_t i = 0U; i < size; ++i) {
    crc ^= static_cast<std::uint16_t>(data[i]) << 8U;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000U) != 0U
        ? static_cast<std::uint16_t>((crc << 1U) ^ 0x1021U)
        : static_cast<std::uint16_t>(crc << 1U);
    }
  }
  return crc;
}

std::vector<std::uint8_t> makePacket(const std::size_t packetSize,
                                     const bool crcEnabled) {
  const std::size_t pecSize = crcEnabled ? 2U : 0U;
  if (packetSize < 6U + 1U + pecSize) {
    throw std::runtime_error("packet too small");
  }
  const std::size_t bodySize = packetSize - 6U;
  if (bodySize > 65536U) {
    throw std::runtime_error("packet too large");
  }

  const std::uint16_t packetId = 0x0042U;
  const std::uint16_t sequence = static_cast<std::uint16_t>(0xC000U | 0x0123U);
  const std::uint16_t dataLength = static_cast<std::uint16_t>(bodySize - 1U);

  std::vector<std::uint8_t> packet(packetSize, 0U);
  packet[0] = static_cast<std::uint8_t>(packetId >> 8U);
  packet[1] = static_cast<std::uint8_t>(packetId);
  packet[2] = static_cast<std::uint8_t>(sequence >> 8U);
  packet[3] = static_cast<std::uint8_t>(sequence);
  packet[4] = static_cast<std::uint8_t>(dataLength >> 8U);
  packet[5] = static_cast<std::uint8_t>(dataLength);

  const std::size_t payloadEnd = packetSize - pecSize;
  for (std::size_t i = 6U; i < payloadEnd; ++i) {
    packet[i] = static_cast<std::uint8_t>((i * 37U + packetSize) & 0xFFU);
  }

  if (crcEnabled) {
    const auto crc = crc16(packet.data(), packetSize - 2U);
    packet[packetSize - 2U] = static_cast<std::uint8_t>(crc >> 8U);
    packet[packetSize - 1U] = static_cast<std::uint8_t>(crc);
  }
  return packet;
}

template <typename F>
PerfSample measure(const std::size_t iterations, F &&operation) {
  for (std::size_t i = 0U; i < 32U; ++i) operation();

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
  if (packetSize <= 64U) return 20000U;
  if (packetSize <= 256U) return 10000U;
  if (packetSize <= 1024U) return 4000U;
  if (packetSize <= 8192U) return 1000U;
  return 250U;
}

void runCase(const std::string &label,
             const std::size_t packetSize,
             const bool crcEnabled) {
  const auto packet = makePacket(packetSize, crcEnabled);
  ccsds::Packet parsed;
  parsed.setDataFieldSize(65535U);
  parsed.setPacketErrorControlMode(
    crcEnabled ? ccsds::PacketErrorControlMode::CRC16
               : ccsds::PacketErrorControlMode::None);

  const auto warm =
    ccsds::buffer::deserializeBounded(parsed, packet.data(), packet.size());
  if (!warm || warm.value() != packet.size()) {
    throw std::runtime_error("parser rejected benchmark packet");
  }

  const auto sample = measure(iterationsFor(packetSize), [&] {
    const auto result =
      ccsds::buffer::deserializeBounded(parsed, packet.data(), packet.size());
    if (!result || result.value() != packet.size()) std::abort();
    g_sink += parsed.getPrimaryHeader().getAPID();
  });

  std::cout << label << ','
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
    std::cerr << "usage: benchmark <label>\n";
    return 2;
  }
  constexpr std::size_t sizes[] = {64U, 256U, 1024U, 8192U, 32768U};
  std::cout << "label,pec,packet_bytes,ns_per_op,allocations_per_op,allocated_bytes_per_op\n";
  try {
    for (const bool crc : {false, true}) {
      for (const auto size : sizes) runCase(argv[1], size, crc);
    }
  } catch (const std::exception &e) {
    std::cerr << "benchmark failed: " << e.what() << '\n';
    return 1;
  }
  return g_sink == static_cast<std::size_t>(-1) ? 3 : 0;
}
