// Copyright (c) 2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/context.h>

#include <crypto/sha256.h>
#include <logging.h>
#include <random.h>

#include <mutex>
#include <string>

namespace kernel {
Context::Context()
{
    static std::once_flag globals_initialized{};
    std::call_once(globals_initialized, []() {
        std::string sha256_algo = SHA256AutoDetect();
        LogInfo("Using the '%s' SHA256 implementation\n", sha256_algo);
        RandomInit();
    });
}


} // namespace kernel
