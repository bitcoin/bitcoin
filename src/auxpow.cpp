// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 Vince Durham
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2019 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include <auxpow.h>

#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <primitives/block.h>
#include <script/script.h>
#include <util/strencodings.h>

#include <algorithm>
#include <logging.h>
namespace
{

/**
 * Decodes a 32-bit little endian integer from raw bytes.
 */
uint32_t
DecodeLE32 (const unsigned char* bytes)
{
  uint32_t res = 0;
  for (int i = 0; i < 4; ++i)
    {
      res <<= 8;
      res |= bytes[3 - i];
    }
  return res;
}

} // anonymous namespace

bool
CAuxPow::check (const uint256& hashAuxBlock, int nChainId,
                const Consensus::Params& params) const
{
    if (params.fStrictChainId) {
        const int32_t &nChainIDParent = parentBlock.GetChainId();
        if(nChainIDParent > 0) {
            if(nChainIDParent == params.nAuxpowChainId)
                return error("Aux POW parent has our chain ID");
        } else {
            const int32_t &nOldChainIDParent = parentBlock.GetOldChainId();
            if(nOldChainIDParent == params.nAuxpowOldChainId)
                return error("Aux POW parent has our old chain ID");
        }
    }

    if (vChainMerkleBranch.size() > 30)
        return error("Aux POW chain merkle branch too long");

    // Check that the chain merkle root is in the coinbase
    const uint256 &nRootHash
      = CheckMerkleBranch (hashAuxBlock, vChainMerkleBranch, nChainIndex);
    valtype vchRootHash(nRootHash.begin (), nRootHash.end ());
    std::reverse (vchRootHash.begin (), vchRootHash.end ()); // correct endian

    // Check that we are in the parent block merkle tree
    if (CheckMerkleBranch(coinbaseTx->GetHash(), vMerkleBranch, 0)
          != parentBlock.hashMerkleRoot)
        return error("Aux POW merkle root incorrect");

    // Check that there is at least one input.
    if (coinbaseTx->vin.empty())
        return error("Aux POW coinbase has no inputs");

    const CScript &script = coinbaseTx->vin[0].scriptSig;

    // Check that the same work is not submitted twice to our chain.
    //

    const unsigned char* const mmHeaderBegin = pchMergedMiningHeader;
    const unsigned char* const mmHeaderEnd
        = mmHeaderBegin + sizeof (pchMergedMiningHeader);
    CScript::const_iterator pcHead =
        std::search(script.begin(), script.end(), mmHeaderBegin, mmHeaderEnd);

    CScript::const_iterator pc =
        std::search(script.begin(), script.end(), vchRootHash.begin(), vchRootHash.end());

    if (pc == script.end())
        return error("Aux POW missing chain merkle root in parent coinbase");

    if (pcHead != script.end())
    {
        // Enforce only one chain merkle root by checking that a single instance of the merged
        // mining header exists just before.
        if (script.end() != std::search(pcHead + 1, script.end(),
                                        mmHeaderBegin, mmHeaderEnd))
            return error("Multiple merged mining headers in coinbase");
        if (pcHead + sizeof(pchMergedMiningHeader) != pc)
            return error("Merged mining header is not just before chain merkle root");
    }
    else
    {
        return error("Missing auxpow header");
    }


    // Ensure we are at a deterministic point in the merkle leaves by hashing
    // a nonce and our chain ID and comparing to the index.
    pc += vchRootHash.size();
    if (script.end() - pc < 8)
        return error("Aux POW missing chain merkle tree size and nonce in parent coinbase");

    const uint32_t &nSize = DecodeLE32 (&pc[0]);
    const unsigned &merkleHeight = vChainMerkleBranch.size ();
    if (nSize != (1u << merkleHeight))
        return error("Aux POW merkle branch size does not match parent coinbase");

    const uint32_t &nNonce = DecodeLE32 (&pc[4]);
    if (nChainIndex != getExpectedIndex (nNonce, nChainId, merkleHeight))
        return error("Aux POW wrong index");

    return true;
}

int
CAuxPow::getExpectedIndex (const uint32_t &nNonce, const int &nChainId,
                           const unsigned &h)
{
  // Choose a pseudo-random slot in the chain merkle tree
  // but have it be fixed for a size/nonce/chain combination.
  //
  // This prevents the same work from being used twice for the
  // same chain while reducing the chance that two chains clash
  // for the same slot.

  /* Note that h is always <= 30 (enforced by the maximum allowed chain
     merkle branch length), so that 32 bits are enough for the result.  */

  const uint32_t mod = (1u << h);

  uint64_t rand = nNonce;
  rand = rand * 1103515245 + 12345;
  rand %= mod;
  rand += nChainId;
  rand = rand * 1103515245 + 12345;
  rand %= mod;

  return rand;
}

uint256
CAuxPow::CheckMerkleBranch (uint256 hash,
                            const std::vector<uint256>& vMerkleBranch,
                            int nIndex)
{
  if (nIndex == -1)
    return uint256 ();
  for (std::vector<uint256>::const_iterator it(vMerkleBranch.begin ());
       it != vMerkleBranch.end (); ++it)
  {
    if (nIndex & 1)
      hash = Hash (*it, hash);
    else
      hash = Hash (hash, *it);
    nIndex >>= 1;
  }
  return hash;
}

std::unique_ptr<CAuxPow>
CAuxPow::createAuxPow (const CPureBlockHeader& header)
{
  assert (header.IsAuxpow ());

  /* Build a minimal coinbase script input for merge-mining.  */
  const uint256 blockHash = header.GetHash ();
  valtype inputData(blockHash.begin (), blockHash.end ());
  std::reverse (inputData.begin (), inputData.end ());
  inputData.push_back (1);
  inputData.insert (inputData.end (), 7, 0);

  /* Fake a parent-block coinbase with just the required input
     script and no outputs.  */
  CMutableTransaction coinbase;
  coinbase.vin.resize (1);
  coinbase.vin[0].prevout.SetNull ();
  coinbase.vin[0].scriptSig = (CScript () << inputData);
  assert (coinbase.vout.empty ());
  CTransactionRef coinbaseRef = MakeTransactionRef (coinbase);

  /* Build a fake parent block with the coinbase.  */
  CBlock parent;
  parent.nVersion = 1;
  parent.vtx.resize (1);
  parent.vtx[0] = coinbaseRef;
  parent.hashMerkleRoot = BlockMerkleRoot (parent);

  /* Construct the auxpow object.  */
  std::unique_ptr<CAuxPow> auxpow(new CAuxPow (std::move (coinbaseRef)));
  assert (auxpow->vMerkleBranch.empty ());
  assert (auxpow->vChainMerkleBranch.empty ());
  auxpow->nChainIndex = 0;
  auxpow->parentBlock = parent;

  return auxpow;
}

CPureBlockHeader&
CAuxPow::initAuxPow (CBlockHeader& header)
{
  /* Set auxpow flag right now, since we take the block hash below when creating
     the minimal auxpow for header.  */
  header.SetAuxpowVersion(true);

  std::unique_ptr<CAuxPow> apow = createAuxPow (header);
  CPureBlockHeader& result = apow->parentBlock;
  header.SetAuxpow (std::move (apow));

  return result;
}