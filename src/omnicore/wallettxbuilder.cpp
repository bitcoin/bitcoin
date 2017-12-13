#include "omnicore/wallettxbuilder.h"

#include "omnicore/encoding.h"
#include "omnicore/errors.h"
#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/parsing.h"
#include "omnicore/script.h"
#include "omnicore/wallettxs.h"

#include "base58.h"
#include "coincontrol.h"
#include "core_io.h"
#include "script/script.h"
#include "script/standard.h"
#include "uint256.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

using mastercore::AddressToPubKey;
using mastercore::SelectCoins;
using mastercore::UseEncodingClassC;

// This function requests the wallet create an Omni transaction using the supplied parameters and payload
int WalletTxBuilder(const std::string& senderAddress, const std::string& receiverAddress, const std::string& redemptionAddress,
        int64_t referenceAmount, const std::vector<unsigned char>& data, uint256& txid, std::string& rawHex, bool commit)
{
#ifdef ENABLE_WALLET
    if (pwalletMain == NULL) return MP_ERR_WALLET_ACCESS;

    // Determine the class to send the transaction via - default is Class C
    int omniTxClass = OMNI_CLASS_C;
    if (!UseEncodingClassC(data.size())) omniTxClass = OMNI_CLASS_B;

    // Prepare the transaction - first setup some vars
    CCoinControl coinControl;
    CWalletTx wtxNew;
    int64_t nFeeRet = 0;
    int nChangePosInOut = -1;
    std::string strFailReason;
    std::vector<std::pair<CScript, int64_t> > vecSend;
    CReserveKey reserveKey(pwalletMain);

    // Next, we set the change address to the sender
    CBitcoinAddress addr = CBitcoinAddress(senderAddress);
    coinControl.destChange = addr.Get();

    // Select the inputs
    if (0 > SelectCoins(senderAddress, coinControl, referenceAmount)) { return MP_INPUTS_INVALID; }

    // Encode the data outputs
    switch(omniTxClass) {
        case OMNI_CLASS_B: { // declaring vars in a switch here so use an expicit code block
            CPubKey redeemingPubKey;
            const std::string& sAddress = redemptionAddress.empty() ? senderAddress : redemptionAddress;
            if (!AddressToPubKey(sAddress, redeemingPubKey)) {
                return MP_REDEMP_BAD_VALIDATION;
            }
            if (!OmniCore_Encode_ClassB(senderAddress,redeemingPubKey,data,vecSend)) { return MP_ENCODING_ERROR; }
        break; }
        case OMNI_CLASS_C:
            if(!OmniCore_Encode_ClassC(data,vecSend)) { return MP_ENCODING_ERROR; }
        break;
    }

    // Then add a paytopubkeyhash output for the recipient (if needed) - note we do this last as we want this to be the highest vout
    if (!receiverAddress.empty()) {
        CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(receiverAddress).Get());
        vecSend.push_back(std::make_pair(scriptPubKey, 0 < referenceAmount ? referenceAmount : GetDustThreshold(scriptPubKey)));
    }

    // Now we have what we need to pass to the wallet to create the transaction, perform some checks first

    if (!coinControl.HasSelected()) return MP_ERR_INPUTSELECT_FAIL;

    std::vector<CRecipient> vecRecipients;
    for (size_t i = 0; i < vecSend.size(); ++i) {
        const std::pair<CScript, int64_t>& vec = vecSend[i];
        CRecipient recipient = {vec.first, vec.second, false};
        vecRecipients.push_back(recipient);
    }

    // Ask the wallet to create the transaction (note mining fee determined by Bitcoin Core params)
    if (!pwalletMain->CreateTransaction(vecRecipients, wtxNew, reserveKey, nFeeRet, nChangePosInOut, strFailReason, &coinControl)) {
        PrintToLog("%s: ERROR: wallet transaction creation failed: %s\n", __func__, strFailReason);
        return MP_ERR_CREATE_TX;
    }

    // If this request is only to create, but not commit the transaction then display it and exit
    if (!commit) {
        rawHex = EncodeHexTx(wtxNew);
        return 0;
    } else {
        // Commit the transaction to the wallet and broadcast)
        PrintToLog("%s: %s; nFeeRet = %d\n", __func__, wtxNew.ToString(), nFeeRet);
        if (!pwalletMain->CommitTransaction(wtxNew, reserveKey)) return MP_ERR_COMMIT_TX;
        txid = wtxNew.GetHash();
        return 0;
    }
#else
    return MP_ERR_WALLET_ACCESS;
#endif

}