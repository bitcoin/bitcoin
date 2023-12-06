// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key_io.h>
#include <rpc/util.h>
#include <script/standard.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void initialize_key_io()
{
    ECC_Start();
    SelectParams(CBaseChainParams::MAIN);
}

FUZZ_TARGET_INIT(key_io, initialize_key_io)
{
    const std::string random_string(buffer.begin(), buffer.end());

    const CKey key = DecodeSecret(random_string);
    if (key.IsValid()) {
        assert(key == DecodeSecret(EncodeSecret(key)));
    }

    const CExtKey ext_key = DecodeExtKey(random_string);
    if (ext_key.key.size() == 32) {
        assert(ext_key == DecodeExtKey(EncodeExtKey(ext_key)));
    }

    const CExtPubKey ext_pub_key = DecodeExtPubKey(random_string);
    if (ext_pub_key.pubkey.size() == CPubKey::COMPRESSED_SIZE) {
        assert(ext_pub_key == DecodeExtPubKey(EncodeExtPubKey(ext_pub_key)));
    }

    const CTxDestination tx_destination = DecodeDestination(random_string);
    (void)DescribeAddress(tx_destination);
    (void)GetKeyForDestination(/* store */ {}, tx_destination);
    (void)GetScriptForDestination(tx_destination);
    (void)IsValidDestination(tx_destination);

    (void)IsValidDestinationString(random_string);
}
