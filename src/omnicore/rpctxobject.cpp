/**
 * @file rpctxobject.cpp
 *
 * Handler for populating RPC transaction objects.
 */

#include <omnicore/rpctxobject.h>

#include <omnicore/dbspinfo.h>
#include <omnicore/dbstolist.h>
#include <omnicore/dbtradelist.h>
#include <omnicore/dbtransaction.h>
#include <omnicore/dbtxlist.h>
#include <omnicore/dex.h>
#include <omnicore/errors.h>
#include <omnicore/mdex.h>
#include <omnicore/omnicore.h>
#include <omnicore/parsing.h>
#include <omnicore/pending.h>
#include <omnicore/sp.h>
#include <omnicore/sto.h>
#include <omnicore/tx.h>
#include <omnicore/utilsbitcoin.h>
#include <omnicore/walletutils.h>

#include <chainparams.h>
#include <index/txindex.h>
#include <validation.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <uint256.h>

#include <univalue.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>
#include <string>
#include <vector>

// Namespaces
using namespace mastercore;

/**
 * Function to standardize RPC output for transactions into a JSON object in either basic or extended mode.
 *
 * Use basic mode for generic calls (e.g. omni_gettransaction/omni_listtransaction etc.)
 * Use extended mode for transaction specific calls (e.g. omni_getsto, omni_gettrade etc.)
 *
 * DEx payments and the extended mode are only available for confirmed transactions.
 */
int populateRPCTransactionObject(const uint256& txid, UniValue& txobj, std::string filterAddress, bool extendedDetails, std::string extendedDetailsFilter, interfaces::Wallet* iWallet)
{
    bool f_txindex_ready = false;
    if (g_txindex) {
        f_txindex_ready = g_txindex->BlockUntilSyncedToCurrentChain();
    }

    // retrieve the transaction from the blockchain and obtain it's height/confs/time
    CTransactionRef tx;
    uint256 blockHash;
    if (!GetTransaction(txid, tx, Params().GetConsensus(), blockHash)) {
        if (!f_txindex_ready) {
            return MP_TXINDEX_STILL_SYNCING;
        } else {
            return MP_TX_NOT_FOUND;
        }
    }

    return populateRPCTransactionObject(*tx, blockHash, txobj, filterAddress, extendedDetails, extendedDetailsFilter, 0, iWallet);
}

