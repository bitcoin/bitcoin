// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_PRIMITIVES_BLOCK_H
#define SYSCOIN_PRIMITIVES_BLOCK_H

#include "primitives/transaction.h"
#include "primitives/pureheader.h"
#include "serialize.h"
#include "uint256.h"
// SYSCOIN for auxpow
#include "auxpow.h"
#include <boost/shared_ptr.hpp>
/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
 // SYSCOIN depends on pureblockheader for auxpow
class CBlockHeader : public CPureBlockHeader
{
public:
	// auxpow (if this is a merge-minded block)
	boost::shared_ptr<CAuxPow> auxpow;
    CBlockHeader()
    {
        SetNull();
    }

	ADD_SERIALIZE_METHODS;

	template <typename Stream, typename Operation>
	inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(*(CPureBlockHeader*)this);

		if (this->IsAuxpow())
		{
			if (ser_action.ForRead())
				auxpow.reset(new CAuxPow());
			assert(auxpow);
			READWRITE(*auxpow);
		}
		else if (ser_action.ForRead())
			auxpow.reset();
	}

	void SetNull()
	{
		CPureBlockHeader::SetNull();
		auxpow.reset();
	}

	/**
	* Set the block's auxpow (or unset it).  This takes care of updating
	* the version accordingly.
	* @param apow Pointer to the auxpow to use or NULL.
	*/
	void SetAuxpow(CAuxPow* apow);
};


class CBlock : public CBlockHeader
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;

    // memory only
    mutable CTxOut txoutMasternode; // masternode payment
    mutable std::vector<CTxOut> voutSuperblock; // superblock payment
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        txoutMasternode = CTxOut();
        voutSuperblock.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
		// SYSCOIN include auxpow in blockheader
		block.auxpow = auxpow;
        return block;
    }

    std::string ToString() const;
};


/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    CBlockLocator(const std::vector<uint256>& vHaveIn)
    {
        vHave = vHaveIn;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // SYSCOIN_PRIMITIVES_BLOCK_H
