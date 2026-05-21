// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/util/rpc.h>

#include <base58.h>
#include <key.h>
#include <key_io.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <span.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

using util::Join;
using util::ToString;

namespace fuzzrpc {
std::string ConsumeScalarRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data)
{
    const size_t max_string_length = 4096;
    const size_t max_base58_bytes_length{64};
    std::string r;
    CallOneOf(
        fuzzed_data_provider,
        [&] {
            // string argument
            r = fuzzed_data_provider.ConsumeRandomLengthString(max_string_length);
        },
        [&] {
            // base64 argument
            r = EncodeBase64(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
        },
        [&] {
            // hex argument
            r = HexStr(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
        },
        [&] {
            // bool argument
            r = fuzzed_data_provider.ConsumeBool() ? "true" : "false";
        },
        [&] {
            // range argument
            r = "[" + ToString(fuzzed_data_provider.ConsumeIntegral<int64_t>()) + "," + ToString(fuzzed_data_provider.ConsumeIntegral<int64_t>()) + "]";
        },
        [&] {
            // integral argument (int64_t)
            r = ToString(fuzzed_data_provider.ConsumeIntegral<int64_t>());
        },
        [&] {
            // integral argument (uint64_t)
            r = ToString(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
        },
        [&] {
            // floating point argument
            r = strprintf("%f", fuzzed_data_provider.ConsumeFloatingPoint<double>());
        },
        [&] {
            // tx destination argument
            r = EncodeDestination(ConsumeTxDestination(fuzzed_data_provider));
        },
        [&] {
            // uint160 argument
            r = ConsumeUInt160(fuzzed_data_provider).ToString();
        },
        [&] {
            // uint256 argument
            r = ConsumeUInt256(fuzzed_data_provider).ToString();
        },
        [&] {
            // base32 argument
            r = EncodeBase32(fuzzed_data_provider.ConsumeRandomLengthString(max_string_length));
        },
        [&] {
            // base58 argument
            r = EncodeBase58(MakeUCharSpan(fuzzed_data_provider.ConsumeRandomLengthString(max_base58_bytes_length)));
        },
        [&] {
            // base58 argument with checksum
            r = EncodeBase58Check(MakeUCharSpan(fuzzed_data_provider.ConsumeRandomLengthString(max_base58_bytes_length)));
        },
        [&] {
            // hex encoded block
            std::optional<CBlock> opt_block = ConsumeDeserializable<CBlock>(fuzzed_data_provider, TX_WITH_WITNESS);
            if (!opt_block) {
                good_data = false;
                return;
            }
            DataStream data_stream{};
            data_stream << TX_WITH_WITNESS(*opt_block);
            r = HexStr(data_stream);
        },
        [&] {
            // hex encoded block header
            std::optional<CBlockHeader> opt_block_header = ConsumeDeserializable<CBlockHeader>(fuzzed_data_provider);
            if (!opt_block_header) {
                good_data = false;
                return;
            }
            DataStream data_stream{};
            data_stream << *opt_block_header;
            r = HexStr(data_stream);
        },
        [&] {
            // hex encoded tx
            std::optional<CMutableTransaction> opt_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
            if (!opt_tx) {
                good_data = false;
                return;
            }
            DataStream data_stream;
            auto allow_witness = (fuzzed_data_provider.ConsumeBool() ? TX_WITH_WITNESS : TX_NO_WITNESS);
            data_stream << allow_witness(*opt_tx);
            r = HexStr(data_stream);
        },
        [&] {
            // base64 encoded psbt
            std::optional<PartiallySignedTransaction> opt_psbt = ConsumeDeserializableConstructor<PartiallySignedTransaction>(fuzzed_data_provider);
            if (!opt_psbt) {
                good_data = false;
                return;
            }
            DataStream data_stream{};
            data_stream << *opt_psbt;
            r = EncodeBase64(data_stream);
        },
        [&] {
            // base58 encoded key
            CKey key = ConsumePrivateKey(fuzzed_data_provider);
            if (!key.IsValid()) {
                good_data = false;
                return;
            }
            r = EncodeSecret(key);
        },
        [&] {
            // hex encoded pubkey
            CKey key = ConsumePrivateKey(fuzzed_data_provider);
            if (!key.IsValid()) {
                good_data = false;
                return;
            }
            r = HexStr(key.GetPubKey());
        });
    return r;
}

std::string ConsumeArrayRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data)
{
    std::vector<std::string> scalar_arguments;
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 100)
    {
        scalar_arguments.push_back(ConsumeScalarRPCArgument(fuzzed_data_provider, good_data));
    }
    return "[\"" + Join(scalar_arguments, "\",\"") + "\"]";
}

std::string ConsumeRPCArgument(FuzzedDataProvider& fuzzed_data_provider, bool& good_data)
{
    return fuzzed_data_provider.ConsumeBool() ? ConsumeScalarRPCArgument(fuzzed_data_provider, good_data) : ConsumeArrayRPCArgument(fuzzed_data_provider, good_data);
}

void ValidateRpcCommands(const std::vector<std::string>& rpc_commands_safe_for_fuzzing,
                         const std::vector<std::string>& rpc_commands_not_safe_for_fuzzing,
                         const std::vector<std::string>& supported_rpc_commands,
                         const std::string_view source_file)
{
    for (const std::string& rpc_command : supported_rpc_commands) {
        const bool safe_for_fuzzing = std::find(rpc_commands_safe_for_fuzzing.begin(), rpc_commands_safe_for_fuzzing.end(), rpc_command) != rpc_commands_safe_for_fuzzing.end();
        const bool not_safe_for_fuzzing = std::find(rpc_commands_not_safe_for_fuzzing.begin(), rpc_commands_not_safe_for_fuzzing.end(), rpc_command) != rpc_commands_not_safe_for_fuzzing.end();
        if (!(safe_for_fuzzing || not_safe_for_fuzzing)) {
            std::cerr << "Error: RPC command \"" << rpc_command << "\" not found in rpc_commands_safe_for_fuzzing or rpc_commands_not_safe_for_fuzzing. Please update " << source_file << ".\n";
            std::terminate();
        }
        if (safe_for_fuzzing && not_safe_for_fuzzing) {
            std::cerr << "Error: RPC command \"" << rpc_command << "\" found in *both* rpc_commands_safe_for_fuzzing and rpc_commands_not_safe_for_fuzzing. Please update " << source_file << ".\n";
            std::terminate();
        }
    }
}
} // namespace fuzzrpc
