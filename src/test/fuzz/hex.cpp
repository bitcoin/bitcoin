// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <primitives/block.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <test/fuzz/fuzz.h>
#include <uint256.h>
#include <univalue.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(hex)
{
    const std::string random_hex_string(buffer.begin(), buffer.end());
    const std::vector<unsigned char> data = ParseHex(random_hex_string);
    const std::vector<std::byte> bytes{ParseHex<std::byte>(random_hex_string)};
    assert(AsBytes(Span{data}) == Span{bytes});
    const std::string hex_data = HexStr(data);
    if (IsHex(random_hex_string)) {
        assert(ToLower(random_hex_string) == hex_data);
    }
    (void)IsHexNumber(random_hex_string);
    (void)uint256::FromHex(random_hex_string);
    (void)uint256S(random_hex_string);
    try {
        (void)HexToPubKey(random_hex_string);
    } catch (const UniValue&) {
    }
    CBlockHeader block_header;
    (void)DecodeHexBlockHeader(block_header, random_hex_string);
    CBlock block;
    (void)DecodeHexBlk(block, random_hex_string);
}
