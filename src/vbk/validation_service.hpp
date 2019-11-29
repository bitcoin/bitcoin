#ifndef BITCOIN_SRC_VBK_VALIDATION_SERVICE_HPP
#define BITCOIN_SRC_VBK_VALIDATION_SERVICE_HPP

class CBlock;
class CBlockIndex;
class ContextInfoContainer;
class CBlockHeader;
class CValidationState;

namespace VeriBlock {

struct ValidationService {
    virtual ~ValidationService() = default;

    virtual bool validateAuthContextInfoContainerToHeader(
        const ContextInfoContainer& authenticatedContextInfoContainer,
        const CBlockHeader& headerToAuthenticatedTo) = 0;

    virtual bool validateVTBsInAltBlock(
        const CBlock& block,
        CValidationState& state) = 0;

    virtual bool determineATVPlausibilityWithAltChainRules(
        long long altChainIdentifier,
        const CBlockHeader& popEndorsementHeader,
        CValidationState& state) = 0;

    virtual bool validateATVsInAltBlock(
        const CBlock& block,
        const CBlockIndex& indexPrev,
        CValidationState& state) = 0;

    virtual bool addBlockPoPValidations(
        const CBlock& block,
        const CBlockIndex& indexPrev,
        CValidationState& state,
        bool fCheckMerkleRoot) = 0;
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_VALIDATION_SERVICE_HPP
