// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Syscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/pureheader.h>

#include <hash.h>
#include <util/strencodings.h>
#include <versionbits.h>
uint256 CPureBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

void CPureBlockHeader::SetBaseVersion(int32_t nBaseVersion, const int32_t &nChainId)
{
    const int32_t topMask = nBaseVersion & VERSIONAUXPOW_TOP_MASK;
    if(topMask > 0) {
        nBaseVersion &= ~topMask;
    }
    assert(nBaseVersion >= 0 && nBaseVersion < VERSION_CHAIN_START);
    assert(!IsAuxpow());
    nVersion = nBaseVersion | (nChainId * VERSION_CHAIN_START);
    if(topMask > 0) {
        nVersion |= topMask;
    }
}

int32_t CPureBlockHeader::GetBaseVersion(int32_t ver)
{
    const int32_t& nVersionWithoutTopBits = ver & ~VERSIONAUXPOW_TOP_MASK;
    if((nVersionWithoutTopBits / VERSION_CHAIN_START) > 0) {
        // TODO: remove magic and replace with params
        ver -= (8 * VERSION_CHAIN_START);
        ver &= ~VERSION_AUXPOW;
    }
    return ver;
}

int32_t CPureBlockHeader::GetChainId() const
{
    const int32_t& nVersionWithoutTopBits = nVersion & ~VERSIONAUXPOW_TOP_MASK;
    return nVersionWithoutTopBits / VERSION_CHAIN_START;
}
