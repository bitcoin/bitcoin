// Copyright (c) 2021-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainlock/clsig.h>

#include <tinyformat.h>

#include <string_view>

namespace chainlock {
static constexpr std::string_view CLSIG_REQUESTID_PREFIX{"clsig"};

ChainLockSig::ChainLockSig() = default;
ChainLockSig::~ChainLockSig() = default;

ChainLockSig::ChainLockSig(int32_t nHeight, const uint256& blockHash, const CBLSSignature& sig) :
    nHeight{nHeight},
    blockHash{blockHash},
    sig{sig}
{
}

std::string ChainLockSig::ToString() const
{
    return strprintf("ChainLockSig(nHeight=%d, blockHash=%s)", nHeight, blockHash.ToString());
}

uint256 GenSigRequestId(const int32_t nHeight)
{
    return ::SerializeHash(std::make_pair(CLSIG_REQUESTID_PREFIX, nHeight));
}
} // namespace chainlock
