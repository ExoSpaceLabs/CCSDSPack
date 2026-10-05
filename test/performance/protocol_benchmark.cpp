// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <CCSDSManager.h>
#include <CCSDSPacket.h>
#include <CCSDSTime.h>
#include <CCSDSValidator.h>
#include <PusSecondaryHeaders.h>
#include <PusTailoring.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
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

void emit(const std::string &label,
          const std::string &operation,
          const std::string &variant,
          const std::size_t iterations,
          const PerfSample &sample) {
  std::cout << label << ','
            << operation << ','
            << variant << ','
            << iterations << ','
            << std::fixed << std::setprecision(2)
            << sample.ns_per_op << ','
            << sample.allocations_per_op << ','
            << sample.allocated_bytes_per_op << '\n';
}

template <typename Header>
void benchHeader(const std::string &label,
                 const std::string &name,
                 Header header,
                 const std::size_t iterations) {
  const auto encoded = header.serialize();
  if (encoded.empty()) throw std::runtime_error(name + " serialized empty");

  const auto enc = measure(iterations, [&] {
    auto bytes = header.serialize();
    if (bytes.empty()) std::abort();
    g_sink += bytes.back();
  });
  emit(label, "pus_encode", name, iterations, enc);

  Header parsed = header;
  const auto dec = measure(iterations, [&] {
    auto result = parsed.deserialize(encoded);
    if (!result) std::abort();
    g_sink += parsed.getServiceType();
  });
  emit(label, "pus_decode", name, iterations, dec);
}

void benchPus(const std::string &label) {
  constexpr std::size_t iterations = 50000U;

  benchHeader(label, "pus_a_tc",
              ccsds::pus::rev_a::TcHeader(17U, 1U, 0x12U, 0x0FU),
              iterations);

  ccsds::pus::rev_a::TmTailoring aTmTailoring{};
  aTmTailoring.destinationIdOctets = 2U;
  aTmTailoring.packetSubcounterPresent = true;
  aTmTailoring.timestampPresent = true;
  aTmTailoring.cuc = {
    ccsds::time::Epoch::Ccsds1958Tai,
    ccsds::time::PFieldMode::Explicit,
    4U,
    2U
  };
  benchHeader(label, "pus_a_tm_timestamp",
              ccsds::pus::rev_a::TmHeader(
                aTmTailoring, 3U, 25U, 7U, 0x1234U, {0x10203040U, 0x4321U}),
              iterations);

  benchHeader(label, "pus_c_tc",
              ccsds::pus::rev_c::TcHeader(17U, 1U, 0x1234U, 0x0FU),
              iterations);

  ccsds::pus::rev_c::TmTailoring cTmTailoring{};
  cTmTailoring.timestampPresent = true;
  cTmTailoring.cuc = {
    ccsds::time::Epoch::Ccsds1958Tai,
    ccsds::time::PFieldMode::Explicit,
    4U,
    2U
  };
  benchHeader(label, "pus_c_tm_timestamp",
              ccsds::pus::rev_c::TmHeader(
                cTmTailoring, 3U, 25U, 0x2345U, 0x1234U, 0x0AU,
                {0x10203040U, 0x4321U}),
              iterations);
}

void benchCuc(const std::string &label) {
  constexpr std::size_t iterations = 100000U;
  const ccsds::time::CucConfiguration cfg{
    ccsds::time::Epoch::Ccsds1958Tai,
    ccsds::time::PFieldMode::Explicit,
    4U,
    2U
  };
  const ccsds::time::CucTime value{0x10203040U, 0x4321U};
  const auto bytesResult = ccsds::time::serialize(value, cfg);
  if (!bytesResult) throw std::runtime_error("CUC setup serialization failed");
  const auto bytes = bytesResult.value();

  const auto enc = measure(iterations, [&] {
    auto result = ccsds::time::serialize(value, cfg);
    if (!result) std::abort();
    g_sink += result.value().back();
  });
  emit(label, "cuc_encode", "explicit_4_2", iterations, enc);

  const auto dec = measure(iterations, [&] {
    auto result = ccsds::time::deserialize(bytes, cfg);
    if (!result) std::abort();
    g_sink += static_cast<std::size_t>(result.value().coarse & 0xFFU);
  });
  emit(label, "cuc_decode", "explicit_4_2", iterations, dec);
}

