// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0
//
// Diagnostic benchmark only: no protocol implementation changes.
// Measures repeated same-process C11 codec, C++ wrapper, validation and
// owning-output costs, reporting median latency and heap allocation counts.

#include <CCSDSTime.h>
#include <ccsdspack/c/cuc.h>
#include <algorithm>
#include <array>
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
std::atomic<std::size_t> allocations{0U};
std::atomic<std::size_t> bytes_allocated{0U};
thread_local bool track_allocations = false;
volatile std::uint64_t sink = 0U;
constexpr std::size_t iterations = 200000U;

void record(const std::size_t size) noexcept {
  if (!track_allocations) return;
  allocations.fetch_add(1U, std::memory_order_relaxed);
  bytes_allocated.fetch_add(size, std::memory_order_relaxed);
}

struct Sample {
  double median_ns{};
  double min_ns{};
  double allocations_per_op{};
  double bytes_per_op{};
};

template <typename Func>
Sample measure(Func &&func) {
  for (std::size_t i = 0U; i < 256U; ++i) func();
  std::array<double, 7U> ns{};
  double alloc_per_op = 0.0;
  double bytes_per_op = 0.0;
  for (std::size_t round = 0U; round < ns.size(); ++round) {
    allocations.store(0U, std::memory_order_relaxed);
    bytes_allocated.store(0U, std::memory_order_relaxed);
    track_allocations = true;
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0U; i < iterations; ++i) func();
    const auto finish = std::chrono::steady_clock::now();
    track_allocations = false;
    ns[round] = static_cast<double>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count()
      ) / static_cast<double>(iterations);
    alloc_per_op += static_cast<double>(
      allocations.load(std::memory_order_relaxed)) / static_cast<double>(iterations);
    bytes_per_op += static_cast<double>(
      bytes_allocated.load(std::memory_order_relaxed)) / static_cast<double>(iterations);
  }
  std::sort(ns.begin(), ns.end());
  return {ns[3U], ns[0U], alloc_per_op / 7.0, bytes_per_op / 7.0};
}

template <typename Func>
void emit(const char *label, Func &&func) {
  const Sample s = measure(func);
  std::cout << label << ',' << iterations << ','
            << std::fixed << std::setprecision(3)
            << s.median_ns << ',' << s.min_ns << ','
            << s.allocations_per_op << ',' << s.bytes_per_op << '\n';
}

void require(const bool ok) {
  if (!ok) throw std::runtime_error("CUC path benchmark setup or codec failed");
}
}  // namespace

void *operator new(std::size_t size) {
  record(size);
  if (void *p = std::malloc(size)) return p;
  throw std::bad_alloc{};
}
void *operator new[](std::size_t size) {
  record(size);
  if (void *p = std::malloc(size)) return p;
  throw std::bad_alloc{};
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

int main() {
  try {
    const ccsds_cuc_config_t ccfg{
      CCSDS_CUC_EPOCH_CCSDS_1958_TAI,
      CCSDS_CUC_PFIELD_EXPLICIT, 4U, 2U
    };
    const ccsds_cuc_time_t ctime{0x10203040U, 0x4321U};
    const ccsds::time::CucConfiguration cppcfg{
      ccsds::time::Epoch::Ccsds1958Tai,
      ccsds::time::PFieldMode::Explicit, 4U, 2U
    };
    const ccsds::time::CucTime cpptime{ctime.coarse, ctime.fine};
    std::array<std::uint8_t, 8U> bytes{};
    std::size_t written = 0U;
    require(ccsds_cuc_encode(&ctime, &ccfg, bytes.data(), bytes.size(), &written)
            == CCSDS_STATUS_OK);
    require(written == 7U);
    const std::vector<std::uint8_t> input(bytes.begin(), bytes.begin() + written);
    const auto cppEncoded = ccsds::time::serialize(cpptime, cppcfg);
    require(cppEncoded && cppEncoded.value() == input);

    std::cout << "operation,iterations,median_ns_per_op,min_ns_per_op,"
                 "allocations_per_op,allocated_bytes_per_op\n";

    emit("c_core_validate", [&] {
      require(ccsds_cuc_validate(&ccfg) == CCSDS_STATUS_OK);
      sink += ccfg.coarse_octets;
    });
    emit("c_core_size", [&] {
      sink += ccsds_cuc_encoded_size(&ccfg);
    });
    emit("c_core_encode_caller_buffer", [&] {
      std::array<std::uint8_t, 8U> output{};
      std::size_t count = 0U;
      require(ccsds_cuc_encode(&ctime, &ccfg, output.data(), output.size(), &count)
              == CCSDS_STATUS_OK && count == written);
      sink += output[count - 1U];
    });
    emit("cpp_vector_construct_only", [&] {
      std::vector<std::uint8_t> output(written);
      sink += output.back();
    });
    emit("c_core_encode_into_vector", [&] {
      std::vector<std::uint8_t> output(written);
      std::size_t count = 0U;
      require(ccsds_cuc_encode(&ctime, &ccfg, output.data(), output.size(), &count)
              == CCSDS_STATUS_OK && count == written);
      sink += output.back();
    });
    emit("cpp_wrapper_encode", [&] {
      const auto encoded = ccsds::time::serialize(cpptime, cppcfg);
      require(encoded && encoded.value().size() == written);
      sink += encoded.value().back();
    });
    emit("c_core_decode_caller_buffer", [&] {
      ccsds_cuc_time_t decoded{};
      require(ccsds_cuc_decode(bytes.data(), written, &ccfg, &decoded)
              == CCSDS_STATUS_OK);
      sink += decoded.coarse & 255U;
    });
    emit("cpp_wrapper_decode", [&] {
      const auto decoded = ccsds::time::deserialize(input, cppcfg);
      require(decoded && decoded.value() == cpptime);
      sink += decoded.value().coarse & 255U;
    });
    std::cout << "CCSDSPACK_CUC_PATH_PROFILE:PASS\n";
    return sink == UINT64_MAX ? 3 : 0;
  } catch (const std::exception &e) {
    std::cerr << "CUC path profile failed: " << e.what() << '\n';
    return 1;
  }
}
