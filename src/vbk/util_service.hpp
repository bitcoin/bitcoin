#ifndef BITCOIN_SRC_VBK_UTIL_SERVICE_HPP
#define BITCOIN_SRC_VBK_UTIL_SERVICE_HPP

#include <array>

#include "interpreter.hpp"
#include <vbk/entity/context_info_container.hpp>
#include <vbk/entity/pop.hpp>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script_error.h>
#include <consensus/params.h>

class CBlockIndex;
class TxValidationState;
class BlockValidationState;
struct PrecomputedTransactionData;

namespace VeriBlock {

using PoPRewards = std::map<CScript, CAmount>;

struct UtilService {
    virtual ~UtilService() = default;

    virtual bool CheckPopInputs(const CTransaction& tx, TxValidationState& state, unsigned int flags, bool cacheSigStore, PrecomputedTransactionData& txdata) = 0;

    virtual bool isKeystone(const CBlockIndex& block) = 0;
    virtual const CBlockIndex* getPreviousKeystone(const CBlockIndex& block) = 0;
    virtual KeystoneArray getKeystoneHashesForTheNextBlock(const CBlockIndex* pindexPrev) = 0;

    virtual uint256 makeTopLevelRoot(int height, const KeystoneArray& keystones, const uint256& txRoot) = 0;

    virtual int compareForks(const CBlockIndex& left, const CBlockIndex& right) = 0;

    virtual PoPRewards getPopRewards(const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) = 0;
    virtual void addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) = 0;
    virtual bool checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams, BlockValidationState& state) = 0;

    virtual bool validatePopTx(const CTransaction& tx, TxValidationState& state) = 0;

    /**
     * Evaluate scriptSig of POP TX.
     * @param script[in] script to evaluate.
     * @param stack[in] stack
     * @param serror[out] error will be written here
     * @param pub[out] publication data (if any) will be written here
     * @param ctx[out] context data (if any) will be written here
     * @param type[out] to distinguish between tx type (publication, context), this argument will contain parsed data type.
     * @param with_checks[in] if true, atv&vtb checks will be enabled
     * @return true, if script is valid, false otherwise.
     */
    virtual bool EvalScript(const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, Publications* pub, Context* ctx, PopTxType* type, bool with_checks) = 0;
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_UTIL_SERVICE_HPP
