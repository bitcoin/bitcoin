// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "WalletOutput.h"
#include "tinyformat.h"
#include "WalletTx.h"
#include "utilmoneystr.h"

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]",
        tx->GetHash().ToHexString(), i, nDepth, FormatMoney(tx->tx->vout[i].nValue));
}
