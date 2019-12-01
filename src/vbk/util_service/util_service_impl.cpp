#include "util_service_impl.hpp"

#include <chain.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <script/interpreter.h>
#include <script/sigcache.h>
#include <validation.h>

#include <vbk/pop_service.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/util.hpp>

namespace VeriBlock {

namespace {
inline bool set_error(ScriptError* ret, const ScriptError serror)
{
    if (ret)
        *ret = serror;
    return false;
}
typedef std::vector<unsigned char> valtype;

#define stacktop(i) (stack.at(stack.size() + (i)))
inline void popstack(std::vector<valtype>& stack)
{
    if (stack.empty())
        throw std::runtime_error("popstack(): stack empty");
    stack.pop_back();
}

} // namespace


bool UtilServiceImpl::CheckPopInputs(const CTransaction& tx, CValidationState& state, unsigned int flags, bool cacheSigStore, PrecomputedTransactionData& txdata)
{
    ScriptError error(SCRIPT_ERR_UNKNOWN_ERROR);

    auto checker = CachingTransactionSignatureChecker(&tx, 0, 0, cacheSigStore, txdata);
    if (!VerifyScript(tx.vin[0].scriptSig, tx.vout[0].scriptPubKey, &tx.vin[0].scriptWitness, flags | SCRIPT_VERIFY_POP, checker, &error))
        return state.Invalid(ValidationInvalidReason::TX_NOT_STANDARD, false, strprintf("invalid pop tx script (%s)", ScriptErrorString(error)));

    return true;
}

bool UtilServiceImpl::isKeystone(const CBlockIndex& block)
{
    return (block.nHeight % getService<Config>().keystone_interval) == 0;
}

const CBlockIndex* UtilServiceImpl::getPreviousKeystone(const CBlockIndex& block)
{
    const CBlockIndex* pblockWalk = &block;
    do {
        pblockWalk = pblockWalk->pprev;
    } while (pblockWalk != nullptr && !this->isKeystone(*pblockWalk));

    return pblockWalk;
}

int UtilServiceImpl::compareForks(const CBlockIndex& left, const CBlockIndex& right)
{
    const CBlockIndex* commonKeystone = FindCommonKeystone(&left, &right);

    assert(commonKeystone != nullptr);

    if (IsCrossedKeystoneBoudary(*commonKeystone, left) && IsCrossedKeystoneBoudary(*commonKeystone, right)) {
        auto& pop_service = VeriBlock::getService<VeriBlock::PopService>();
        return pop_service.compareTwoBranches(commonKeystone, &left, &right);
    }

    return 0;
}

KeystoneArray UtilServiceImpl::getKeystoneHashesForTheNextBlock(const CBlockIndex* pindexPrev)
{
    const CBlockIndex* pwalk = pindexPrev;

    KeystoneArray keystones;
    auto it = keystones.begin();
    auto end = keystones.end();
    while (it != end) {
        if (pwalk == nullptr) {
            break;
        }

        if (isKeystone(*pwalk)) {
            *it = pwalk->GetBlockHash();
            ++it;
        }

        pwalk = getPreviousKeystone(*pwalk);
    }
    return keystones;
}

const CBlockIndex* UtilServiceImpl::FindCommonKeystone(const CBlockIndex* leftFork, const CBlockIndex* rightFork)
{
    const CBlockIndex* workingLeft = leftFork;
    const CBlockIndex* workingRight = rightFork;

    while (workingLeft && workingRight) {
        if (workingLeft == workingRight && isKeystone(*workingLeft)) {
            break;
        } else if (workingLeft->nHeight > workingRight->nHeight) {
            workingLeft = workingLeft->pprev;
        } else if (workingRight->nHeight > workingLeft->nHeight) {
            workingRight = workingRight->pprev;
        } else if (workingRight->nHeight == workingLeft->nHeight) {
            workingRight = getPreviousKeystone(*workingRight);
            workingLeft = getPreviousKeystone(*workingLeft);
        }
    }

    if (workingLeft == nullptr || workingRight == nullptr) {
        return nullptr;
    }

    return workingRight;
}

bool UtilServiceImpl::IsCrossedKeystoneBoudary(const CBlockIndex& bottom, const CBlockIndex& tip)
{
    uint32_t keystone_interval = VeriBlock::getService<VeriBlock::Config>().keystone_interval;

    int32_t keystoneIntervalAmount = bottom.nHeight / keystone_interval;
    int32_t tipIntervalAmount = tip.nHeight / keystone_interval;

    return keystoneIntervalAmount < tipIntervalAmount;
}

uint256 UtilServiceImpl::makeTopLevelRoot(int height, const KeystoneArray& keystones, const uint256& txRoot)
{
    ContextInfoContainer context(height, keystones, txRoot);
    auto contextHash = context.getUnauthenticatedHash();
    return Hash(contextHash.begin(), contextHash.end(), txRoot.begin(), txRoot.end());
}

void UtilServiceImpl::addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev)
{
    PoPRewards rewards = getService<UtilService>().getPopRewards(pindexPrev);

    for (const auto& itr : rewards) {
        CTxOut out;
        out.scriptPubKey = itr.first;
        out.nValue = itr.second;
        coinbaseTx.vout.push_back(out);
    }
}

