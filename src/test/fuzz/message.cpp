// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/signmessage.h>
#include <key_io.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

void initialize_message()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(message, .init = initialize_message)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string random_message = fuzzed_data_provider.ConsumeRandomLengthString(1024);
    {
        CKey private_key = ConsumePrivateKey(fuzzed_data_provider);
        std::string signature;
        const bool message_signed = MessageSign(private_key, random_message, signature);
        if (private_key.IsValid()) {
            assert(message_signed);
            const MessageVerificationResult verification_result = MessageVerify(EncodeDestination(PKHash(private_key.GetPubKey().GetID())), signature, random_message);
            assert(verification_result == MessageVerificationResult::OK);
        }
    }
    {
        (void)MessageHash(random_message);
        auto address = fuzzed_data_provider.ConsumeRandomLengthString(1024);
        auto signature = fuzzed_data_provider.ConsumeRandomLengthString(1024);
        (void)MessageVerify(address, signature, random_message);
        (void)SigningResultString(fuzzed_data_provider.PickValueInArray({SigningResult::OK, SigningResult::PRIVATE_KEY_NOT_AVAILABLE, SigningResult::SIGNING_FAILED}));
    }
}
