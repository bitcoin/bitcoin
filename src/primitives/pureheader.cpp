// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Syscoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/pureheader.h>
#include <hash.h>
uint256 CPureBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

void CPureBlockHeader::SetBaseVersion(int32_t nBaseVersion, const int32_t &nChainId)
{
    const int32_t withoutTopMask = nBaseVersion & ~VERSIONAUXPOW_TOP_MASK;
    assert(withoutTopMask >= 0 && withoutTopMask < VERSION_CHAIN_START);
    assert(!IsAuxpow());
    nVersion = nBaseVersion | (nChainId << VERSION_START_BIT);
}

void CPureBlockHeader::SetOldBaseVersion(int32_t nBaseVersion, const int32_t &nChainId)
{
    assert(nBaseVersion >= 1 && nBaseVersion < VERSION_CHAIN_START);
    assert(!IsAuxpow());
    nVersion = nBaseVersion | (nChainId << VERSION_START_BIT);
}

int32_t CPureBlockHeader::GetBaseVersion(const int32_t &ver) 
{
    // remove AuxPow flag and Chain ID
    return (ver & ~VERSION_AUXPOW) & ~VERSION_AUXPOW_CHAINID_SHIFTED;
}

int32_t CPureBlockHeader::GetOldBaseVersion(const int32_t &ver)
{
    
    return (ver & ~VERSION_AUXPOW) & ~VERSION_OLD_AUXPOW_CHAINID_SHIFTED;
}

int32_t CPureBlockHeader::GetChainId(const int32_t &ver)
{
    // if auxpow is set then mask with Chain ID and shift back VERSION_START_BIT to get the real value
    if((ver & VERSION_AUXPOW) > 0)
        return (ver & MASK_AUXPOW_CHAINID_SHIFTED) >> VERSION_START_BIT;
    else
        return 0;
}

int32_t CPureBlockHeader::GetOldChainId(const int32_t &ver) 
{
    if((ver & VERSION_AUXPOW) > 0)
        return (ver & MASK_OLD_AUXPOW_CHAINID_SHIFTED) >> VERSION_START_BIT;
    else
        return 0;
}
