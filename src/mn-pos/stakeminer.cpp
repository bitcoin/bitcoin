#include "key.h"
#include "legacysigner.h"
#include "stakeminer.h"
#include "kernel.h"
#include "stakevalidation.h"
#include "util.h"

//! Search a specific period of timestamps to see if a valid proof hash is created
bool SearchTimeSpan(Kernel& kernel, uint32_t nTimeStart, uint32_t nTimeEnd, const uint256& nTarget)
{
    uint64_t nTimeStake = nTimeStart;
    kernel.SetStakeTime(nTimeStart);

    while (!kernel.IsValidProof(nTarget)) {
        ++nTimeStake;
        kernel.SetStakeTime(nTimeStake);

        //Searched through the requested period
        if (nTimeStake > nTimeEnd)
            break;
    }

    return kernel.IsValidProof(nTarget);
}

bool SignBlock(CBlock* pblock)
{
    CPubKey pubKeyNode;
    CKey keyNode;

    std::string strPrivKey;
    std::vector<unsigned char> vchSigSignover;
    if (fMasterNode) {
        strPrivKey = strMasterNodePrivKey;
        vchSigSignover = activeMasternode.vchSigSignover;
    } else {
        strPrivKey = strSystemNodePrivKey;
        vchSigSignover = activeSystemnode.vchSigSignover;
    }

    std::string strErrorMessage;
    if (!legacySigner.SetKey(strPrivKey, strErrorMessage, keyNode, pubKeyNode)) {
        strErrorMessage = strprintf("Can't find keys for masternode - %s", strErrorMessage);
        return error("%s: Failed to setkey %s\n", __func__, strErrorMessage);
    }

    // Switch keys if using signed over staking key
    if (!vchSigSignover.empty()) {
        pblock->stakePointer.pubKeyCollateral = pblock->stakePointer.pubKeyProofOfStake;
        pblock->stakePointer.pubKeyProofOfStake = pubKeyNode;
        pblock->stakePointer.vchSigCollateralSignOver = vchSigSignover;
    }

    if (pubKeyNode != pblock->stakePointer.pubKeyProofOfStake) {
        return error("%s: using wrong pubkey. pointer=%s pubkeynode=%s\n", __func__,
                  pblock->stakePointer.pubKeyProofOfStake.GetHash().GetHex(),
                  pubKeyNode.GetHash().GetHex());
    }

    std::vector<unsigned char> vchSig;
    if (!keyNode.Sign(pblock->GetHash(), vchSig))
        return error("%s : Failed to sign block\n", __func__);

    pblock->vchBlockSig = vchSig;
    return CheckBlockSignature(*pblock, pblock->stakePointer.pubKeyProofOfStake);
}


