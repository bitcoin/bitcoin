// Copyright (c) 2018-2020 Daniel Kraft
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/auxpow_miner.h>

#include <arith_uint256.h>
#include <auxpow.h>
#include <chainparams.h>
#include <net.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <validation.h>

#include <cassert>
#include <util/check.h>
namespace
{

void auxMiningCheck(const JSONRPCRequest& request)
{
  NodeContext& node = EnsureAnyNodeContext (request.context);
  if (!node.connman)
    throw JSONRPCError (RPC_CLIENT_P2P_DISABLED,
                        "Error: Peer-to-peer functionality missing or"
                        " disabled");

  if (node.connman->GetNodeCount (ConnectionDirection::Both) == 0
        && !Params ().MineBlocksOnDemand ())
    throw JSONRPCError (RPC_CLIENT_NOT_CONNECTED,
                        "Syscoin is not connected!");

  if (::ChainstateActive ().IsInitialBlockDownload ()
        && !Params ().MineBlocksOnDemand ())
    throw JSONRPCError (RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                        "Syscoin is downloading blocks...");

  /* This should never fail, since the chain is already
     past the point of merge-mining start.  Check nevertheless.  */
  {
    LOCK (cs_main);
    const auto auxpowStart = Params ().GetConsensus ().nAuxpowStartHeight;
    if (::ChainActive ().Height () + 1 < auxpowStart)
      throw std::runtime_error ("mining auxblock method is not yet available");
  }
}

}  // anonymous namespace

const CBlock*
AuxpowMiner::getCurrentBlock (const CTxMemPool& mempool,
                              const CScript& scriptPubKey, uint256& target)
{
  AssertLockHeld(cs);
  const CBlock* pblockCur = nullptr;

  {
    LOCK (cs_main);
    CScriptID scriptID (scriptPubKey);
    auto iter = curBlocks.find(scriptID);
    if (iter != curBlocks.end())
      pblockCur = iter->second;

    if (pblockCur == nullptr
        || pindexPrev != ::ChainActive ().Tip ()
        || (mempool.GetTransactionsUpdated () != txUpdatedLast
            && GetTime () - startTime > 60))
      {
        if (pindexPrev != ::ChainActive ().Tip ())
          {
            /* Clear old blocks since they're obsolete now.  */
            blocks.clear ();
            templates.clear ();
            curBlocks.clear ();
          }

        /* Create new block with nonce = 0 and extraNonce = 1.  */
        std::unique_ptr<CBlockTemplate> newBlock
            = BlockAssembler (::ChainstateActive(), mempool, Params ()).CreateNewBlock (scriptPubKey);
        if (newBlock == nullptr)
          throw JSONRPCError (RPC_OUT_OF_MEMORY, "out of memory");

        /* Update state only when CreateNewBlock succeeded.  */
        txUpdatedLast = mempool.GetTransactionsUpdated ();
        pindexPrev = ::ChainActive ().Tip ();
        startTime = GetTime ();

        /* Finalise it by setting the version and building the merkle root.  */
        IncrementExtraNonce (&newBlock->block, pindexPrev, extraNonce);
        newBlock->block.SetAuxpowVersion (true);

        /* Save in our map of constructed blocks.  */
        pblockCur = &newBlock->block;
        curBlocks.try_emplace(scriptID, pblockCur);
        blocks[pblockCur->GetHash ()] = pblockCur;
        templates.push_back (std::move (newBlock));
      }
  }

  /* At this point, pblockCur is always initialised:  If we make it here
     without creating a new block above, it means that, in particular,
     pindexPrev == ::ChainActive ().Tip().  But for that to happen, we must
     already have created a pblockCur in a previous call, as pindexPrev is
     initialised only when pblockCur is.  */
  CHECK_NONFATAL(pblockCur);

  arith_uint256 arithTarget;
  bool fNegative, fOverflow;
  arithTarget.SetCompact (pblockCur->nBits, &fNegative, &fOverflow);
  if (fNegative || fOverflow || arithTarget == 0)
    throw std::runtime_error ("invalid difficulty bits in block");
  target = ArithToUint256 (arithTarget);

  return pblockCur;
}

const CBlock*
AuxpowMiner::lookupSavedBlock (const std::string& hashHex) const
{
  AssertLockHeld(cs);

  uint256 hash;
  hash.SetHex (hashHex);

  const auto iter = blocks.find (hash);
  if (iter == blocks.end ())
    throw JSONRPCError (RPC_INVALID_PARAMETER, "block hash unknown");

  return iter->second;
}

UniValue
AuxpowMiner::createAuxBlock (const JSONRPCRequest& request,
                             const CScript& scriptPubKey)
{
  auxMiningCheck (request);
  LOCK (cs);

  const auto& mempool = EnsureAnyMemPool (request.context);

  uint256 target;
  const CBlock* pblock = getCurrentBlock (mempool, scriptPubKey, target);

  UniValue result(UniValue::VOBJ);
  result.pushKV ("hash", pblock->GetHash ().GetHex ());
  result.pushKV ("chainid", pblock->GetChainId ());
  result.pushKV ("previousblockhash", pblock->hashPrevBlock.GetHex ());
  result.pushKV ("coinbasevalue",
                 static_cast<int64_t> (pblock->vtx[0]->vout[0].nValue));
  result.pushKV ("bits", strprintf ("%08x", pblock->nBits));
  result.pushKV ("height", static_cast<int64_t> (pindexPrev->nHeight + 1));
  result.pushKV ("_target", HexStr (target));

  return result;
}

bool
AuxpowMiner::submitAuxBlock (const JSONRPCRequest& request,
                             const std::string& hashHex,
                             const std::string& auxpowHex) const
{
  auxMiningCheck (request);
  auto& chainman = EnsureAnyChainman (request.context);

  std::shared_ptr<CBlock> shared_block;
  {
    LOCK (cs);
    const CBlock* pblock = lookupSavedBlock (hashHex);
    shared_block = std::make_shared<CBlock> (*pblock);
  }

  const std::vector<unsigned char> vchAuxPow = ParseHex (auxpowHex);
  CDataStream ss(vchAuxPow, SER_GETHASH, PROTOCOL_VERSION);
  std::unique_ptr<CAuxPow> pow(new CAuxPow ());
  ss >> *pow;
  shared_block->SetAuxpow (std::move (pow));
  CHECK_NONFATAL(shared_block->GetHash ().GetHex () == hashHex);

  return chainman.ProcessNewBlock (Params (), shared_block, true, nullptr);
}

AuxpowMiner&
AuxpowMiner::get ()
{
  static AuxpowMiner* instance = nullptr;
  static RecursiveMutex lock;

  LOCK (lock);
  if (instance == nullptr)
    instance = new AuxpowMiner ();

  return *instance;
}