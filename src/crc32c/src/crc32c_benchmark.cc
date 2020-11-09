// Copyright 2017 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <cstddef>
#include <cstdint>

#ifdef CRC32C_HAVE_CONFIG_H
#include "crc32c/crc32c_config.h"
#endif

#include "benchmark/benchmark.h"

#if CRC32C_TESTS_BUILT_WITH_GLOG
#include "glog/logging.h"
#endif  // CRC32C_TESTS_BUILT_WITH_GLOG

#include "./crc32c_arm64.h"
#include "./crc32c_arm64_linux_check.h"
#include "./crc32c_internal.h"
#include "./crc32c_sse42.h"
#include "./crc32c_sse42_check.h"
#include "crc32c/crc32c.h"

class CRC32CBenchmark : public benchmark::Fixture {
 public:
  void SetUp(const benchmark::State& state) override {
    block_size_ = static_cast<size_t>(state.range(0));
    block_data_ = std::string(block_size_, 'x');
    block_buffer_ = reinterpret_cast<const uint8_t*>(block_data_.data());
  }

 protected:
  std::string block_data_;
  const uint8_t* block_buffer_;
  size_t block_size_;
};

BENCHMARK_DEFINE_F(CRC32CBenchmark, Public)(benchmark::State& state) {
  uint32_t crc = 0;
  for (auto _ : state)
    crc = crc32c::Extend(crc, block_buffer_, block_size_);
  state.SetBytesProcessed(state.iterations() * block_size_);
}
BENCHMARK_REGISTER_F(CRC32CBenchmark, Public)
    ->RangeMultiplier(16)
    ->Range(256, 16777216);  // Block size.

BENCHMARK_DEFINE_F(CRC32CBenchmark, Portable)(benchmark::State& state) {
  uint32_t crc = 0;
  for (auto _ : state)
    crc = crc32c::ExtendPortable(crc, block_buffer_, block_size_);
  state.SetBytesProcessed(state.iterations() * block_size_);
}
BENCHMARK_REGISTER_F(CRC32CBenchmark, Portable)
    ->RangeMultiplier(16)
    ->Range(256, 16777216);  // Block size.

#if HAVE_ARM64_CRC32C

BENCHMARK_DEFINE_F(CRC32CBenchmark, ArmLinux)(benchmark::State& state) {
  if (!crc32c::CanUseArm64Linux()) {
    state.SkipWithError("ARM CRC32C instructions not available or not enabled");
    return;
  }

  uint32_t crc = 0;
  for (auto _ : state)
    crc = crc32c::ExtendArm64(crc, block_buffer_, block_size_);
  state.SetBytesProcessed(state.iterations() * block_size_);
}
BENCHMARK_REGISTER_F(CRC32CBenchmark, ArmLinux)
    ->RangeMultiplier(16)
    ->Range(256, 16777216);  // Block size.

#endif  // HAVE_ARM64_CRC32C

#if HAVE_SSE42 && (defined(_M_X64) || defined(__x86_64__))

BENCHMARK_DEFINE_F(CRC32CBenchmark, Sse42)(benchmark::State& state) {
  if (!crc32c::CanUseSse42()) {
    state.SkipWithError("SSE4.2 instructions not available or not enabled");
    return;
  }

  uint32_t crc = 0;
  for (auto _ : state)
    crc = crc32c::ExtendSse42(crc, block_buffer_, block_size_);
  state.SetBytesProcessed(state.iterations() * block_size_);
}
BENCHMARK_REGISTER_F(CRC32CBenchmark, Sse42)
    ->RangeMultiplier(16)
    ->Range(256, 16777216);  // Block size.

#endif  // HAVE_SSE42 && (defined(_M_X64) || defined(__x86_64__))

int main(int argc, char** argv) {
#if CRC32C_TESTS_BUILT_WITH_GLOG
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif  // CRC32C_TESTS_BUILT_WITH_GLOG

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}
