#include <omnicore/wallettxbuilder.h>

#include <omnicore/encoding.h>
#include <omnicore/errors.h>
#include <omnicore/log.h>
#include <omnicore/omnicore.h>
#include <omnicore/parsing.h>
#include <omnicore/script.h>
#include <omnicore/walletutils.h>

#include <amount.h>
#include <base58.h>
#include <coins.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <validation.h>
#include <net.h>
#include <node/context.h>
#include <node/transaction.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/standard.h>
#include <sync.h>
#include <txmempool.h>
#include <uint256.h>
#ifdef ENABLE_WALLET
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>
#endif

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

using mastercore::AddressToPubKey;
using mastercore::UseEncodingClassC;

/** Creates and sends a transaction. */
int WalletTxBuilder(
        const std::string& senderAddress,
        const std::string& receiverAddress,
        const std::string& redemptionAddress,
        int64_t referenceAmount,
        const std::vector<unsigned char>& payload,
        uint256& retTxid,
        std::string& retRawTx,
        bool commit,
        interfaces::Wallet* iWallet,
        CAmount minFee)
{
#ifdef ENABLE_WALLET
    if (!iWallet) return MP_ERR_WALLET_ACCESS;

    // Determine the class to send the transaction via - default is Class C
    int omniTxClass = OMNI_CLASS_C;
    if (!UseEncodingClassC(payload.size() + 1 /* OP_RETURN */ + 2 /* pushdata opcodes */)) omniTxClass = OMNI_CLASS_B;

    // Prepare the transaction - first setup some vars
    CCoinControl coinControl;
    std::vector<std::pair<CScript, int64_t> > vecSend;

    // Next, we set the change address to the sender
    coinControl.destChange = DecodeDestination(senderAddress);

    // Amount required for outputs
    CAmount outputAmount{0};

    // Encode the data outputs
    switch(omniTxClass) {
        case OMNI_CLASS_B: { // declaring vars in a switch here so use an expicit code block
            CPubKey redeemingPubKey;
            const std::string& sAddress = redemptionAddress.empty() ? senderAddress : redemptionAddress;
            if (!AddressToPubKey(iWallet, sAddress, redeemingPubKey)) {
                return MP_REDEMP_BAD_VALIDATION;
            }
            if (!OmniCore_Encode_ClassB(senderAddress, redeemingPubKey, payload, vecSend, &outputAmount)) { return MP_ENCODING_ERROR; }
        break; }
        case OMNI_CLASS_C:
            if(!OmniCore_Encode_ClassC(payload,vecSend)) { return MP_ENCODING_ERROR; }
        break;
    }

    // Then add a paytopubkeyhash output for the recipient (if needed) - note we do this last as we want this to be the highest vout
    if (!receiverAddress.empty()) {
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(receiverAddress));
        outputAmount += 0 < referenceAmount ? referenceAmount : OmniGetDustThreshold(scriptPubKey);
        vecSend.push_back(std::make_pair(scriptPubKey, 0 < referenceAmount ? referenceAmount : OmniGetDustThreshold(scriptPubKey)));
    }

    // Create CRecipients for outputs
    std::vector<CRecipient> vecRecipients;
    for (size_t i = 0; i < vecSend.size(); ++i) {
        const std::pair<CScript, int64_t>& vec = vecSend[i];
        CRecipient recipient = {vec.first, vec.second, false};
        vecRecipients.push_back(recipient);
    }

    CAmount nFeeRequired{std::max(minFee, iWallet->getMinimumFee(1000, coinControl, nullptr, nullptr))};
    CTransactionRef wtxNew;
    std::string strFailReason;
    CAmount nFeeRet{0};
    bool createTX{true};

    while (createTX) {
        // Select the inputs
        auto selected = mastercore::SelectCoins(*iWallet, senderAddress, coinControl, outputAmount + nFeeRequired);

        // Did not select anything at all!
        if (!coinControl.HasSelected()) {
            return MP_ERR_INPUTSELECT_FAIL;
        }

        // Could not select to enough to cover outputs and fee
        if (selected < outputAmount) {
            return MP_INPUTS_INVALID;
        }

        // Ask the wallet to create the transaction (note mining fee determined by Bitcoin Core params)
        int nChangePosInOut = -1;
        wtxNew = iWallet->createTransaction(vecRecipients, coinControl, true /* sign */, nChangePosInOut, nFeeRet, strFailReason, false, minFee);

        // TX creation was a success or fee no longer incremeneintg
        if (wtxNew || nFeeRet <= nFeeRequired) {
            createTX = false;
        } else {
            // Set new fee required
            nFeeRequired = nFeeRet;

            PrintToLog("Increasing fee. nFeeRequired: %d selected: %d outputAmount: %d\n", nFeeRequired, selected, outputAmount);
        }
    }

    if (!wtxNew) {
        PrintToLog("%s: ERROR: wallet transaction creation failed: %s\n", __func__, strFailReason);
        return MP_ERR_CREATE_TX;
    }

    // If this request is only to create, but not commit the transaction then display it and exit
    if (!commit) {
        retRawTx = EncodeHexTx(*wtxNew);
        return 0;
    } else {
        // Commit the transaction to the wallet and broadcast)
        PrintToLog("%s: %s; nFeeRet = %d\n", __func__, wtxNew->ToString(), nFeeRet);
        iWallet->commitTransaction(wtxNew, {}, {});
        retTxid = wtxNew->GetHash();
        return 0;
    }
