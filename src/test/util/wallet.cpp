// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/wallet.h>

#include <key_io.h>
#include <outputtype.h>
#include <script/standard.h>
#ifdef ENABLE_WALLET
#include <util/translation.h>
#include <wallet/wallet.h>
#endif

const std::string ADDRESS_BCRT1_UNSPENDABLE = "bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq3xueyj";

#ifdef ENABLE_WALLET
std::string getnewaddress(CWallet& w)
{
    constexpr auto output_type = OutputType::BECH32;
    CTxDestination dest;
    bilingual_str error;
    if (!w.GetNewDestination(output_type, "", dest, error)) assert(false);

    return EncodeDestination(dest);
}

#endif // ENABLE_WALLET
