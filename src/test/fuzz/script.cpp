// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <compressor.h>
#include <core_io.h>
#include <core_memusage.h>
#include <key_io.h>
#include <policy/policy.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <univalue.h>
#include <util/chaintype.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void initialize_script()
{
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(script, .init = initialize_script)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CScript script{ConsumeScript(fuzzed_data_provider)};

    CompressedScript compressed;
    if (CompressScript(script, compressed)) {
        const unsigned int size = compressed[0];
        compressed.erase(compressed.begin());
        assert(size <= 5);
        CScript decompressed_script;
        const bool ok = DecompressScript(decompressed_script, size, compressed);
        assert(ok);
        assert(script == decompressed_script);
    }

    TxoutType which_type;
    bool is_standard_ret = IsStandard(script, which_type);
    if (!is_standard_ret) {
        assert(which_type == TxoutType::NONSTANDARD ||
               which_type == TxoutType::NULL_DATA ||
               which_type == TxoutType::MULTISIG);
    }
    if (which_type == TxoutType::NONSTANDARD) {
        assert(!is_standard_ret);
    }
    if (which_type == TxoutType::NULL_DATA) {
        assert(script.IsUnspendable());
    }
    if (script.IsUnspendable()) {
        assert(which_type == TxoutType::NULL_DATA ||
               which_type == TxoutType::NONSTANDARD);
    }

    CTxDestination address;
    bool extract_destination_ret = ExtractDestination(script, address);
    if (!extract_destination_ret) {
        assert(which_type == TxoutType::PUBKEY ||
               which_type == TxoutType::NONSTANDARD ||
               which_type == TxoutType::NULL_DATA ||
               which_type == TxoutType::MULTISIG);
    }
    if (which_type == TxoutType::NONSTANDARD ||
        which_type == TxoutType::NULL_DATA ||
        which_type == TxoutType::MULTISIG) {
        assert(!extract_destination_ret);
    }

    const FlatSigningProvider signing_provider;
    (void)InferDescriptor(script, signing_provider);
    (void)IsSegWitOutput(signing_provider, script);

    (void)RecursiveDynamicUsage(script);

    std::vector<std::vector<unsigned char>> solutions;
    (void)Solver(script, solutions);

    {
        // Exercises ScriptCompression::Unser path.
        // First, this always runs inside a Coin record, so more data follows the script
        // in the stream and it must not be consumed.
        //
        // A compressed script starts with its type: one of six special encodings (0 to 5) with
        // a fixed-size payload, or a raw script length plus nSpecialScripts. A special script
        // must decode to a non-empty script or throw, never turn into a different script.
        // The rest of scripts either decode raw or as OP_RETURN for ones exceeding MAX_SCRIPT_SIZE.
        unsigned int type;
        unsigned int payload_size;
        if (fuzzed_data_provider.ConsumeBool()) {
            type = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, ScriptCompression::nSpecialScripts - 1);
            payload_size = GetSpecialScriptSize(type);
        } else {
            // For a script with length > MAX_SCRIPT_SIZE the script is replaced for an OP_RETURN
            payload_size = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, MAX_SCRIPT_SIZE + 16);
            type = payload_size + ScriptCompression::nSpecialScripts;
        }

        const std::vector<uint8_t> payload = ConsumeFixedLengthByteVector(fuzzed_data_provider, payload_size);
        const std::vector<std::byte> trailing = ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider);
        // Decodable unless it is a pubkey record (type 4 or 5) whose X coordinate is not a valid compressed pubkey.
        bool decodable{true};
        if (type == 4 || type == 5) {
            std::vector<unsigned char> pubkey{static_cast<unsigned char>(type - 2)};
            pubkey.insert(pubkey.end(), payload.begin(), payload.end());
            decodable = CPubKey{pubkey}.IsFullyValid();
        }

        DataStream stream = DataStream{} << VARINT(type) << std::span{payload} << std::span{trailing};
        try {
            CScript decompressed_script;
            stream >> Using<ScriptCompression>(decompressed_script);
            assert(decodable);
            if (type < ScriptCompression::nSpecialScripts) {
                assert(!decompressed_script.empty()); // future: check each type individually
            } else if (payload_size > MAX_SCRIPT_SIZE) {
                assert(decompressed_script == CScript() << OP_RETURN);
            } else {
                assert(decompressed_script == CScript(payload.begin(), payload.end()));
            }
            assert(std::ranges::equal(stream, trailing)); // only the script was consumed
        } catch (const std::ios_base::failure&) {
            assert(!decodable);
        }
    }

    const std::optional<CScript> other_script = ConsumeDeserializable<CScript>(fuzzed_data_provider);
    if (other_script) {
        {
            CScript script_mut{script};
            (void)FindAndDelete(script_mut, *other_script);
        }
        const std::vector<std::string> random_string_vector = ConsumeRandomLengthStringVector(fuzzed_data_provider);
        const auto flags_rand{fuzzed_data_provider.ConsumeIntegral<script_verify_flags::value_type>()};
        const auto flags = script_verify_flags::from_int(flags_rand) | SCRIPT_VERIFY_P2SH;
        {
            CScriptWitness wit;
            for (const auto& s : random_string_vector) {
                wit.stack.emplace_back(s.begin(), s.end());
            }
            (void)CountWitnessSigOps(script, *other_script, wit, flags);
            wit.SetNull();
        }
    }

    (void)GetOpName(ConsumeOpcodeType(fuzzed_data_provider));
    (void)ScriptErrorString(static_cast<ScriptError>(fuzzed_data_provider.ConsumeIntegralInRange<int>(0, SCRIPT_ERR_ERROR_COUNT)));

    {
        const std::vector<uint8_t> bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        CScript append_script{bytes.begin(), bytes.end()};
        append_script << fuzzed_data_provider.ConsumeIntegral<int64_t>();
        append_script << ConsumeOpcodeType(fuzzed_data_provider);
        append_script << CScriptNum{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
        append_script << ConsumeRandomLengthByteVector(fuzzed_data_provider);
    }

    {
        const CTxDestination tx_destination_1{
            fuzzed_data_provider.ConsumeBool() ?
                DecodeDestination(fuzzed_data_provider.ConsumeRandomLengthString()) :
                ConsumeTxDestination(fuzzed_data_provider)};
        const CTxDestination tx_destination_2{ConsumeTxDestination(fuzzed_data_provider)};
        const std::string encoded_dest{EncodeDestination(tx_destination_1)};
        const UniValue json_dest{DescribeAddress(tx_destination_1)};
        (void)GetKeyForDestination(/*store=*/{}, tx_destination_1);
        const CScript dest{GetScriptForDestination(tx_destination_1)};
        const bool valid{IsValidDestination(tx_destination_1)};

        if (!std::get_if<PubKeyDestination>(&tx_destination_1)) {
            // Only try to round trip non-pubkey destinations since PubKeyDestination has no encoding
            Assert(dest.empty() != valid);
            Assert(tx_destination_1 == DecodeDestination(encoded_dest));
            Assert(valid == IsValidDestinationString(encoded_dest));
        }

        (void)(tx_destination_1 < tx_destination_2);
        if (tx_destination_1 == tx_destination_2) {
            Assert(encoded_dest == EncodeDestination(tx_destination_2));
            Assert(json_dest.write() == DescribeAddress(tx_destination_2).write());
            Assert(dest == GetScriptForDestination(tx_destination_2));
        }
    }
}
