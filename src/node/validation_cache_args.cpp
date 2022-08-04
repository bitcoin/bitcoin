// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/validation_cache_args.h>

#include <kernel/validation_cache_sizes.h>

#include <util/system.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

using kernel::ValidationCacheSizes;

namespace node {
void ApplyArgsManOptions(const ArgsManager& argsman, ValidationCacheSizes& cache_sizes)
{
    if (auto max_size = argsman.GetIntArg("-maxsigcachesize")) {
        // 1. When supplied with a max_size of 0, both InitSignatureCache and
        //    InitScriptExecutionCache create the minimum possible cache (2
        //    elements). Therefore, we can use 0 as a floor here.
        // 2. Multiply first, divide after to avoid integer truncation.
        size_t clamped_size_each = std::max<int64_t>(*max_size, 0) * (1 << 20) / 2;
        cache_sizes = {
            .signature_cache_bytes = clamped_size_each,
            .script_execution_cache_bytes = clamped_size_each,
        };
    }
}
} // namespace node
