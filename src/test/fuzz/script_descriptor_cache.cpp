// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <optional.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(script_descriptor_cache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    DescriptorCache descriptor_cache;
    while (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<uint8_t> code = fuzzed_data_provider.ConsumeBytes<uint8_t>(BIP32_EXTKEY_SIZE);
        if (code.size() == BIP32_EXTKEY_SIZE) {
            CExtPubKey xpub;
            xpub.Decode(code.data());
            const uint32_t key_exp_pos = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            CExtPubKey xpub_fetched;
            if (fuzzed_data_provider.ConsumeBool()) {
                (void)descriptor_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched);
                descriptor_cache.CacheParentExtPubKey(key_exp_pos, xpub);
                assert(descriptor_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched));
            } else {
                const uint32_t der_index = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
                (void)descriptor_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched);
                descriptor_cache.CacheDerivedExtPubKey(key_exp_pos, der_index, xpub);
                assert(descriptor_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched));
            }
            assert(xpub == xpub_fetched);
        }
        (void)descriptor_cache.GetCachedParentExtPubKeys();
        (void)descriptor_cache.GetCachedDerivedExtPubKeys();
    }
}