bool UtilServiceImpl::checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, CValidationState& state)
{
    PoPRewards rewards = getService<UtilService>().getPopRewards(pindexPrev);

    if (tx.vout.size() < rewards.size()) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false,
            strprintf("checkCoinbaseTxWithPopRewards(): coinbase has incorrect size of pop vouts (vouts_size=%d vs pop_vouts=%d)", tx.vout.size(), rewards.size()), "bad-pop-vouts-size");
    }

    CAmount nTotalPopReward = 0;
    for (const auto& out : tx.vout) {
        auto it = rewards.find(out.scriptPubKey);
        if (it != rewards.end() && it->second == out.nValue) {
            nTotalPopReward += it->second; // Pop reward
            rewards.erase(it);
        }
    }

    if (!rewards.empty()) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, strprintf("checkCoinbaseTxWithPopRewards(): coinbase has incorrect pop vout in coinbase trx"), "bad-pop-vout");
    }

    if (tx.GetValueOut() > nTotalPopReward + PoWBlockReward) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS,
            error("ConnectBlock(): coinbase pays too much (actual=%d vs limit=%d)", tx.GetValueOut(), PoWBlockReward + nTotalPopReward), "bad-cb-amount");
    }

    return true;
}

PoPRewards UtilServiceImpl::getPopRewards(const CBlockIndex& pindexPrev)
{
    PoPRewards rewards;
    auto& config = getService<Config>();
    auto& pop_service = VeriBlock::getService<VeriBlock::PopService>();

    int nextBlockHeight = pindexPrev.nHeight + 1;
    if (nextBlockHeight > config.POP_REWARD_PAYMENT_DELAY) {
        int32_t checkHeight = nextBlockHeight - config.POP_REWARD_PAYMENT_DELAY;
        const CBlockIndex* endorsedBlock = pindexPrev.GetAncestor(checkHeight);
        const CBlockIndex* contaningBlocksTip = pindexPrev.GetAncestor(checkHeight + config.POP_REWARD_SETTLEMENT_INTERVAL);
        const CBlockIndex* difficultyBlockStart = pindexPrev.GetAncestor(checkHeight - config.POP_DIFFICULTY_AVERAGING_INTERVAL);
        const CBlockIndex* difficultyBlockEnd = pindexPrev.GetAncestor(checkHeight + config.POP_REWARD_SETTLEMENT_INTERVAL - 1);

        if (!endorsedBlock || !contaningBlocksTip || !difficultyBlockStart || !difficultyBlockEnd) {
            return rewards;
        }

        std::string difficulty = pop_service.rewardsCalculatePopDifficulty(*difficultyBlockStart, *difficultyBlockEnd);
        pop_service.rewardsCalculateOutputs(checkHeight, *endorsedBlock, *contaningBlocksTip, difficulty, rewards);
    }

    return rewards;
}

