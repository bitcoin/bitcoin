// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcointalkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <momentum.h>
#include <crypto/common.h>
#include <axiom.h>
#include <groestl.h>
#include <util/strencodings.h>

uint256 CBlockHeader::GetSHA256() const
{
    return SerializeHash(*this);
}

uint256 CBlockHeader::GetMemHash() const
{
    return AxiomHash(BEGIN(nVersion), END(nNonce));
}
#ifdef ENABLE_MOMENTUM_HASH_ALGO
uint256 CBlockHeader::GetMomentumHash() const
{
    return Hash(BEGIN(nVersion), END(nBirthdayB));
}

uint256 CBlockHeader::GetMidHash() const
{
    return Hash(BEGIN(nVersion), END(nNonce));
}

uint256 CBlockHeader::GetVerifiedHash() const
{
 
 	uint256 midHash = GetMidHash();
 		    	
	uint256 r = Hash(BEGIN(nVersion), END(nBirthdayB));

 	if(!momentum_verify( midHash, nBirthdayA, nBirthdayB)){
 		return uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeeee");
 	}
   
     return r;
}
 
uint256 CBlockHeader::CalculateBestBirthdayHash() {
 				
	uint256 midHash = GetMidHash();		
	std::vector< std::pair<uint32_t,uint32_t> > results =momentum_search( midHash );
	uint32_t candidateBirthdayA=0;
	uint32_t candidateBirthdayB=0;
	uint256 smallestHashSoFar = uint256S("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffdddd");
	for (unsigned i=0; i < results.size(); i++) {
		nBirthdayA = results[i].first;
		nBirthdayB = results[i].second;
		uint256 fullHash = Hash(BEGIN(nVersion), END(nBirthdayB));
		if(fullHash<smallestHashSoFar){		
			smallestHashSoFar=fullHash;
			candidateBirthdayA=results[i].first;
			candidateBirthdayB=results[i].second;
		}
		nBirthdayA = candidateBirthdayA;
		nBirthdayB = candidateBirthdayB;
		//std::cout << "candidateBirthdayA = " << candidateBirthdayA<< "\n";
		//std::cout << "candidateBirthdayB = " << candidateBirthdayB<< "\n";
	} 		
	return GetHash();
}
#endif
uint256 CBlockHeader::GetGroestlHash() const
{
	CGroestlHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
	ss << *this;
	return ss.GetHash();
}

uint256 CBlockHeader::GetHash() const
{
#ifdef ENABLE_SHA_HASH_ALGO
    return GetSHA256();
#endif
#ifdef ENABLE_MEM_HASH_ALGO 
    return GetMemHash();
#endif
#ifdef ENABLE_MOMENTUM_HASH_ALGO
    return GetMomentumHash();
#endif
#ifdef ENABLE_GROESTL_HASH_ALGO
    return GetGroestlHash();
#endif
}

#ifdef ENABLE_PROOF_OF_STAKE
uint256 CBlockHeader::GetHashWithoutSign() const
{
    return SerializeHash(*this, SER_GETHASH | SER_WITHOUT_SIGNATURE);
}
#endif

std::string CBlockHeader::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlockHeader(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u)\n",
        GetHash().ToString(), nVersion, hashPrevBlock.ToString(), hashMerkleRoot.ToString(), nTime, nBits, nNonce);
    return s.str();
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u)",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce);
#ifdef ENABLE_MOMENTUM_HASH_ALGO

        s << strprintf(" nBirthdayA=%u, nBirthdayB=%u ", nBirthdayA, nBirthdayB);
#endif
        s << strprintf(" vtx=%u, \n)", vtx.size());

#ifdef ENABLE_PROOF_OF_STAKE
        std::string type = "Proof-of-work";
        if (IsProofOfStake())
            type="Proof-of-stake";

        s << strprintf("( prevoutStake.ToString(), vchBlockSig=%s, proof=%s, prevoutStake=%s \n)", HexStr(vchBlockSig), type,prevoutStake.ToString());
#endif
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