#else
    return MP_ERR_WALLET_ACCESS;
#endif

}

#ifdef ENABLE_WALLET
/** Locks all available coins that are not in the set of destinations. */
static void LockUnrelatedCoins(
        interfaces::Wallet* iWallet,
        const std::set<CTxDestination>& destinations,
        std::vector<COutPoint>& retLockedCoins)
{
    if (!iWallet) {
        return;
    }

    // NOTE: require: LOCK2(cs_main, pwallet->cs_wallet);

    // lock any other output
    std::vector<COutput> vCoins;
    iWallet->availableCoins(vCoins, false, nullptr, 0);

    for (COutput& output : vCoins) {
        CTxDestination address;
        const CScript& scriptPubKey = output.tx->tx->vout[output.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, address);

        // don't lock specified coins, but any other
        if (fValidAddress && destinations.count(address)) {
            continue;
        }

        COutPoint outpointLocked(output.tx->GetHash(), output.i);
        iWallet->lockCoin(outpointLocked);
        retLockedCoins.push_back(outpointLocked);
    }
}

/** Unlocks all coins, which were previously locked. */
static void UnlockCoins(
        interfaces::Wallet* iWallet,
        const std::vector<COutPoint>& vToUnlock)
{
    // NOTE: require: LOCK2(cs_main, pwallet->cs_wallet);

    for (const COutPoint& output : vToUnlock) {
        iWallet->unlockCoin(output);
    }
}

/**
 * Creates and sends a raw transaction by selecting all coins from the sender
 * and enough coins from a fee source. Change is sent to the fee source!
 */