ccsds::Packet makePusPacket(const std::size_t payloadSize) {
  ccsds::Packet packet;
  packet.setDataFieldSize(65535U);
  packet.setPacketErrorControlMode(ccsds::PacketErrorControlMode::CRC16);
  const auto primary = packet.setPrimaryHeader(
    ccsds::PrimaryHeader{0U, 0U, 1U, 0x123U, ccsds::UNSEGMENTED, 0x11U, 0U});
  if (!primary) throw std::runtime_error("setPrimaryHeader failed");
  const auto secondary = packet.setSecondaryHeader(
    std::make_shared<ccsds::pus::rev_c::TcHeader>(17U, 1U, 0x1234U, 0x0FU));
  if (!secondary) throw std::runtime_error("setSecondaryHeader failed");

  std::vector<std::uint8_t> payload(payloadSize, 0U);
  for (std::size_t i = 0; i < payload.size(); ++i)
    payload[i] = static_cast<std::uint8_t>((i * 13U + payloadSize) & 0xFFU);
  const auto app = packet.setApplicationData(payload);
  if (!app) throw std::runtime_error("setApplicationData failed");
  const auto update = packet.update();
  if (!update) throw std::runtime_error("update failed");
  return packet;
}

void benchValidator(const std::string &label) {
  constexpr std::size_t iterations = 100000U;
  const auto packet = makePusPacket(256U);
  ccsds::Validator validator(packet);
  validator.configure(true, false, true);

  const auto sample = measure(iterations, [&] {
    const auto report = validator.validate(packet);
    if (!report) std::abort();
    g_sink += report.size();
  });
  emit(label, "validator", "pus_c_tc_crc16_256", iterations, sample);
}

void benchManager(const std::string &label) {
  constexpr std::size_t iterations = 3000U;
  auto templ = makePusPacket(16U);
  templ.setDataFieldSize(256U);

  std::vector<std::uint8_t> payload(4096U, 0U);
  for (std::size_t i = 0; i < payload.size(); ++i)
    payload[i] = static_cast<std::uint8_t>((i * 19U) & 0xFFU);

  ccsds::Manager generator;
  if (!generator.setPacketTemplate(templ))
    throw std::runtime_error("manager template setup failed");

  const auto segment = measure(iterations, [&] {
    generator.clearPackets();
    const auto result = generator.setApplicationData(payload);
    if (!result) std::abort();
    g_sink += generator.getTotalPackets();
  });
  emit(label, "manager_segment", "4096_to_256", iterations, segment);

  generator.clearPackets();
  if (!generator.setApplicationData(payload))
    throw std::runtime_error("manager fixture generation failed");
  const auto streamResult = generator.getPacketsBuffer();
  if (!streamResult) throw std::runtime_error("manager stream fixture failed");
  const auto stream = streamResult.value();

  ccsds::Manager receiver;
  if (!receiver.setPacketTemplate(templ))
    throw std::runtime_error("manager receiver template failed");

  const auto load = measure(iterations, [&] {
    receiver.clearPackets();
    const auto result = receiver.load(stream);
    if (!result) std::abort();
    g_sink += receiver.getTotalPackets();
  });
  emit(label, "manager_stream_load", "4096_from_256", iterations, load);

  receiver.clearPackets();
  if (!receiver.load(stream)) throw std::runtime_error("manager load setup failed");
  const auto reassembly = measure(iterations, [&] {
    const auto result = receiver.getApplicationDataBuffer();
    if (!result || result.value().size() != payload.size()) std::abort();
    g_sink += result.value().back();
  });
  emit(label, "manager_reassembly", "4096_from_256", iterations, reassembly);
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
    std::cerr << "usage: protocol-benchmark <label>\n";
    return 2;
  }

  std::cout << "label,operation,variant,iterations,ns_per_op,allocations_per_op,allocated_bytes_per_op\n";
  try {
    benchPus(argv[1]);
    benchCuc(argv[1]);
    benchValidator(argv[1]);
    benchManager(argv[1]);
  } catch (const std::exception &error) {
    std::cerr << "protocol benchmark failed: " << error.what() << '\n';
    return 1;
  }

  std::cout << "CCSDSPACK_PROTOCOL_BENCHMARK:PASS\n";
  return g_sink == static_cast<std::size_t>(-1) ? 3 : 0;
}
