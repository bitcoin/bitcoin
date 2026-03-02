// Copyright (c) 2018-2020 Daniel Kraft
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/auxpow_miner.h>

#include <arith_uint256.h>
#include <auxpow.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <evo/specialtx.h>
#include <net.h>
#include <rpc/blockchain.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <validation.h>
#include <node/context.h>
#include <cassert>
#include <util/check.h>
#include <rpc/server_util.h>

namespace
{
void auxMiningCheck(const node::JSONRPCRequest& request)
{
  node::NodeContext& node = request.nodeContext? *request.nodeContext: EnsureAnyNodeContext (request.context);
  if (!node.connman)
    throw JSONRPCError (RPC_CLIENT_P2P_DISABLED,
                        "Error: Peer-to-peer functionality missing or"
                        " disabled");

  if (node.connman->GetNodeCount (ConnectionDirection::Both) == 0
        && !Params ().MineBlocksOnDemand ())
    throw JSONRPCError (RPC_CLIENT_NOT_CONNECTED,
                        "Syscoin is not connected!");

  if (node.chainman->IsInitialBlockDownload ()
        && !Params ().MineBlocksOnDemand ())
    throw JSONRPCError (RPC_CLIENT_IN_INITIAL_DOWNLOAD,
                        "Syscoin is downloading blocks...");

  /* This should never fail, since the chain is already
     past the point of merge-mining start.  Check nevertheless.  */
  {
    LOCK (cs_main);
    const auto auxpowStart = Params ().GetConsensus ().nAuxpowStartHeight;
    if (node.chainman->ActiveChain().Height () + 1 < auxpowStart)
      throw std::runtime_error ("mining auxblock method is not yet available");
  }
}
// SYSCOIN
bool IsWitnessCommitmentPush(const std::vector<unsigned char>& vch)
{
  return vch.size() >= 36 &&
         vch[0] == 0xaa &&
         vch[1] == 0x21 &&
         vch[2] == 0xa9 &&
         vch[3] == 0xed;
}

void InjectBTCPREVCommitment(CBlock& block, const uint256& btcPrevHash)
{
    if (block.vtx.empty() || !block.vtx[0]) {
      throw std::runtime_error("invalid block: missing coinbase");
    }

    // If already present, enforce match and return.
    {
      uint256 existing;
      if (ExtractBTCPREVCommitment(block, existing)) {
        if (existing != btcPrevHash) {
          throw std::runtime_error("BTCPREV commitment mismatch with existing template");
        }
        return;
      }
    }

    CMutableTransaction mtx(*block.vtx[0]);
    int nOut = GetSyscoinDataOutput(mtx);
    if (nOut == -1) {
      throw std::runtime_error("invalid coinbase: missing OP_RETURN output");
    }

    // Preserve witness-commitment push (if present) as the first push after OP_RETURN.
    const CScript& oldScript = mtx.vout[nOut].scriptPubKey;
    CScript::const_iterator pc = oldScript.begin();
    opcodetype opcode;
    std::vector<unsigned char> firstPush;
    bool hasWitnessCommit{false};
    std::vector<unsigned char> payload;

    if (!oldScript.GetOp(pc, opcode) || opcode != OP_RETURN) {
      throw std::runtime_error("invalid coinbase: unexpected Syscoin OP_RETURN script");
    }
    if (!oldScript.GetOp(pc, opcode, firstPush)) {
      throw std::runtime_error("invalid coinbase: missing OP_RETURN data push");
    }
    if (IsWitnessCommitmentPush(firstPush)) {
      hasWitnessCommit = true;
      if (!oldScript.GetOp(pc, opcode, payload)) {
        payload.clear();
      }
    } else {
      payload = firstPush;
    }

    // Append BTCPREV marker + value to the (possibly empty) Syscoin payload.
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << BTCPREV_MAGIC_BYTES;
    ds << btcPrevHash;
    const auto bytes = MakeUCharSpan(ds);
    payload.insert(payload.end(), bytes.begin(), bytes.end());

    CScript newScript;
    newScript << OP_RETURN;
    if (hasWitnessCommit) {
      newScript << firstPush;
    }
    newScript << payload;
    mtx.vout[nOut].scriptPubKey = newScript;

    block.vtx[0] = MakeTransactionRef(std::move(mtx));
    block.hashMerkleRoot = BlockMerkleRoot(block);
}

}  // anonymous namespace
// SYSCOIN
const CBlock*
AuxpowMiner::getCurrentBlock (ChainstateManager &chainman, const CTxMemPool& mempool,
                              const CScript& scriptPubKey, uint256& target,
                              const std::optional<uint256>& btcPrevHash, bool btcpRequired)
{
  AssertLockHeld(cs);
  const CBlock* pblockCur = nullptr;

  {
    LOCK (cs_main);
    CScriptID scriptID (scriptPubKey);
    auto iter = curBlocks.find(scriptID);
    if (iter != curBlocks.end())
      pblockCur = iter->second;
    // SYSCOIN
    bool templateHasCorrectBTCPREV{true};
    if (btcpRequired && pblockCur != nullptr) {
      if (!btcPrevHash.has_value()) {
        templateHasCorrectBTCPREV = false;
      } else {
        uint256 existing;
        if (!ExtractBTCPREVCommitment(*pblockCur, existing) || existing != *btcPrevHash) {
          templateHasCorrectBTCPREV = false;
        }
      }
    }
    // SYSCOIN
    if (pblockCur == nullptr
        || pindexPrev != chainman.ActiveTip()
        || (mempool.GetTransactionsUpdated () != txUpdatedLast
            && GetTime () - startTime > 60)
        || !templateHasCorrectBTCPREV)
      {
        if (pindexPrev != chainman.ActiveTip())
          {
            /* Clear old blocks since they're obsolete now.  */
            blocks.clear ();
            templates.clear ();
            curBlocks.clear ();
          }

        /* Create new block with nonce = 0 and extraNonce = 1.  */
        std::unique_ptr<CBlockTemplate> newBlock
            = BlockAssembler (chainman.ActiveChainstate(), &mempool).CreateNewBlock (scriptPubKey);
        if (newBlock == nullptr)
          throw JSONRPCError (RPC_OUT_OF_MEMORY, "out of memory");

        /* Update state only when CreateNewBlock succeeded.  */
        txUpdatedLast = mempool.GetTransactionsUpdated ();
        pindexPrev = chainman.ActiveTip();
        startTime = GetTime ();

        /* Finalise it by setting the version and building the merkle root.  */
        IncrementExtraNonce (&newBlock->block, pindexPrev, extraNonce);
        // SYSCOIN
        const int32_t nChainId = chainman.GetConsensus ().nAuxpowChainId;
        const int32_t nVersion = chainman.m_versionbitscache.ComputeBlockVersion(pindexPrev, chainman.GetConsensus ());
        newBlock->block.SetBaseVersion(nVersion, nChainId);
        newBlock->block.SetAuxpowVersion (true);
        if(!fRegTest) {
          newBlock->block.SetNEVMVersion();
        }

        // SYSCOIN: commit BTCPREV into the sign-offset block's coinbase payload (merkle-root committed).
        if (btcpRequired) {
          if (!btcPrevHash.has_value()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "btcprevhash is required at this height");
          }
          InjectBTCPREVCommitment(newBlock->block, *btcPrevHash);
        }

        /* Save in our map of constructed blocks.  */
        pblockCur = &newBlock->block;
        curBlocks.try_emplace(scriptID, pblockCur);
        blocks[pblockCur->GetHash ()] = pblockCur;
        templates.push_back (std::move (newBlock));
      }
  }

  /* At this point, pblockCur is always initialised:  If we make it here
     without creating a new block above, it means that, in particular,
     pindexPrev == chainman->ActiveTip().  But for that to happen, we must
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
// SYSCOIN
const CScript AuxpowMiner::createScriptPubKey(const uint256& auxRoot, int height)
{
  CScript sysCommitScript;
  CDataStream ssData(SER_NETWORK, PROTOCOL_VERSION);
  ssData << pchSyscoinHeader;
  ssData << auxRoot;
  ssData << static_cast<uint32_t>(height);

  // Build OP_RETURN output script
  const auto bytesVec = MakeUCharSpan(ssData);
  sysCommitScript << OP_RETURN << std::vector<unsigned char>(bytesVec.begin(), bytesVec.end());
  return sysCommitScript;
}

UniValue
AuxpowMiner::createAuxBlock (const node::JSONRPCRequest& request,
                             const CScript& scriptPubKey)
{
  auxMiningCheck (request);
  // SYSCOIN
  const node::NodeContext& node = request.nodeContext? *request.nodeContext: EnsureAnyNodeContext(request.context);
  const CBlockIndex* pindexTip = WITH_LOCK(::cs_main, return node.chainman->ActiveChain().Tip(););
  LOCK (cs);
  // SYSCOIN
  std::optional<uint256> btcPrevHash;
  // createauxblock(address, [btcprevhash]) passes btcprevhash as params[1].
  // getauxblock([btcprevhash]) (wallet RPC) may pass btcprevhash as params[0].
  if (request.params.size() > 1 && !request.params[1].isNull()) {
      uint256 h;
      h.SetHex(request.params[1].get_str());
      btcPrevHash = h;
  } else if (request.params.size() == 1 && request.params[0].isStr()) {
      const std::string s = request.params[0].get_str();
      // Heuristic: treat a 32-byte hex string as btcprevhash (wallet getauxblock create path).
      if (s.size() == 64 && IsHex(s)) {
          uint256 h;
          h.SetHex(s);
          btcPrevHash = h;
      }
  }
  const int nextHeight = pindexTip ? (pindexTip->nHeight + 1) : 0;
  const int start = Params().GetConsensus().nCLReceiptStartBlock;
  const bool btcpRequired = nextHeight >= start && (nextHeight % BTCCHECK_PERIOD) == BTCCHECK_SIGN_OFFSET;

  const auto& mempool = EnsureAnyMemPool (request.nodeContext? request.nodeContext: request.context);
  uint256 target;
  const CBlock* pblock = getCurrentBlock (*node.chainman, mempool, scriptPubKey, target, btcPrevHash, btcpRequired);

  // SYSCOIN
  int nActiveHeight = pindexTip->nHeight - 5;
  nActiveHeight -= nActiveHeight % 10;
  const CBlockIndex* refIndex = pindexTip->GetAncestor(nActiveHeight);

  UniValue result(UniValue::VOBJ);
  result.pushKV ("hash", pblock->GetHash ().GetHex ());
  result.pushKV ("chainid", pblock->GetChainId ());
  result.pushKV ("previousblockhash", pblock->hashPrevBlock.GetHex ());
  result.pushKV ("coinbasevalue",
                 static_cast<int64_t> (pblock->vtx[0]->vout[0].nValue));
  // SYSCOIN
  result.pushKV ("coinbasescript", HexStr(createScriptPubKey(refIndex->GetBlockHash(), refIndex->nHeight)));
  result.pushKV ("bits", strprintf ("%08x", pblock->nBits));
  result.pushKV ("height", static_cast<int64_t> (pindexPrev->nHeight + 1));
  result.pushKV ("_target", HexStr (target));

  return result;
}

bool
AuxpowMiner::submitAuxBlock (const node::JSONRPCRequest& request,
                             const std::string& hashHex,
                             const std::string& auxpowHex) const
{
  auxMiningCheck (request);
  auto& chainman = EnsureAnyChainman (request.nodeContext? request.nodeContext: request.context);

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

  return chainman.ProcessNewBlock(shared_block, true, true, nullptr);
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