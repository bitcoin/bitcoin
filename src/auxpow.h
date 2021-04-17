// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2019 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_AUXPOW_H
#define SYSCOIN_AUXPOW_H

#include <consensus/params.h>
#include <primitives/pureheader.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

#include <cassert>
#include <memory>
#include <vector>

class CBlock;
class CBlockHeader;
class CBlockIndex;
class UniValue;
class ChainstateManager;
namespace auxpow_tests
{
class CAuxPowForTest;
}

/** Header for merge-mining data in the coinbase.  */
static const unsigned char pchMergedMiningHeader[] = { 0xfa, 0xbe, 'm', 'm' };

/**
 * Data for the merge-mining auxpow.  This uses a merkle tx (the parent block's
 * coinbase tx) and a second merkle branch to link the actual Syscoin block
 * header to the parent block header, which is mined to satisfy the PoW.
 */
class CAuxPow
{

private:

  /** The parent block's coinbase transaction.  */
  CTransactionRef coinbaseTx;

  /** The Merkle branch of the coinbase tx to the parent block's root.  */
  std::vector<uint256> vMerkleBranch;

  /** The merkle branch connecting the aux block to our coinbase.  */
  std::vector<uint256> vChainMerkleBranch;

  /** Merkle tree index of the aux block header in the coinbase.  */
  int nChainIndex;

  /** Parent block header (on which the real PoW is done).  */
  CPureBlockHeader parentBlock;

  /**
   * Check a merkle branch.  This used to be in CBlock, but was removed
   * upstream.  Thus include it here now.
   */
  static uint256 CheckMerkleBranch (uint256 hash,
                                    const std::vector<uint256>& vMerkleBranch,
                                    int nIndex);

  friend UniValue AuxpowToJSON(const CAuxPow& auxpow, ChainstateManager& chainstate);
  friend class auxpow_tests::CAuxPowForTest;

public:

  /* Prevent accidental conversion.  */
  inline explicit CAuxPow (CTransactionRef&& txIn)
    : coinbaseTx (std::move (txIn))
  {}

  CAuxPow () = default;

  CAuxPow (CAuxPow&&) = default;
  CAuxPow& operator= (CAuxPow&&) = default;

  CAuxPow (const CAuxPow&) = delete;
  void operator= (const CAuxPow&) = delete;


  SERIALIZE_METHODS(CAuxPow, obj)
  {
    /* The coinbase Merkle tx' hashBlock field is never actually verified
       or used in the code for an auxpow (and never was).  The parent block
       is known anyway directly, so this is also redundant.  By setting the
       value to zero (for serialising), we make sure that the format is
       backwards compatible but the data can be compressed.  */
    uint256 hashBlock;

    /* The index of the parent coinbase tx is always zero.  */
    int nIndex = 0;

    /* Data from the coinbase transaction READWRITE(obj.key);READWRITE(obj.key);as Merkle tx.  */
    READWRITE (obj.coinbaseTx, hashBlock, obj.vMerkleBranch, nIndex, obj.vChainMerkleBranch, obj.nChainIndex, obj.parentBlock);
  }

  /**
   * Check the auxpow, given the merge-mined block's hash and our chain ID.
   * Note that this does not verify the actual PoW on the parent block!  It
   * just confirms that all the merkle branches are valid.
   * @param hashAuxBlock Hash of the merge-mined block.
   * @param nChainId The auxpow chain ID of the block to check.
   * @param params Consensus parameters.
   * @return True if the auxpow is valid.
   */
  bool check (const uint256& hashAuxBlock, int nChainId,
              const Consensus::Params& params) const;

  /**
   * Returns the parent block hash.  This is used to validate the PoW.
   */
  inline uint256
  getParentBlockHash () const
  {
    return parentBlock.GetHash ();
  }

  /**
   * Calculate the expected index in the merkle tree.  This is also used
   * for the test-suite.
   * @param nNonce The coinbase's nonce value.
   * @param nChainId The chain ID.
   * @param h The merkle block height.
   * @return The expected index for the aux hash.
   */
  static int getExpectedIndex (const uint32_t &nNonce, const int &nChainId, const unsigned &h);

  /**
   * Constructs a minimal CAuxPow object for the given block header and
   * returns it.  The caller should make sure to set the auxpow flag on the
   * header already, since the block hash to which the auxpow commits depends
   * on that!
   */
  static std::unique_ptr<CAuxPow> createAuxPow (const CPureBlockHeader& header);

  /**
   * Initialises the auxpow of the given block header.  This builds a minimal
   * auxpow object like createAuxPow and sets it on the block header.  Returns
   * a reference to the parent header so it can be mined as a follow-up.
   */
  static CPureBlockHeader& initAuxPow (CBlockHeader& header);

};

#endif // SYSCOIN_AUXPOW_H