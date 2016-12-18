// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/pureheader.h"

#include "chainparams.h"
#include "hash.h"
#include "utilstrencodings.h"
#include "versionbits.h"
int32_t CBlockVersion::GetBaseVersion() const
{
	// base version is everything above the top_mask (everything below in bits is auxpow info)
    return nVersion & VERSIONBITS_TOP_MASK;
}
int32_t CBlockVersion::GetAuxVersion() const
{
    return nVersion & ~VERSIONBITS_TOP_MASK;
}
void CBlockVersion::SetAuxpow(bool auxpow)
{
    if (auxpow)
	{
        nVersion |= VERSION_AUXPOW;
	}
    else
	{
        nVersion &= ~VERSION_AUXPOW;
	}
}
// start from scratch and apply the base -> auxpow bit -> new chainid to the version flag
void CBlockVersion::SetChainId(int32_t chainId)
{
	// create new chainid mask
    int32_t chaindIdVersion = chainId * VERSION_CHAIN_START;
	// was auxpow flag already set?
	bool isAuxpow = IsAuxpow();
	// start with the base version
	nVersion = GetBaseVersion();
	// apply auxpow flag if it was set
	SetAuxpow(isAuxpow);
	// add in the new chain id mask
	nVersion |= chaindIdVersion;
}
void CBlockVersion::SetBaseVersion(int32_t nBaseVersion)
{
	// get the current aux version (auxpow bit and chain id)
	int32_t auxVersion = GetAuxVersion();
	// set new base version along with old auxpow information
	nVersion = nBaseVersion | auxVersion;
}

uint256 CPureBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}