// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSBuffer.h>
#include <ccsdspack/c/ccsdspack.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
std::atomic<std::size_t> g_allocations{0U};
std::atomic<std::size_t> g_allocated_bytes{0U};
thread_local bool g_track_allocations = false;
volatile std::size_t g_sink = 0U;

struct AllocationSample {
  std::size_t allocations{};
  std::size_t bytes{};
};

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

std::vector<std::uint8_t> makePacket(const std::size_t packetSize,
                                     const bool crcEnabled) {
  const std::size_t pecSize = crcEnabled ? 2U : 0U;
  if (packetSize < CCSDS_PRIMARY_HEADER_SIZE + 1U + pecSize) {
    throw std::runtime_error("packet size is too small for requested PEC mode");
  }

  const std::size_t bodySize = packetSize - CCSDS_PRIMARY_HEADER_SIZE;
  if (bodySize > 65536U) {
    throw std::runtime_error("packet body exceeds the CCSDS Packet Data Length field");
  }

  const ccsds_primary_header_t header{
    0U,
    0U,
    0U,
    0x0042U,
    3U,
    0x0123U,
    static_cast<std::uint16_t>(bodySize - 1U)
  };

  std::vector<std::uint8_t> packet(packetSize, 0U);
  if (ccsds_primary_header_encode(&header, packet.data(), packet.size())
      != CCSDS_STATUS_OK) {
    throw std::runtime_error("failed to encode benchmark primary header");
  }

  const std::size_t payloadEnd = packetSize - pecSize;
  for (std::size_t index = CCSDS_PRIMARY_HEADER_SIZE; index < payloadEnd; ++index) {
    packet[index] = static_cast<std::uint8_t>((index * 37U + packetSize) & 0xFFU);
  }

  if (crcEnabled) {
    std::uint16_t crc = 0U;
    if (ccsds_crc16_ccitt_false(packet.data(), packetSize - pecSize, &crc)
        != CCSDS_STATUS_OK) {
      throw std::runtime_error("failed to calculate benchmark CRC");
    }
    ccsds_store_be16(packet.data() + packetSize - pecSize, crc);
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

void validateZeroCopy(const std::vector<std::uint8_t> &packet,
                      const bool crcEnabled) {
  ccsds_packet_view_t view{};
  const auto status = ccsds_packet_view_parse(
    packet.data(),
    packet.size(),
    crcEnabled ? CCSDS_PACKET_ERROR_CONTROL_CRC16
               : CCSDS_PACKET_ERROR_CONTROL_NONE,
    nullptr,
    &view);
  if (status != CCSDS_STATUS_OK) {
    throw std::runtime_error("C zero-copy parser rejected benchmark packet");
  }
  if (view.packet.data != packet.data()
      || view.data_field.data != packet.data() + CCSDS_PRIMARY_HEADER_SIZE
      || view.consumed != packet.size()) {
    throw std::runtime_error("C packet view stopped aliasing the caller buffer");
  }
}

void runCase(const std::size_t packetSize, const bool crcEnabled) {
  const auto packet = makePacket(packetSize, crcEnabled);
  const auto iterations = iterationsFor(packetSize);
  validateZeroCopy(packet, crcEnabled);

  ccsds_packet_view_t view{};
  const auto cSample = measure(iterations, [&] {
    const auto status = ccsds_packet_view_parse(
      packet.data(),
      packet.size(),
      crcEnabled ? CCSDS_PACKET_ERROR_CONTROL_CRC16
                 : CCSDS_PACKET_ERROR_CONTROL_NONE,
      nullptr,
      &view);
    if (status != CCSDS_STATUS_OK) std::abort();
    g_sink += view.primary_header.apid + view.data_field.size;
  });

  if (cSample.allocations_per_op != 0.0 || cSample.allocated_bytes_per_op != 0.0) {
    throw std::runtime_error("C packet view performed a heap allocation");
  }

  ccsds::Packet cppPacket;
  cppPacket.setDataFieldSize(65535U);
  cppPacket.setPacketErrorControlMode(
    crcEnabled ? ccsds::PacketErrorControlMode::CRC16
               : ccsds::PacketErrorControlMode::None);

  const auto warmResult =
    ccsds::buffer::deserializeBounded(cppPacket, packet.data(), packet.size());
  if (!warmResult || warmResult.value() != packet.size()) {
    throw std::runtime_error("C++ owning parser rejected benchmark packet");
  }

  const auto cppSample = measure(iterations, [&] {
    const auto parsed =
      ccsds::buffer::deserializeBounded(cppPacket, packet.data(), packet.size());
    if (!parsed || parsed.value() != packet.size()) std::abort();
    g_sink += cppPacket.getPrimaryHeader().getAPID();
  });

  std::cout
    << "c_zero_copy,"
    << (crcEnabled ? "crc16" : "none") << ','
    << packetSize << ','
    << iterations << ','
    << std::fixed << std::setprecision(2)
    << cSample.ns_per_op << ','
    << cSample.allocations_per_op << ','
    << cSample.allocated_bytes_per_op << ','
    << 0U << '\n';

  std::cout
    << "cpp_owning_raw,"
    << (crcEnabled ? "crc16" : "none") << ','
    << packetSize << ','
    << iterations << ','
    << std::fixed << std::setprecision(2)
    << cppSample.ns_per_op << ','
    << cppSample.allocations_per_op << ','
    << cppSample.allocated_bytes_per_op << ','
    << packetSize << '\n';
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

void operator delete(void *ptr) noexcept {
  std::free(ptr);
}

void operator delete[](void *ptr) noexcept {
  std::free(ptr);
}

void operator delete(void *ptr, std::size_t) noexcept {
  std::free(ptr);
}

void operator delete[](void *ptr, std::size_t) noexcept {
  std::free(ptr);
}

int main() {
  constexpr std::size_t packetSizes[] = {64U, 256U, 1024U, 8192U, 32768U};

  std::cout << "CCSDSPACK_PERFORMANCE_BASELINE_V1\n";
  std::cout
    << "path,pec,packet_bytes,iterations,ns_per_op,allocations_per_op,"
       "allocated_bytes_per_op,mandatory_input_copy_bytes_per_op\n";

  try {
    for (const bool crcEnabled : {false, true}) {
      for (const auto packetSize : packetSizes) {
        runCase(packetSize, crcEnabled);
      }
    }
  } catch (const std::exception &error) {
    std::cerr << "CCSDSPACK_PERFORMANCE_BASELINE:FAIL: " << error.what() << '\n';
    return 1;
  }

  std::cout << "CCSDSPACK_PERFORMANCE_BASELINE:PASS\n";
  return g_sink == static_cast<std::size_t>(-1) ? 2 : 0;
}
