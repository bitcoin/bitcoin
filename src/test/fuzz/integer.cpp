// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <compressor.h>
#include <consensus/merkle.h>
#include <core_io.h>
#include <crypto/common.h>
#include <crypto/siphash.h>
#include <key_io.h>
#include <memusage.h>
#include <netbase.h>
#include <policy/settings.h>
#include <pow.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <serialize.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/time.h>

#include <cassert>
#include <limits>
#include <vector>

void initialize()
{
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    if (buffer.size() < sizeof(uint256) + sizeof(uint160)) {
        return;
    }
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const uint256 u256(fuzzed_data_provider.ConsumeBytes<unsigned char>(sizeof(uint256)));
    const uint160 u160(fuzzed_data_provider.ConsumeBytes<unsigned char>(sizeof(uint160)));
    const uint64_t u64 = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
    const int64_t i64 = fuzzed_data_provider.ConsumeIntegral<int64_t>();
    const uint32_t u32 = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    const int32_t i32 = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    const uint16_t u16 = fuzzed_data_provider.ConsumeIntegral<uint16_t>();
    const int16_t i16 = fuzzed_data_provider.ConsumeIntegral<int16_t>();
    const uint8_t u8 = fuzzed_data_provider.ConsumeIntegral<uint8_t>();
    const int8_t i8 = fuzzed_data_provider.ConsumeIntegral<int8_t>();
    // We cannot assume a specific value of std::is_signed<char>::value:
    // ConsumeIntegral<char>() instead of casting from {u,}int8_t.
    const char ch = fuzzed_data_provider.ConsumeIntegral<char>();

    const Consensus::Params& consensus_params = Params().GetConsensus();
    (void)CheckProofOfWork(u256, u32, consensus_params);
    (void)CompressAmount(u64);
    static const uint256 u256_min(uint256S("0000000000000000000000000000000000000000000000000000000000000000"));
    static const uint256 u256_max(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    const std::vector<uint256> v256{u256, u256_min, u256_max};
    (void)ComputeMerkleRoot(v256);
    (void)CountBits(u64);
    (void)DecompressAmount(u64);
    (void)FormatISO8601Date(i64);
    (void)FormatISO8601DateTime(i64);
    (void)GetSizeOfCompactSize(u64);
    (void)GetSpecialScriptSize(u32);
    // (void)GetVirtualTransactionSize(i64, i64); // function defined only for a subset of int64_t inputs
    // (void)GetVirtualTransactionSize(i64, i64, u32); // function defined only for a subset of int64_t/uint32_t inputs
    (void)HexDigit(ch);
    (void)i64tostr(i64);
    (void)IsDigit(ch);
    (void)IsSpace(ch);
    (void)IsSwitchChar(ch);
    (void)itostr(i32);
    (void)memusage::DynamicUsage(ch);
    (void)memusage::DynamicUsage(i16);
    (void)memusage::DynamicUsage(i32);
    (void)memusage::DynamicUsage(i64);
    (void)memusage::DynamicUsage(i8);
    (void)memusage::DynamicUsage(u16);
    (void)memusage::DynamicUsage(u32);
    (void)memusage::DynamicUsage(u64);
    (void)memusage::DynamicUsage(u8);
    const unsigned char uch = static_cast<unsigned char>(u8);
    (void)memusage::DynamicUsage(uch);
    (void)MillisToTimeval(i64);
    const double d = ser_uint64_to_double(u64);
    assert(ser_double_to_uint64(d) == u64);
    const float f = ser_uint32_to_float(u32);
    assert(ser_float_to_uint32(f) == u32);
    (void)SighashToStr(uch);
    (void)SipHashUint256(u64, u64, u256);
    (void)SipHashUint256Extra(u64, u64, u256, u32);
    (void)ToLower(ch);

    const arith_uint256 au256 = UintToArith256(u256);
    assert(ArithToUint256(au256) == u256);
    assert(uint256S(au256.GetHex()) == u256);
    (void)au256.bits();
    (void)au256.GetCompact(/* fNegative= */ false);
    (void)au256.GetCompact(/* fNegative= */ true);
    (void)au256.getdouble();
    (void)au256.GetHex();
    (void)au256.GetLow64();
    (void)au256.size();
    (void)au256.ToString();

    const CKeyID key_id{u160};
    const CScriptID script_id{u160};
    // CTxDestination = CNoDestination ∪ PKHash ∪ ScriptHash ∪ WitnessV0ScriptHash ∪ WitnessV0KeyHash ∪ WitnessUnknown
    const PKHash pk_hash{u160};
    const ScriptHash script_hash{u160};
    const WitnessV0KeyHash witness_v0_key_hash{u160};
    const WitnessV0ScriptHash witness_v0_script_hash{u256};
    const std::vector<CTxDestination> destinations{pk_hash, script_hash, witness_v0_key_hash, witness_v0_script_hash};
    const SigningProvider store;
    for (const CTxDestination& destination : destinations) {
        (void)DescribeAddress(destination);
        (void)EncodeDestination(destination);
        (void)GetKeyForDestination(store, destination);
        (void)GetScriptForDestination(destination);
        (void)IsValidDestination(destination);
    }
}