int populateRPCTransactionObject(const CTransaction& tx, const uint256& blockHash, UniValue& txobj, std::string filterAddress, bool extendedDetails, std::string extendedDetailsFilter, int blockHeight, interfaces::Wallet* iWallet)
{
    int confirmations = 0;
    int64_t blockTime = 0;
    int positionInBlock = 0;

    if (blockHeight == 0) {
        blockHeight = GetHeight();
    }

    if (!blockHash.IsNull()) {
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (nullptr != pBlockIndex) {
            confirmations = 1 + blockHeight - pBlockIndex->nHeight;
            blockTime = pBlockIndex->nTime;
            blockHeight = pBlockIndex->nHeight;
        }
    }

    // attempt to parse the transaction
    CMPTransaction mp_obj;
    int parseRC = ParseTransaction(tx, blockHeight, 0, mp_obj, blockTime);
    if (parseRC == -101) {
        return MP_RPC_DECODE_INPUTS_MISSING;
    }
    if (parseRC < 0) {
        return MP_TX_IS_NOT_OMNI_PROTOCOL;
    }

    const uint256& txid = tx.GetHash();

    // DEx BTC payment needs special handling since it's not actually an Omni message - handle and return
    if (parseRC > 0) {
        if (confirmations <= 0) {
            // only confirmed DEx payments are currently supported
            return MP_TX_UNCONFIRMED;
        }
        std::string tmpBuyer, tmpSeller;
        uint64_t tmpVout, tmpNValue, tmpPropertyId;
        {
            LOCK(cs_tally);
            pDbTransactionList->getPurchaseDetails(txid, 1, &tmpBuyer, &tmpSeller, &tmpVout, &tmpPropertyId, &tmpNValue);
        }
        UniValue purchases(UniValue::VARR);
        if (populateRPCDExPurchases(tx, purchases, filterAddress, iWallet) <= 0) return -1;
        txobj.pushKV("txid", txid.GetHex());
        txobj.pushKV("type", "DEx Purchase");
        txobj.pushKV("sendingaddress", tmpBuyer);
        txobj.pushKV("purchases", purchases);
        txobj.pushKV("blockhash", blockHash.GetHex());
        txobj.pushKV("blocktime", blockTime);
        txobj.pushKV("block", blockHeight);
        txobj.pushKV("confirmations", confirmations);
        return 0;
    }

    // check if we're filtering from listtransactions_MP, and if so whether we have a non-match we want to skip
    if (!filterAddress.empty() && mp_obj.getSender() != filterAddress && mp_obj.getReceiver() != filterAddress) return -1;

    // parse packet and populate mp_obj
    if (!mp_obj.interpret_Transaction()) return MP_TX_IS_NOT_OMNI_PROTOCOL;

    // obtain validity - only confirmed transactions can be valid
    bool valid = false;
    if (confirmations > 0) {
        LOCK(cs_tally);
        valid = pDbTransactionList->getValidMPTX(txid);
        positionInBlock = pDbTransaction->FetchTransactionPosition(txid);
    }

    // populate some initial info for the transaction
    bool fMine = false;
    if (IsMyAddress(mp_obj.getSender(), iWallet) || IsMyAddress(mp_obj.getReceiver(), iWallet)) fMine = true;
    txobj.pushKV("txid", txid.GetHex());
    txobj.pushKV("fee", FormatDivisibleMP(mp_obj.getFeePaid()));
    txobj.pushKV("sendingaddress", mp_obj.getSender());
    if (showRefForTx(mp_obj.getType())) txobj.pushKV("referenceaddress", mp_obj.getReceiver());
    txobj.pushKV("ismine", fMine);
    txobj.pushKV("version", (uint64_t)mp_obj.getVersion());
    txobj.pushKV("type_int", (uint64_t)mp_obj.getType());
    if (mp_obj.getType() != MSC_TYPE_SIMPLE_SEND) { // Type 0 will add "Type" attribute during populateRPCTypeSimpleSend
        txobj.pushKV("type", mp_obj.getTypeString());
    }

    // populate type specific info and extended details if requested
    // extended details are not available for unconfirmed transactions
    if (confirmations <= 0) extendedDetails = false;
    populateRPCTypeInfo(mp_obj, txobj, mp_obj.getType(), extendedDetails, extendedDetailsFilter, confirmations, iWallet);

    // state and chain related information
    if (confirmations != 0 && !blockHash.IsNull()) {
        txobj.pushKV("valid", valid);
        if (!valid) {
            txobj.pushKV("invalidreason", pDbTransaction->FetchInvalidReason(txid));
        }
        txobj.pushKV("blockhash", blockHash.GetHex());
        txobj.pushKV("blocktime", blockTime);
        txobj.pushKV("positioninblock", positionInBlock);
    }
    if (confirmations != 0) {
        txobj.pushKV("block", blockHeight);
    }
    txobj.pushKV("confirmations", confirmations);

    // finished
    return 0;
}

/* Function to call respective populators based on message type
 */
