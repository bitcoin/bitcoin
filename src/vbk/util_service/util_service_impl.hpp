#ifndef BITCOIN_SRC_VBK_UTIL_SERVICE_UTIL_SERVICE_IMPL_HPP
#define BITCOIN_SRC_VBK_UTIL_SERVICE_UTIL_SERVICE_IMPL_HPP

#include "vbk/config.hpp"
#include "vbk/util_service.hpp"

#include <consensus/validation.h>
#include <vector>

namespace VeriBlock {

struct UtilServiceImpl : public UtilService {
    ~UtilServiceImpl() override = default;

    bool CheckPopInputs(const CTransaction& tx, TxValidationState& state, unsigned int flags, bool cacheSigStore, PrecomputedTransactionData& txdata) override;

    bool isKeystone(const CBlockIndex& block) override;

    const CBlockIndex* getPreviousKeystone(const CBlockIndex& block) override;

    KeystoneArray getKeystoneHashesForTheNextBlock(const CBlockIndex* pindexPrev) override;

    uint256 makeTopLevelRoot(int height, const KeystoneArray& keystones, const uint256& txRoot) override;

    /// same as:
    /// bool CForkResolution::operator()(const CBlockIndex* leftFork, const CBlockIndex* rightFork) const

    int compareForks(const CBlockIndex& left, const CBlockIndex& right) override;

    // Pop rewards methods
    PoPRewards getPopRewards(const CBlockIndex& pindexPrev) override;
    void addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev) override;
    bool checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, BlockValidationState& state) override;

    bool EvalScript(const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, Publications* pub, bool with_checks = true) override;
    bool validatePopTx(const CTransaction& tx, TxValidationState& state) override;
    bool validatePopTxInput(const CTxIn& in, TxValidationState& state);
    bool validatePopTxOutput(const CTxOut& in, TxValidationState& state);

    // statefull VeriBlock validation
    //bool blockPopValidation(const CBlock& block, const CBlockIndex* pindexPrev, const Consensus::Params& params, CValidationState& state) override;

protected:
    const CBlockIndex* FindCommonKeystone(const CBlockIndex* leftFork, const CBlockIndex* rightFork);

    bool IsCrossedKeystoneBoundary(const CBlockIndex& bottom, const CBlockIndex& tip);
};
} // namespace VeriBlock


#endif //BITCOIN_SRC_VBK_UTIL_SERVICE_UTIL_SERVICE_IMPL_HPP