bool UtilServiceImpl::validatePopTx(const CTransaction& tx, CValidationState& state)
{
    // validate input
    {
        if (tx.vin.size() != 1) {
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, "bad-pop-vin-not-single");
        }
        if (!validatePopTxInput(tx.vin[0], state)) {
            return false; // reason already set by ValidatePopTxInput
        }
    }

    // validate output
    {
        if (tx.vout.size() != 1) {
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, "bad-pop-vout-not-single");
        }
        if (!validatePopTxOutput(tx.vout[0], state)) {
            return false; // reason already set by ValidatePopTxOutput
        }
    }

    // check size
    auto& config = getService<Config>();
    if (::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) > config.max_pop_tx_weight) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, "bad-pop-txns-oversize");
    }

    return true;
}

bool UtilServiceImpl::validatePopTxInput(const CTxIn& in, CValidationState& state)
{
    if (!isVBKNoInput(in.prevout)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, "bad-pop-prevout");
    }
    return true;
}

bool UtilServiceImpl::validatePopTxOutput(const CTxOut& out, CValidationState& state)
{
    if (out.scriptPubKey.size() != 1) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, "bad-pop-out-scriptpubkey-not-single");
    }
    if (out.scriptPubKey[0] != OP_RETURN) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, "bad-pop-out-scriptpubkey-unexpected");
    }
    return true;
}


bool UtilServiceImpl::EvalScript(const CScript& script, std::vector<std::vector<uint8_t>>& stack, ScriptError* serror, Publications* ret, bool with_checks)
{
    CScript::const_iterator pc = script.begin();
    CScript::const_iterator pend = script.end();
    static const valtype vchFalse(0);
    static const valtype vchTrue(1, 1);
    opcodetype opcode;
    std::vector<unsigned char> vchPushValue;
    auto& config = getService<Config>();
    auto& pop = getService<PopService>();
    Publications pub;

    if (script.size() > config.max_pop_script_size) {
        return set_error(serror, SCRIPT_ERR_SCRIPT_SIZE);
    }

    size_t maxPushSize = std::max(config.max_atv_size, config.max_vtb_size);
    size_t minPushSize = std::min(config.min_atv_size, config.min_vtb_size);
    try {
        while (pc < pend) {
            if (!script.GetOp(pc, opcode, vchPushValue)) {
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }

            if (!vchPushValue.empty()) {
                // we don't know if it is ATV or VTB yet, so do sanity check here
                if (vchPushValue.size() > maxPushSize || vchPushValue.size() < minPushSize) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }
                stack.push_back(vchPushValue);
                continue;
            }

            // at this point, opcode is always one of these 3
            switch (opcode) {
            case OP_CHECKATV: {
                // tx has one ATV
                if (!pub.atv.empty()) {
                    return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                }

                // validate ATV size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_atv_size || el.size() < config.min_atv_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                // validate ATV content
                if (with_checks && !pop.checkATVinternally(el)) {
                    return set_error(serror, SCRIPT_ERR_VBK_ATVFAIL);
                }

                pub.atv = el;
                popstack(stack);
                break;
            }
            case OP_CHECKVTB: {
                // validate VTB size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_vtb_size || el.size() < config.min_vtb_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                // validate VTB content
                if (with_checks && !pop.checkVTBinternally(el)) {
                    return set_error(serror, SCRIPT_ERR_VBK_VTBFAIL);
                }

                // tx has many VTBs
                pub.vtbs.push_back(el);
                popstack(stack);
                break;
            }
            case OP_CHECKPOP: {
                // OP_CHECKPOP should be last opcode
                if (script.GetOp(pc, opcode, vchPushValue)) {
                    // we could read next opcode. extra opcodes is an error
                    return set_error(serror, SCRIPT_ERR_VBK_EXTRA_OPCODE);
                }

                // stack should be empty at this point
                if (!stack.empty()) {
                    return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
                }

                // we did all checks, put true on stack to signal successful execution
                stack.push_back(vchTrue);
                break;
            }
            default:
                // forbid any other opcodes in pop transactions
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }
        }
    } catch (...) {
        return set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    }

    if (pub.atv.empty()) {
        // must contain non-empty ATV
        return set_error(serror, SCRIPT_ERR_VBK_ATVFAIL);
    }

    if (pub.vtbs.empty()) {
        // must contain at least one VTB
        return set_error(serror, SCRIPT_ERR_VBK_VTBFAIL);
    }

    // set return value
    if (ret != nullptr) {
        *ret = pub;
    }

    return true;
}
} // namespace VeriBlock