void populateRPCTypeInfo(CMPTransaction& mp_obj, UniValue& txobj, uint32_t txType, bool extendedDetails, std::string extendedDetailsFilter, int confirmations, interfaces::Wallet *iWallet)
{
    switch (txType) {
        case MSC_TYPE_SIMPLE_SEND:
            populateRPCTypeSimpleSend(mp_obj, txobj);
            break;
        case MSC_TYPE_SEND_TO_OWNERS:
            populateRPCTypeSendToOwners(mp_obj, txobj, extendedDetails, extendedDetailsFilter, iWallet);
            break;
        case MSC_TYPE_SEND_ALL:
            populateRPCTypeSendAll(mp_obj, txobj, confirmations);
            break;
        case MSC_TYPE_TRADE_OFFER:
            populateRPCTypeTradeOffer(mp_obj, txobj);
            break;
        case MSC_TYPE_METADEX_TRADE:
            populateRPCTypeMetaDExTrade(mp_obj, txobj, extendedDetails);
            break;
        case MSC_TYPE_METADEX_CANCEL_PRICE:
            populateRPCTypeMetaDExCancelPrice(mp_obj, txobj, extendedDetails);
            break;
        case MSC_TYPE_METADEX_CANCEL_PAIR:
            populateRPCTypeMetaDExCancelPair(mp_obj, txobj, extendedDetails);
            break;
        case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM:
            populateRPCTypeMetaDExCancelEcosystem(mp_obj, txobj, extendedDetails);
            break;
        case MSC_TYPE_ACCEPT_OFFER_BTC:
            populateRPCTypeAcceptOffer(mp_obj, txobj);
            break;
        case MSC_TYPE_CREATE_PROPERTY_FIXED:
            populateRPCTypeCreatePropertyFixed(mp_obj, txobj, confirmations);
            break;
        case MSC_TYPE_CREATE_PROPERTY_VARIABLE:
            populateRPCTypeCreatePropertyVariable(mp_obj, txobj, confirmations);
            break;
        case MSC_TYPE_CREATE_PROPERTY_MANUAL:
            populateRPCTypeCreatePropertyManual(mp_obj, txobj, confirmations);
            break;
        case MSC_TYPE_CLOSE_CROWDSALE:
            populateRPCTypeCloseCrowdsale(mp_obj, txobj);
            break;
        case MSC_TYPE_GRANT_PROPERTY_TOKENS:
            populateRPCTypeGrant(mp_obj, txobj);
            break;
        case MSC_TYPE_REVOKE_PROPERTY_TOKENS:
            populateRPCTypeRevoke(mp_obj, txobj);
            break;
        case MSC_TYPE_CHANGE_ISSUER_ADDRESS:
            populateRPCTypeChangeIssuer(mp_obj, txobj);
            break;
        case MSC_TYPE_ENABLE_FREEZING:
            populateRPCTypeEnableFreezing(mp_obj, txobj);
            break;
        case MSC_TYPE_DISABLE_FREEZING:
            populateRPCTypeDisableFreezing(mp_obj, txobj);
            break;
        case MSC_TYPE_FREEZE_PROPERTY_TOKENS:
            populateRPCTypeFreezeTokens(mp_obj, txobj);
            break;
        case MSC_TYPE_UNFREEZE_PROPERTY_TOKENS:
            populateRPCTypeUnfreezeTokens(mp_obj, txobj);
            break;
        case MSC_TYPE_ADD_DELEGATE :
            populateRPCTypeAddDelegate(mp_obj, txobj);
            break;
        case MSC_TYPE_REMOVE_DELEGATE:
            populateRPCTypeRemoveDelegate(mp_obj, txobj);
            break;
        case MSC_TYPE_ANYDATA:
            populateRPCTypeAnyData(mp_obj, txobj);
            break;
        case MSC_TYPE_SEND_NONFUNGIBLE:
            populateRPCTypeSendNonFungible(mp_obj, txobj);
            break;
        case MSC_TYPE_NONFUNGIBLE_DATA:
            populateRPCTypeSetNonFungibleData(mp_obj, txobj);
            break;
        case OMNICORE_MESSAGE_TYPE_ACTIVATION:
            populateRPCTypeActivation(mp_obj, txobj);
            break;
    }
}

/* Function to determine whether to display the reference address based on transaction type
 */
bool showRefForTx(uint32_t txType)
{
    switch (txType) {
        case MSC_TYPE_SIMPLE_SEND: return true;
        case MSC_TYPE_SEND_TO_OWNERS: return false;
        case MSC_TYPE_TRADE_OFFER: return false;
        case MSC_TYPE_METADEX_TRADE: return false;
        case MSC_TYPE_METADEX_CANCEL_PRICE: return false;
        case MSC_TYPE_METADEX_CANCEL_PAIR: return false;
        case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM: return false;
        case MSC_TYPE_ACCEPT_OFFER_BTC: return true;
        case MSC_TYPE_CREATE_PROPERTY_FIXED: return false;
        case MSC_TYPE_CREATE_PROPERTY_VARIABLE: return false;
        case MSC_TYPE_CREATE_PROPERTY_MANUAL: return false;
        case MSC_TYPE_CLOSE_CROWDSALE: return false;
        case MSC_TYPE_GRANT_PROPERTY_TOKENS: return true;
        case MSC_TYPE_REVOKE_PROPERTY_TOKENS: return false;
        case MSC_TYPE_CHANGE_ISSUER_ADDRESS: return true;
        case MSC_TYPE_SEND_ALL: return true;
        case MSC_TYPE_ENABLE_FREEZING: return false;
        case MSC_TYPE_DISABLE_FREEZING: return false;
        case MSC_TYPE_FREEZE_PROPERTY_TOKENS: return true;
        case MSC_TYPE_UNFREEZE_PROPERTY_TOKENS: return true;
        case MSC_TYPE_REMOVE_DELEGATE: return true;
        case MSC_TYPE_ADD_DELEGATE: return true;
        case MSC_TYPE_ANYDATA: return true;
        case MSC_TYPE_SEND_NONFUNGIBLE: return true;
        case MSC_TYPE_NONFUNGIBLE_DATA: return true;
        case OMNICORE_MESSAGE_TYPE_ACTIVATION: return false;
    }
    return true; // default to true, shouldn't be needed but just in case
}

