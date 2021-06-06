// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "bls.hpp"

#if BLSALLOC_SODIUM
#include "sodium.h"
#endif

namespace bls {

bool BLSInitResult = BLS::Init();

Util::SecureAllocCallback Util::secureAllocCallback;
Util::SecureFreeCallback Util::secureFreeCallback;

static void relic_core_initializer(void* ptr)
{
    core_init();
    if (err_get_code() != RLC_OK) {
        throw std::runtime_error("core_init() failed");
    }

    const int r = ep_param_set_any_pairf();
    if (r != RLC_OK) {
        throw std::runtime_error("ep_param_set_any_pairf() failed");
    }
}

bool BLS::Init()
{
    if (ALLOC != AUTO) {
        throw std::runtime_error("Must have ALLOC == AUTO");
    }
#if BLSALLOC_SODIUM
    if (sodium_init() < 0) {
        throw std::runtime_error("libsodium init failed");
    }
    SetSecureAllocator(sodium_malloc, sodium_free);
#else
    SetSecureAllocator(malloc, free);
#endif

#if MULTI != RELIC_NONE
    core_set_thread_initializer(relic_core_initializer, nullptr);
#else
    relic_core_initializer(nullptr);
#endif
    
    return true;
}

void BLS::SetSecureAllocator(
    Util::SecureAllocCallback allocCb,
    Util::SecureFreeCallback freeCb)
{
    Util::secureAllocCallback = allocCb;
    Util::secureFreeCallback = freeCb;
}


void BLS::CheckRelicErrors()
{
    if (!core_get()) {
        throw std::runtime_error("Library not initialized properly. Call BLS::Init()");
    }
    if (core_get()->code != RLC_OK) {
        core_get()->code = RLC_OK;
        throw std::invalid_argument("Relic library error");
    }
}

}  // end namespace bls