int CreateFundedTransaction(
        const std::string& senderAddress,
        const std::string& receiverAddress,
        const std::string& feeAddress,
        const std::vector<unsigned char>& payload,
        uint256& retTxid,
        interfaces::Wallet* iWallet,
        NodeContext& node)
{
    if (!iWallet) {
        return MP_ERR_WALLET_ACCESS;
    }

    if (!UseEncodingClassC(payload.size())) {
        return MP_ENCODING_ERROR;
    }
    
    // add payload output
    std::vector<std::pair<CScript, int64_t> > vecSend;
    if (!OmniCore_Encode_ClassC(payload, vecSend)) {
        return MP_ENCODING_ERROR;
    }

    // Maximum number of expected outputs
    std::vector<CTxOut>::size_type max_outputs = 2;

    // add reference output, if there is one
    if (!receiverAddress.empty() && receiverAddress != feeAddress) {
        max_outputs = 3;
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(receiverAddress));
        vecSend.push_back(std::make_pair(scriptPubKey, OmniGetDustThreshold(scriptPubKey)));
    }

    // convert into recipients objects
    std::vector<CRecipient> vecRecipients;
    for (size_t i = 0; i < vecSend.size(); ++i) {
        const std::pair<CScript, int64_t>& vec = vecSend[i];
        CRecipient recipient = {vec.first, vec.second, false};
        vecRecipients.push_back(recipient);
    }

    bool fSuccess = false;
    CAmount nFeeRequired = 0;
    std::string strFailReason;
    int nChangePosRet = 0; // add change first

    // set change
    CCoinControl coinControl;
    coinControl.destChange = DecodeDestination(feeAddress);
    coinControl.fAllowOtherInputs = true;

    if (!mastercore::SelectAllCoins(*iWallet, senderAddress, coinControl)) {
        PrintToLog("%s: ERROR: sender %s has no coins\n", __func__, senderAddress);
        return MP_INPUTS_INVALID;
    }

    // prepare sources for fees
    std::set<CTxDestination> feeSources;
    feeSources.insert(DecodeDestination(feeAddress));

    std::vector<COutPoint> vLockedCoins;
    LockUnrelatedCoins(iWallet, feeSources, vLockedCoins);

    auto wtxNew = iWallet->createTransaction(vecRecipients, coinControl, false /* sign */, nChangePosRet, nFeeRequired, strFailReason, true);

    if (wtxNew) {
        fSuccess = true;
    }

    if (fSuccess && nChangePosRet == -1 && receiverAddress == feeAddress) {
        fSuccess = false;
        strFailReason = "send to self without change";
    }

    if (fSuccess && wtxNew->vout.size() > max_outputs)
    {
        strFailReason = "more outputs than expected";
        PrintToLog("%s: ERROR: more outputs than expected (Max expected %d, actual %d)\n Failed transaction: %s\n",
                   __func__, max_outputs, wtxNew->vout.size(), wtxNew->ToString());
    }

    // to restore the original order of inputs, create a new transaction and add
    // inputs and outputs step by step
    CMutableTransaction tx;

    if (fSuccess)
    {
        std::vector<COutPoint> vSelectedInputs;
        coinControl.ListSelected(vSelectedInputs);

        // add previously selected coins
        for(const COutPoint& txIn : vSelectedInputs) {
            tx.vin.push_back(CTxIn(txIn));
        }

        // add other selected coins
        for(const CTxIn& txin : wtxNew->vin) {
            if (!coinControl.IsSelected(txin.prevout)) {
                tx.vin.push_back(txin);
            }
        }

        // add outputs
        for(const CTxOut& txOut : wtxNew->vout) {
            tx.vout.push_back(txOut);
        }
    }

    // restore original locking state
    UnlockCoins(iWallet, vLockedCoins);

    // lock selected outputs for this transaction // TODO: could be removed?
    if (fSuccess) {
        for(const CTxIn& txIn : tx.vin) {
            iWallet->lockCoin(txIn.prevout);
        }
    }

    if (!fSuccess) {
        PrintToLog("%s: ERROR: wallet transaction creation failed: %s\n", __func__, strFailReason);
        return MP_ERR_CREATE_TX;
    }

    // sign the transaction

    // fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK2(cs_main, mempool.cs);
        CCoinsViewCache &viewChain = ::ChainstateActive().CoinsTip();
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for(const CTxIn& txin : tx.vin) {
            const auto& prevout = txin.prevout;
            view.AccessCoin(prevout); // this is certainly allowed to fail
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    int nHashType = SIGHASH_ALL;

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        CTxIn& txin = tx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            PrintToLog("%s: ERROR: wallet transaction signing failed: input not found or already spent\n", __func__);
            continue;
        }
        const CScript& prevPubKey = coin.out.scriptPubKey;
        const CAmount& amount = coin.out.nValue;

        SignatureData sigdata;
        if (!iWallet->produceSignature(MutableTransactionSignatureCreator(&tx, i, amount, nHashType), prevPubKey, sigdata)) {
            PrintToLog("%s: ERROR: wallet transaction signing failed\n", __func__);
            return MP_ERR_CREATE_TX;
        }

        UpdateInput(txin, sigdata);
    }

    // send the transaction

    TxValidationState state;
    CTransactionRef ctx(MakeTransactionRef(std::move(tx)));

    {
        LOCK(cs_main);
        if (!AcceptToMemoryPool(mempool, state, ctx, nullptr, false, DEFAULT_TRANSACTION_MAXFEE)) {
            PrintToLog("%s: ERROR: failed to broadcast transaction: %s\n", __func__, state.GetRejectReason());
            return MP_ERR_COMMIT_TX;
        }
    }

    std::string err_string;

    const TransactionError err = BroadcastTransaction(node, ctx, err_string, iWallet->getDefaultMaxTxFee(), true, false);
    if (TransactionError::OK != err) {
        LogPrintf("%s: BroadcastTransaction failed error: %s\n", __func__, err_string);
    }

    retTxid = ctx->GetHash();

    return 0;
}

