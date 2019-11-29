#ifndef BITCOIN_SRC_VBK_UTIL_SERVICE_HPP
#define BITCOIN_SRC_VBK_UTIL_SERVICE_HPP

#include <array>

#include "vbk/entity/context_info_container.hpp"
#include "vbk/entity/publications.hpp"
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script_error.h>

class CBlockIndex;
class CValidationState;
struct PrecomputedTransactionData;

namespace VeriBlock {

using PoPRewards = std::map<CScript, CAmount>;

struct UtilService {
    virtual ~UtilService() = default;

    virtual bool CheckPopInputs(const CTransaction& tx, CValidationState& state, unsigned int flags, bool cacheSigStore, PrecomputedTransactionData& txdata) = 0;

    virtual bool isKeystone(const CBlockIndex& block) = 0;
    virtual const CBlockIndex* getPreviousKeystone(const CBlockIndex& block) = 0;
    virtual KeystoneArray getKeystoneHashesForTheNextBlock(const CBlockIndex* pindexPrev) = 0;

    virtual uint256 makeTopLevelRoot(int height, const KeystoneArray& keystones, const uint256& txRoot) = 0;

    /// same as:
    /// bool CForkResolution::operator()(const CBlockIndex* leftFork, const CBlockIndex* rightFork) const
    virtual int compareForks(const CBlockIndex& left, const CBlockIndex& right) = 0;

    // Pop rewards methods
    virtual PoPRewards getPopRewards(const CBlockIndex* pindexPrev) = 0;
    virtual void addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex* pindexPrev) = 0;
    virtual bool checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex* pindexPrev, CValidationState& state) = 0;

    virtual bool validatePopTx(const CTransaction& tx, CValidationState& state) = 0;

    /**
     * Executes script of POP transaction.
     * @param script bitcoin script
     * @param stack bitcoin stack
     * @param serror if set (not nullptr), will be equal to error code on errorneous script
     * @param pub if set (non nullptr), will be equal to parsed ATV & VTBs
     * @return true if script is valid, false otherwise
     */
    virtual bool EvalScript(const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, Publications* pub, bool with_checks = true) = 0;
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_UTIL_SERVICE_HPP
