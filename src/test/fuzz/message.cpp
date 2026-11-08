// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <common/signmessage.h>
#include <crypto/common.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/script_error.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <array>
#include <cassert>
#include <cstddef>
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
    // BIP322 seeds: selector, 20/32-byte witness program, LE16 message length,
    // message, proof. Selector low bits choose WPKH/WSH/TR/unknown v2; upper
    // bits choose binary witness with no/smp/ful/pof prefix, or raw proof text.
    if (buffer.size() >= 23) {
        const auto type = buffer[0] & 3;
        const size_t program_size = type == 0 ? 20 : 32;
        const size_t header_size = 1 + program_size + 2;
        if (buffer.size() >= header_size) {
            const auto program = buffer.subspan(1, program_size);
            const size_t message_size = ReadLE16(buffer.data() + 1 + program_size);
            if (message_size <= buffer.size() - header_size) {
                const auto destination = [&]() -> CTxDestination {
                    switch (type) {
                    case 0: return WitnessV0KeyHash{uint160{program}};
                    case 1: return WitnessV0ScriptHash{uint256{program}};
                    case 2: return WitnessV1Taproot{XOnlyPubKey{program}};
                    default: return WitnessUnknown{2, {program.begin(), program.end()}};
                    }
                }();
                const std::string message{buffer.begin() + header_size, buffer.begin() + header_size + message_size};
                const auto proof = buffer.subspan(header_size + message_size);
                const auto mode = (buffer[0] >> 2) % 5;
                const std::string signature = mode == 4 ? std::string{proof.begin(), proof.end()} :
                    std::array{"", "smp", "ful", "pof"}[mode] + EncodeBase64(proof);
                ScriptError error;
                const auto result = BIP322Verify(destination, signature, message, &error);
                if (mode < 2) {
                    const auto alternate = mode == 0 ? "smp" + signature : signature.substr(3);
                    assert(result == BIP322Verify(destination, alternate, message));
                }
                if (type == 3) assert(result != BIP322VerificationResult::OK);
            }
        }
    }

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