void populateRPCTypeSimpleSend(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    int64_t crowdPropertyId = 0, crowdTokens = 0, issuerTokens = 0;
    LOCK(cs_tally);
    bool crowdPurchase = isCrowdsalePurchase(omniObj.getHash(), omniObj.getReceiver(), &crowdPropertyId, &crowdTokens, &issuerTokens);
    if (crowdPurchase) {
        CMPSPInfo::Entry sp;
        if (false == pDbSpInfo->getSP(crowdPropertyId, sp)) {
            PrintToLog("SP Error: Crowdsale purchase for non-existent property %d in transaction %s", crowdPropertyId, omniObj.getHash().GetHex());
            return;
        }
        txobj.pushKV("type", "Crowdsale Purchase");
        txobj.pushKV("propertyid", (uint64_t)propertyId);
        txobj.pushKV("divisible", isPropertyDivisible(propertyId));
        txobj.pushKV("amount", FormatMP(propertyId, omniObj.getAmount()));
        txobj.pushKV("purchasedpropertyid", crowdPropertyId);
        txobj.pushKV("purchasedpropertyname", sp.name);
        txobj.pushKV("purchasedpropertydivisible", sp.isDivisible());
        txobj.pushKV("purchasedtokens", FormatMP(crowdPropertyId, crowdTokens));
        txobj.pushKV("issuertokens", FormatMP(crowdPropertyId, issuerTokens));
    } else {
        txobj.pushKV("type", "Simple Send");
        txobj.pushKV("propertyid", (uint64_t)propertyId);
        txobj.pushKV("divisible", isPropertyDivisible(propertyId));
        txobj.pushKV("amount", FormatMP(propertyId, omniObj.getAmount()));
    }
}

void populateRPCTypeSendToOwners(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails, std::string extendedDetailsFilter, interfaces::Wallet *iWallet)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("divisible", isPropertyDivisible(propertyId));
    txobj.pushKV("amount", FormatMP(propertyId, omniObj.getAmount()));
    if (extendedDetails) populateRPCExtendedTypeSendToOwners(omniObj.getHash(), extendedDetailsFilter, txobj, omniObj.getVersion(), iWallet);
}

void populateRPCTypeSendAll(CMPTransaction& omniObj, UniValue& txobj, int confirmations)
{
    UniValue subSends(UniValue::VARR);
    if (omniObj.getEcosystem() == 1)
        txobj.pushKV("ecosystem", "main");
    else if (omniObj.getEcosystem() == 2)
        txobj.pushKV("ecosystem", "test");
    else
        txobj.pushKV("ecosystem", "all");

    if (confirmations > 0) {
        if (populateRPCSendAllSubSends(omniObj.getHash(), subSends) > 0) txobj.pushKV("subsends", subSends);
    }
}

void populateRPCTypeTradeOffer(CMPTransaction& omniObj, UniValue& txobj)
{
    CMPOffer temp_offer(omniObj);
    uint32_t propertyId = omniObj.getProperty();
    int64_t amountOffered = omniObj.getAmount();
    int64_t amountDesired = temp_offer.getBTCDesiredOriginal();
    uint8_t sellSubAction = temp_offer.getSubaction();

    {
        // NOTE: some manipulation of sell_subaction is needed here
        // TODO: interpretPacket should provide reliable data, cleanup at RPC layer is not cool
        if (sellSubAction > 3) sellSubAction = 0; // case where subaction byte >3, to have been allowed must be a v0 sell, flip byte to 0
        if (sellSubAction == 0 && amountOffered > 0) sellSubAction = 1; // case where subaction byte=0, must be a v0 sell, amount > 0 means a new sell
        if (sellSubAction == 0 && amountOffered == 0) sellSubAction = 3; // case where subaction byte=0. must be a v0 sell, amount of 0 means a cancel
    }
    {
        // Check levelDB to see if the amount for sale has been amended due to a partial purchase
        // TODO: DEx phase 1 really needs an overhaul to work like MetaDEx with original amounts for sale and amounts remaining etc
        int tmpblock = 0;
        unsigned int tmptype = 0;
        uint64_t amountNew = 0;
        LOCK(cs_tally);
        bool tmpValid = pDbTransactionList->getValidMPTX(omniObj.getHash(), &tmpblock, &tmptype, &amountNew);
        if (tmpValid && amountNew > 0) {
            amountDesired = calculateDesiredBTC(amountOffered, amountDesired, amountNew);
            amountOffered = amountNew;
        }
    }

    // Populate
    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("divisible", isPropertyDivisible(propertyId));
    txobj.pushKV("amount", FormatMP(propertyId, amountOffered));
    txobj.pushKV("bitcoindesired", FormatDivisibleMP(amountDesired));
    txobj.pushKV("timelimit",  temp_offer.getBlockTimeLimit());
    txobj.pushKV("feerequired", FormatDivisibleMP(temp_offer.getMinFee()));
    if (sellSubAction == 1) txobj.pushKV("action", "new");
    if (sellSubAction == 2) txobj.pushKV("action", "update");
    if (sellSubAction == 3) txobj.pushKV("action", "cancel");
}

