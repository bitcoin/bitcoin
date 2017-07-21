// Copyright (c) 2017 The Bitcoin Unlimited Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "unlimited.h"
#include "buip055fork.h"
#include "chain.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "clientversion.h"
#include "consensus/consensus.h"
#include "consensus/params.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "expedited.h"
#include "hash.h"
#include "leakybucket.h"
#include "main.h"
#include "miner.h"
#include "net.h"
#include "parallel.h"
#include "policy/policy.h"
#include "primitives/block.h"
#include "requestManager.h"
#include "rpc/server.h"
#include "stat.h"
#include "thinblock.h"
#include "timedata.h"
#include "tinyformat.h"
#include "tweak.h"
#include "txmempool.h"
#include "ui_interface.h"
#include "util.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"
#include "validationinterface.h"
#include "version.h"

#include <atomic>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <inttypes.h>
#include <iomanip>
#include <limits>
#include <queue>

const int REQ_6_1_SUNSET_HEIGHT = 530000;

static const std::string ANTI_REPLAY_MAGIC_VALUE = "Bitcoin: A Peer-to-Peer Electronic Cash System";

std::vector<unsigned char> invalidOpReturn =
    std::vector<unsigned char>(std::begin(ANTI_REPLAY_MAGIC_VALUE), std::end(ANTI_REPLAY_MAGIC_VALUE));


bool UpdateBUIP055Globals(CBlockIndex *activeTip)
{
    if (activeTip)
    {
        if ((miningForkTime.value != 0) && activeTip->forkAtNextBlock(miningForkTime.value))
        {
            excessiveBlockSize = miningForkEB.value;
            maxGeneratedBlock = miningForkMG.value;
            return true;
        }
    }
    return false;
}

bool ValidateBUIP055Block(const CBlock &block, CValidationState &state, int nHeight)
{
    // Validate transactions are HF compatible
    for (const CTransaction &tx : block.vtx)
    {
        if ((nHeight <= REQ_6_1_SUNSET_HEIGHT) && (!ValidateBUIP055Tx(tx)))
            return state.DoS(100,
                             error("transaction is invalid on BUIP055 chain"), REJECT_INVALID, "bad-txns-wrong-fork");
    }
    return true;
}

bool ValidateBUIP055Tx(const CTransaction& tx)
{
    bool ret = !IsTxOpReturnInvalid(tx);
    // TODO: validate new signature (REQ-6-2, REQ-6-3)
    return ret;
}

bool IsTxOpReturnInvalid(const CTransaction &tx)
{
    for (auto txout : tx.vout)
    {
        int idx = txout.scriptPubKey.Find(OP_RETURN);
        if (idx)
        {
            CScript::const_iterator pc(txout.scriptPubKey.begin());
            opcodetype op;
            for (;pc != txout.scriptPubKey.end();)
            {
                if (txout.scriptPubKey.GetOp(pc, op))
                {
                    if (op == OP_RETURN) break;
                }
            }
            if (pc != txout.scriptPubKey.end())
            {
                std::vector<unsigned char> data;
                if (txout.scriptPubKey.GetOp(pc, op, data))
                {
                    // Note this code only works if the size <= 75 (or we'd have OP_PUSHDATAn instead)
                    if (op == invalidOpReturn.size())
                    {
                        if (data == invalidOpReturn)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}