/**
 * Used by the omni_senddexpay RPC call to creates and send a
 * transaction to pay for an accepted offer on the traditional DEx.
 */
int CreateDExTransaction(interfaces::Wallet* pwallet, const std::string& buyerAddress, const std::string& sellerAddress, const CAmount& nAmount, uint256& txid)
{
    if (!pwallet) {
        return MP_ERR_WALLET_ACCESS;
    }

    // Set the change address to the sender
    CCoinControl coinControl;
    coinControl.destChange = DecodeDestination(buyerAddress);

    // Create scripts for outputs
    CScript exodus = GetScriptForDestination(ExodusAddress());
    CScript destScript = GetScriptForDestination(DecodeDestination(sellerAddress));

    // Calculate dust for Exodus output
    CAmount dust = OmniGetDustThreshold(exodus);

    // Create CRecipients for outputs
    std::vector<CRecipient> vecRecipients;
    vecRecipients.push_back({exodus, dust, false}); // Exodus
    vecRecipients.push_back({destScript, nAmount, false}); // Seller

    CAmount nFeeRequired{pwallet->getMinimumFee(1000, coinControl, nullptr, nullptr)};
    CTransactionRef wtxNew;
    std::string strFailReason;
    CAmount nFeeRet{0};
    CAmount outputAmount{nAmount + dust};
    bool createTX{true};

    while (createTX) {
        // Select the inputs
        auto selected = mastercore::SelectCoins(*pwallet, buyerAddress, coinControl, outputAmount + nFeeRequired);

        // Did not select anything at all!
        if (!coinControl.HasSelected()) {
            return MP_ERR_INPUTSELECT_FAIL;
        }

        // Could not select to enough to cover outputs and fee
        if (selected < outputAmount) {
            return MP_INPUTS_INVALID;
        }

        int nChangePosInOut = -1;
        wtxNew = pwallet->createTransaction(vecRecipients, coinControl, true /* sign */, nChangePosInOut, nFeeRet, strFailReason, false);

        // TX creation was a success or fee no longer incremeneintg
        if (wtxNew || nFeeRet <= nFeeRequired) {
            createTX = false;
        } else {
            // Set new fee required
            nFeeRequired = nFeeRet;

            PrintToLog("Increasing fee. nFeeRequired: %d selected: %d outputAmount: %d\n", nFeeRequired, selected, outputAmount);
        }
    }

    if (!wtxNew) {
        return MP_ERR_CREATE_TX;
    }

    // Commit the transaction to the wallet and broadcast
    pwallet->commitTransaction(wtxNew, {}, {});
    txid = wtxNew->GetHash();

    return 0;
}
#endif