void populateRPCTypeMetaDExTrade(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    bool propertyIdForSaleIsDivisible = isPropertyDivisible(omniObj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(metaObj.getDesProperty());
    std::string unitPriceStr = metaObj.displayFullUnitPrice();

    // populate
    txobj.pushKV("propertyidforsale", (uint64_t)omniObj.getProperty());
    txobj.pushKV("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible);
    txobj.pushKV("amountforsale", FormatMP(omniObj.getProperty(), omniObj.getAmount()));
    txobj.pushKV("propertyiddesired", (uint64_t)metaObj.getDesProperty());
    txobj.pushKV("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible);
    txobj.pushKV("amountdesired", FormatMP(metaObj.getDesProperty(), metaObj.getAmountDesired()));
    txobj.pushKV("unitprice", unitPriceStr);
    if (extendedDetails) populateRPCExtendedTypeMetaDExTrade(omniObj.getHash(), omniObj.getProperty(), omniObj.getAmount(), txobj);
}

void populateRPCTypeMetaDExCancelPrice(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    bool propertyIdForSaleIsDivisible = isPropertyDivisible(omniObj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(metaObj.getDesProperty());
    std::string unitPriceStr = metaObj.displayFullUnitPrice();

    // populate
    txobj.pushKV("propertyidforsale", (uint64_t)omniObj.getProperty());
    txobj.pushKV("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible);
    txobj.pushKV("amountforsale", FormatMP(omniObj.getProperty(), omniObj.getAmount()));
    txobj.pushKV("propertyiddesired", (uint64_t)metaObj.getDesProperty());
    txobj.pushKV("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible);
    txobj.pushKV("amountdesired", FormatMP(metaObj.getDesProperty(), metaObj.getAmountDesired()));
    txobj.pushKV("unitprice", unitPriceStr);
    if (extendedDetails) populateRPCExtendedTypeMetaDExCancel(omniObj.getHash(), txobj);
}

void populateRPCTypeMetaDExCancelPair(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    // populate
    txobj.pushKV("propertyidforsale", (uint64_t)omniObj.getProperty());
    txobj.pushKV("propertyiddesired", (uint64_t)metaObj.getDesProperty());
    if (extendedDetails) populateRPCExtendedTypeMetaDExCancel(omniObj.getHash(), txobj);
}

void populateRPCTypeMetaDExCancelEcosystem(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails)
{
    txobj.pushKV("ecosystem", strEcosystem(omniObj.getEcosystem()));
    if (extendedDetails) populateRPCExtendedTypeMetaDExCancel(omniObj.getHash(), txobj);
}

void populateRPCTypeAcceptOffer(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    int64_t amount = omniObj.getAmount();

    // Check levelDB to see if the amount accepted has been amended due to over accepting amount available
    // TODO: DEx phase 1 really needs an overhaul to work like MetaDEx with original amounts for sale and amounts remaining etc
    int tmpblock = 0;
    uint32_t tmptype = 0;
    uint64_t amountNew = 0;

    LOCK(cs_tally);
    bool tmpValid = pDbTransactionList->getValidMPTX(omniObj.getHash(), &tmpblock, &tmptype, &amountNew);
    if (tmpValid && amountNew > 0) amount = amountNew;

    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("divisible", isPropertyDivisible(propertyId));
    txobj.pushKV("amount", FormatMP(propertyId, amount));
}

void populateRPCTypeCreatePropertyFixed(CMPTransaction& omniObj, UniValue& txobj, int confirmations)
{
    LOCK(cs_tally);
    if (confirmations > 0) {
        uint32_t propertyId = pDbSpInfo->findSPByTX(omniObj.getHash());
        if (propertyId > 0) {
            txobj.pushKV("propertyid", (uint64_t) propertyId);
            txobj.pushKV("divisible", isPropertyDivisible(propertyId));
        }
    }
    txobj.pushKV("ecosystem", strEcosystem(omniObj.getEcosystem()));
    txobj.pushKV("propertytype", strPropertyType(omniObj.getPropertyType()));
    txobj.pushKV("category", omniObj.getSPCategory());
    txobj.pushKV("subcategory", omniObj.getSPSubCategory());
    txobj.pushKV("propertyname", omniObj.getSPName());
    txobj.pushKV("data", omniObj.getSPData());
    txobj.pushKV("url", omniObj.getSPUrl());
    std::string strAmount = FormatByType(omniObj.getAmount(), omniObj.getPropertyType());
    txobj.pushKV("amount", strAmount);
}

void populateRPCTypeCreatePropertyVariable(CMPTransaction& omniObj, UniValue& txobj, int confirmations)
{
    LOCK(cs_tally);
    if (confirmations > 0) {
        uint32_t propertyId = pDbSpInfo->findSPByTX(omniObj.getHash());
        if (propertyId > 0) {
            txobj.pushKV("propertyid", (uint64_t) propertyId);
            txobj.pushKV("divisible", isPropertyDivisible(propertyId));
        }
    }
    txobj.pushKV("propertytype", strPropertyType(omniObj.getPropertyType()));
    txobj.pushKV("ecosystem", strEcosystem(omniObj.getEcosystem()));
    txobj.pushKV("category", omniObj.getSPCategory());
    txobj.pushKV("subcategory", omniObj.getSPSubCategory());
    txobj.pushKV("propertyname", omniObj.getSPName());
    txobj.pushKV("data", omniObj.getSPData());
    txobj.pushKV("url", omniObj.getSPUrl());
    txobj.pushKV("propertyiddesired", (uint64_t) omniObj.getProperty());
    std::string strPerUnit = FormatMP(omniObj.getProperty(), omniObj.getAmount());
    txobj.pushKV("tokensperunit", strPerUnit);
    txobj.pushKV("deadline", omniObj.getDeadline());
    txobj.pushKV("earlybonus", omniObj.getEarlyBirdBonus());
    txobj.pushKV("percenttoissuer", omniObj.getIssuerBonus());
    std::string strAmount = FormatByType(0, omniObj.getPropertyType());
    txobj.pushKV("amount", strAmount); // crowdsale token creations don't issue tokens with the create tx
}

void populateRPCTypeCreatePropertyManual(CMPTransaction& omniObj, UniValue& txobj, int confirmations)
{
    LOCK(cs_tally);
    if (confirmations > 0) {
        uint32_t propertyId = pDbSpInfo->findSPByTX(omniObj.getHash());
        if (propertyId > 0) {
            txobj.pushKV("propertyid", (uint64_t) propertyId);
            txobj.pushKV("divisible", isPropertyDivisible(propertyId));
        }
    }
    txobj.pushKV("propertytype", strPropertyType(omniObj.getPropertyType()));
    txobj.pushKV("ecosystem", strEcosystem(omniObj.getEcosystem()));
    txobj.pushKV("category", omniObj.getSPCategory());
    txobj.pushKV("subcategory", omniObj.getSPSubCategory());
    txobj.pushKV("propertyname", omniObj.getSPName());
    txobj.pushKV("data", omniObj.getSPData());
    txobj.pushKV("url", omniObj.getSPUrl());
    std::string strAmount = FormatByType(0, omniObj.getPropertyType());
    txobj.pushKV("amount", strAmount); // managed token creations don't issue tokens with the create tx
}

void populateRPCTypeCloseCrowdsale(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("divisible", isPropertyDivisible(propertyId));
}

void populateRPCTypeGrant(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("divisible", isPropertyDivisible(propertyId));
    txobj.pushKV("amount", FormatMP(propertyId, omniObj.getAmount()));
    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (!pDbSpInfo->getSP(propertyId, sp)) {
            return; // TODO : handle error
        }
    }
    if (sp.unique) {
        populateRPCExtendedTypeGrantNonFungible(omniObj, txobj);
    }
}

void populateRPCTypeRevoke(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("divisible", isPropertyDivisible(propertyId));
    txobj.pushKV("amount", FormatMP(propertyId, omniObj.getAmount()));
}

void populateRPCTypeSendNonFungible(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("tokenstart", omniObj.getNonFungibleTokenStart());
    txobj.pushKV("tokenend", omniObj.getNonFungibleTokenEnd());
    int64_t amount = (omniObj.getNonFungibleTokenEnd() - omniObj.getNonFungibleTokenStart()) + 1;
    txobj.pushKV("amount", amount);
}

void populateRPCTypeSetNonFungibleData(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("tokenid", omniObj.getNonFungibleTokenStart());
    txobj.pushKV("issuer", omniObj.getNonFungibleDataType() == 1 ? "true" : "false");
    txobj.pushKV("data", omniObj.getNonFungibleData());
}

void populateRPCTypeChangeIssuer(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.pushKV("propertyid", (uint64_t)propertyId);
    txobj.pushKV("divisible", isPropertyDivisible(propertyId));
}

void populateRPCTypeActivation(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.pushKV("featureid", (uint64_t) omniObj.getFeatureId());
    txobj.pushKV("activationblock", (uint64_t) omniObj.getActivationBlock());
    txobj.pushKV("minimumversion", (uint64_t) omniObj.getMinClientVersion());
}

void populateRPCTypeEnableFreezing(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.pushKV("propertyid", (uint64_t) omniObj.getProperty());
}

void populateRPCTypeDisableFreezing(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.pushKV("propertyid", (uint64_t) omniObj.getProperty());
}
void populateRPCTypeFreezeTokens(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.pushKV("propertyid", (uint64_t) omniObj.getProperty());
}

void populateRPCTypeUnfreezeTokens(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.pushKV("propertyid", (uint64_t) omniObj.getProperty());
}

void populateRPCTypeAddDelegate(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.pushKV("propertyid", (uint64_t) omniObj.getProperty());
}

void populateRPCTypeRemoveDelegate(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.pushKV("propertyid", (uint64_t) omniObj.getProperty());
}

void populateRPCTypeAnyData(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.pushKV("data", omniObj.getPayloadData());
}

void populateRPCExtendedTypeSendToOwners(const uint256 txid, std::string extendedDetailsFilter, UniValue& txobj, uint16_t version, interfaces::Wallet *iWallet)
{
    UniValue receiveArray(UniValue::VARR);
    uint64_t tmpAmount = 0, stoFee = 0, numRecipients = 0;
    LOCK(cs_tally);
    pDbStoList->getRecipients(txid, extendedDetailsFilter, &receiveArray, &tmpAmount, &numRecipients, iWallet);
    if (version == MP_TX_PKT_V0) {
        stoFee = numRecipients * TRANSFER_FEE_PER_OWNER;
    } else {
        stoFee = numRecipients * TRANSFER_FEE_PER_OWNER_V1;
    }
    txobj.pushKV("totalstofee", FormatDivisibleMP(stoFee)); // fee always OMNI so always divisible
    txobj.pushKV("recipients", receiveArray);
}

void populateRPCExtendedTypeGrantNonFungible(CMPTransaction& omniObj, UniValue& txobj)
{
    LOCK(cs_tally);
    std::pair<int64_t,int64_t> grantedRange = pDbTransactionList->GetNonFungibleGrant(omniObj.getHash());
    txobj.pushKV("tokenstart", FormatIndivisibleMP(grantedRange.first));
    txobj.pushKV("tokenend", FormatIndivisibleMP(grantedRange.second));
    txobj.pushKV("grantdata", omniObj.getNonFungibleData());
}

void populateRPCExtendedTypeMetaDExTrade(const uint256& txid, uint32_t propertyIdForSale, int64_t amountForSale, UniValue& txobj)
{
    UniValue tradeArray(UniValue::VARR);
    int64_t totalReceived = 0, totalSold = 0;
    LOCK(cs_tally);
    pDbTradeList->getMatchingTrades(txid, propertyIdForSale, tradeArray, totalSold, totalReceived);
    int tradeStatus = MetaDEx_getStatus(txid, propertyIdForSale, amountForSale, totalSold);
    if (tradeStatus == TRADE_OPEN || tradeStatus == TRADE_OPEN_PART_FILLED) {
        const CMPMetaDEx* tradeObj = MetaDEx_RetrieveTrade(txid);
        if (tradeObj != nullptr) {
            txobj.pushKV("amountremaining", FormatMP(tradeObj->getProperty(), tradeObj->getAmountRemaining()));
            txobj.pushKV("amounttofill", FormatMP(tradeObj->getDesProperty(), tradeObj->getAmountToFill()));
        }
    }
    txobj.pushKV("status", MetaDEx_getStatusText(tradeStatus));
    if (tradeStatus == TRADE_CANCELLED || tradeStatus == TRADE_CANCELLED_PART_FILLED) {
        txobj.pushKV("canceltxid", pDbTransactionList->findMetaDExCancel(txid).GetHex());
    }
    txobj.pushKV("matches", tradeArray);
}

void populateRPCExtendedTypeMetaDExCancel(const uint256& txid, UniValue& txobj)
{
    UniValue cancelArray(UniValue::VARR);
    LOCK(cs_tally);
    int numberOfCancels = pDbTransactionList->getNumberOfMetaDExCancels(txid);
    if (0<numberOfCancels) {
        for(int refNumber = 1; refNumber <= numberOfCancels; refNumber++) {
            UniValue cancelTx(UniValue::VOBJ);
            std::string strValue = pDbTransactionList->getKeyValue(txid.ToString() + "-C" + strprintf("%d",refNumber));
            if (strValue.empty()) continue;
            std::vector<std::string> vstr;
            boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
            if (vstr.size() != 3) {
                PrintToLog("TXListDB Error - trade cancel number of tokens is not as expected (%s)\n", strValue);
                continue;
            }
            uint32_t propId = boost::lexical_cast<uint32_t>(vstr[1]);
            int64_t amountUnreserved = boost::lexical_cast<int64_t>(vstr[2]);
            cancelTx.pushKV("txid", vstr[0]);
            cancelTx.pushKV("propertyid", (uint64_t) propId);
            cancelTx.pushKV("amountunreserved", FormatMP(propId, amountUnreserved));
            cancelArray.push_back(cancelTx);
        }
    }
    txobj.pushKV("cancelledtransactions", cancelArray);
}

/* Function to enumerate sub sends for a given txid and add to supplied JSON array
 * Note: this function exists as send all has the potential to carry multiple sends in a single transaction.
 */
int populateRPCSendAllSubSends(const uint256& txid, UniValue& subSends)
{
    int numberOfSubSends = 0;
    {
        LOCK(cs_tally);
        numberOfSubSends = pDbTransactionList->getNumberOfSubRecords(txid);
    }
    if (numberOfSubSends <= 0) {
        PrintToLog("TXLISTDB Error: Transaction %s parsed as a send all but could not locate sub sends in txlistdb.\n", txid.GetHex());
        return -1;
    }
    for (int subSend = 1; subSend <= numberOfSubSends; subSend++) {
        UniValue subSendObj(UniValue::VOBJ);
        uint32_t propertyId;
        int64_t amount;
        {
            LOCK(cs_tally);
            pDbTransactionList->getSendAllDetails(txid, subSend, propertyId, amount);
        }
        subSendObj.pushKV("propertyid", (uint64_t)propertyId);
        subSendObj.pushKV("divisible", isPropertyDivisible(propertyId));
        subSendObj.pushKV("amount", FormatMP(propertyId, amount));
        subSends.push_back(subSendObj);
    }
    return subSends.size();
}

/* Function to enumerate DEx purchases for a given txid and add to supplied JSON array
 * Note: this function exists as it is feasible for a single transaction to carry multiple outputs
 *       and thus make multiple purchases from a single transaction
 */
int populateRPCDExPurchases(const CTransaction& wtx, UniValue& purchases, std::string filterAddress, interfaces::Wallet *iWallet)
{
    int numberOfPurchases = 0;
    {
        LOCK(cs_tally);
        numberOfPurchases = pDbTransactionList->getNumberOfSubRecords(wtx.GetHash());
    }
    if (numberOfPurchases <= 0) {
        PrintToLog("TXLISTDB Error: Transaction %s parsed as a DEx payment but could not locate purchases in txlistdb.\n", wtx.GetHash().GetHex());
        return -1;
    }
    for (int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++) {
        UniValue purchaseObj(UniValue::VOBJ);
        std::string buyer, seller;
        uint64_t vout, nValue, propertyId;
        {
            LOCK(cs_tally);
            pDbTransactionList->getPurchaseDetails(wtx.GetHash(), purchaseNumber, &buyer, &seller, &vout, &propertyId, &nValue);
        }
        if (!filterAddress.empty() && buyer != filterAddress && seller != filterAddress) continue; // filter requested & doesn't match
        bool bIsMine = false;
        if (IsMyAddress(buyer, iWallet) || IsMyAddress(seller, iWallet)) bIsMine = true;
        int64_t amountPaid = wtx.vout[vout].nValue;
        purchaseObj.pushKV("vout", vout);
        purchaseObj.pushKV("amountpaid", FormatDivisibleMP(amountPaid));
        purchaseObj.pushKV("ismine", bIsMine);
        purchaseObj.pushKV("referenceaddress", seller);
        purchaseObj.pushKV("propertyid", propertyId);
        purchaseObj.pushKV("amountbought", FormatMP(propertyId, nValue));
        purchaseObj.pushKV("valid", true); //only valid purchases are stored, anything else is regular BTC tx
        purchases.push_back(purchaseObj);
    }
    return purchases.size();
}
