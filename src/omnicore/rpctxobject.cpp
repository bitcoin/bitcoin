/**
 * @file rpctxobject.cpp
 *
 * Handler for populating RPC transaction objects.
 */

#include "omnicore/rpctxobject.h"

#include "omnicore/dex.h"
#include "omnicore/errors.h"
#include "omnicore/mdex.h"
#include "omnicore/omnicore.h"
#include "omnicore/pending.h"
#include "omnicore/rpctxobject.h"
#include "omnicore/sp.h"
#include "omnicore/sto.h"
#include "omnicore/tx.h"
#include "omnicore/utilsbitcoin.h"
#include "omnicore/wallettxs.h"

#include "chainparams.h"
#include "main.h"
#include "primitives/transaction.h"
#include "sync.h"
#include "uint256.h"

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
int populateRPCTransactionObject(const uint256& txid, UniValue& txobj, std::string filterAddress, bool extendedDetails, std::string extendedDetailsFilter)
{
    // retrieve the transaction from the blockchain and obtain it's height/confs/time
    CTransaction tx;
    uint256 blockHash;
    if (!GetTransaction(txid, tx, Params().GetConsensus(), blockHash, true)) {
        return MP_TX_NOT_FOUND;
    }

    return populateRPCTransactionObject(tx, blockHash, txobj, filterAddress, extendedDetails, extendedDetailsFilter);
}

int populateRPCTransactionObject(const CTransaction& tx, const uint256& blockHash, UniValue& txobj, std::string filterAddress, bool extendedDetails, std::string extendedDetailsFilter, int blockHeight)
{
    int confirmations = 0;
    int64_t blockTime = 0;
    int positionInBlock = 0;

    if (blockHeight == 0) {
        blockHeight = GetHeight();
    }

    if (!blockHash.IsNull()) {
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (NULL != pBlockIndex) {
            confirmations = 1 + blockHeight - pBlockIndex->nHeight;
            blockTime = pBlockIndex->nTime;
            blockHeight = pBlockIndex->nHeight;
        }
    }

    // attempt to parse the transaction
    CMPTransaction mp_obj;
    int parseRC = ParseTransaction(tx, blockHeight, 0, mp_obj, blockTime);
    if (parseRC < 0) return MP_TX_IS_NOT_MASTER_PROTOCOL;

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
            p_txlistdb->getPurchaseDetails(txid, 1, &tmpBuyer, &tmpSeller, &tmpVout, &tmpPropertyId, &tmpNValue);
        }
        UniValue purchases(UniValue::VARR);
        if (populateRPCDExPurchases(tx, purchases, filterAddress) <= 0) return -1;
        txobj.push_back(Pair("txid", txid.GetHex()));
        txobj.push_back(Pair("type", "DEx Purchase"));
        txobj.push_back(Pair("sendingaddress", tmpBuyer));
        txobj.push_back(Pair("purchases", purchases));
        txobj.push_back(Pair("blockhash", blockHash.GetHex()));
        txobj.push_back(Pair("blocktime", blockTime));
        txobj.push_back(Pair("block", blockHeight));
        txobj.push_back(Pair("confirmations", confirmations));
        return 0;
    }

    // check if we're filtering from listtransactions_MP, and if so whether we have a non-match we want to skip
    if (!filterAddress.empty() && mp_obj.getSender() != filterAddress && mp_obj.getReceiver() != filterAddress) return -1;

    // parse packet and populate mp_obj
    if (!mp_obj.interpret_Transaction()) return MP_TX_IS_NOT_MASTER_PROTOCOL;

    // obtain validity - only confirmed transactions can be valid
    bool valid = false;
    if (confirmations > 0) {
        LOCK(cs_tally);
        valid = getValidMPTX(txid);
        positionInBlock = p_OmniTXDB->FetchTransactionPosition(txid);
    }

    // populate some initial info for the transaction
    bool fMine = false;
    if (IsMyAddress(mp_obj.getSender()) || IsMyAddress(mp_obj.getReceiver())) fMine = true;
    txobj.push_back(Pair("txid", txid.GetHex()));
    txobj.push_back(Pair("fee", FormatDivisibleMP(mp_obj.getFeePaid())));
    txobj.push_back(Pair("sendingaddress", mp_obj.getSender()));
    if (showRefForTx(mp_obj.getType())) txobj.push_back(Pair("referenceaddress", mp_obj.getReceiver()));
    txobj.push_back(Pair("ismine", fMine));
    txobj.push_back(Pair("version", (uint64_t)mp_obj.getVersion()));
    txobj.push_back(Pair("type_int", (uint64_t)mp_obj.getType()));
    if (mp_obj.getType() != MSC_TYPE_SIMPLE_SEND) { // Type 0 will add "Type" attribute during populateRPCTypeSimpleSend
        txobj.push_back(Pair("type", mp_obj.getTypeString()));
    }

    // populate type specific info and extended details if requested
    // extended details are not available for unconfirmed transactions
    if (confirmations <= 0) extendedDetails = false;
    populateRPCTypeInfo(mp_obj, txobj, mp_obj.getType(), extendedDetails, extendedDetailsFilter, confirmations);

    // state and chain related information
    if (confirmations != 0 && !blockHash.IsNull()) {
        txobj.push_back(Pair("valid", valid));
        if (!valid) {
            txobj.push_back(Pair("invalidreason", p_OmniTXDB->FetchInvalidReason(txid)));
        }
        txobj.push_back(Pair("blockhash", blockHash.GetHex()));
        txobj.push_back(Pair("blocktime", blockTime));
        txobj.push_back(Pair("positioninblock", positionInBlock));
    }
    if (confirmations != 0) {
        txobj.push_back(Pair("block", blockHeight));
    }
    txobj.push_back(Pair("confirmations", confirmations));

    // finished
    return 0;
}

/* Function to call respective populators based on message type
 */
void populateRPCTypeInfo(CMPTransaction& mp_obj, UniValue& txobj, uint32_t txType, bool extendedDetails, std::string extendedDetailsFilter, int confirmations)
{
    switch (txType) {
        case MSC_TYPE_SIMPLE_SEND:
            populateRPCTypeSimpleSend(mp_obj, txobj);
            break;
        case MSC_TYPE_SEND_TO_OWNERS:
            populateRPCTypeSendToOwners(mp_obj, txobj, extendedDetails, extendedDetailsFilter);
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
        if (false == _my_sps->getSP(crowdPropertyId, sp)) {
            PrintToLog("SP Error: Crowdsale purchase for non-existent property %d in transaction %s", crowdPropertyId, omniObj.getHash().GetHex());
            return;
        }
        txobj.push_back(Pair("type", "Crowdsale Purchase"));
        txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
        txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
        txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
        txobj.push_back(Pair("purchasedpropertyid", crowdPropertyId));
        txobj.push_back(Pair("purchasedpropertyname", sp.name));
        txobj.push_back(Pair("purchasedpropertydivisible", sp.isDivisible()));
        txobj.push_back(Pair("purchasedtokens", FormatMP(crowdPropertyId, crowdTokens)));
        txobj.push_back(Pair("issuertokens", FormatMP(crowdPropertyId, issuerTokens)));
    } else {
        txobj.push_back(Pair("type", "Simple Send"));
        txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
        txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
        txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
    }
}

void populateRPCTypeSendToOwners(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails, std::string extendedDetailsFilter)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
    if (extendedDetails) populateRPCExtendedTypeSendToOwners(omniObj.getHash(), extendedDetailsFilter, txobj, omniObj.getVersion());
}

void populateRPCTypeSendAll(CMPTransaction& omniObj, UniValue& txobj, int confirmations)
{
    UniValue subSends(UniValue::VARR);
    if (omniObj.getEcosystem() == 1) txobj.push_back(Pair("ecosystem", "main"));
    if (omniObj.getEcosystem() == 2) txobj.push_back(Pair("ecosystem", "test"));
    if (confirmations > 0) {
        if (populateRPCSendAllSubSends(omniObj.getHash(), subSends) > 0) txobj.push_back(Pair("subsends", subSends));
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
        bool tmpValid = getValidMPTX(omniObj.getHash(), &tmpblock, &tmptype, &amountNew);
        if (tmpValid && amountNew > 0) {
            amountDesired = calculateDesiredBTC(amountOffered, amountDesired, amountNew);
            amountOffered = amountNew;
        }
    }

    // Populate
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, amountOffered)));
    txobj.push_back(Pair("bitcoindesired", FormatDivisibleMP(amountDesired)));
    txobj.push_back(Pair("timelimit",  temp_offer.getBlockTimeLimit()));
    txobj.push_back(Pair("feerequired", FormatDivisibleMP(temp_offer.getMinFee())));
    if (sellSubAction == 1) txobj.push_back(Pair("action", "new"));
    if (sellSubAction == 2) txobj.push_back(Pair("action", "update"));
    if (sellSubAction == 3) txobj.push_back(Pair("action", "cancel"));
}

void populateRPCTypeMetaDExTrade(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    bool propertyIdForSaleIsDivisible = isPropertyDivisible(omniObj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(metaObj.getDesProperty());
    std::string unitPriceStr = metaObj.displayFullUnitPrice();

    // populate
    txobj.push_back(Pair("propertyidforsale", (uint64_t)omniObj.getProperty()));
    txobj.push_back(Pair("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible));
    txobj.push_back(Pair("amountforsale", FormatMP(omniObj.getProperty(), omniObj.getAmount())));
    txobj.push_back(Pair("propertyiddesired", (uint64_t)metaObj.getDesProperty()));
    txobj.push_back(Pair("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible));
    txobj.push_back(Pair("amountdesired", FormatMP(metaObj.getDesProperty(), metaObj.getAmountDesired())));
    txobj.push_back(Pair("unitprice", unitPriceStr));
    if (extendedDetails) populateRPCExtendedTypeMetaDExTrade(omniObj.getHash(), omniObj.getProperty(), omniObj.getAmount(), txobj);
}

void populateRPCTypeMetaDExCancelPrice(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    bool propertyIdForSaleIsDivisible = isPropertyDivisible(omniObj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(metaObj.getDesProperty());
    std::string unitPriceStr = metaObj.displayFullUnitPrice();

    // populate
    txobj.push_back(Pair("propertyidforsale", (uint64_t)omniObj.getProperty()));
    txobj.push_back(Pair("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible));
    txobj.push_back(Pair("amountforsale", FormatMP(omniObj.getProperty(), omniObj.getAmount())));
    txobj.push_back(Pair("propertyiddesired", (uint64_t)metaObj.getDesProperty()));
    txobj.push_back(Pair("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible));
    txobj.push_back(Pair("amountdesired", FormatMP(metaObj.getDesProperty(), metaObj.getAmountDesired())));
    txobj.push_back(Pair("unitprice", unitPriceStr));
    if (extendedDetails) populateRPCExtendedTypeMetaDExCancel(omniObj.getHash(), txobj);
}

void populateRPCTypeMetaDExCancelPair(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    // populate
    txobj.push_back(Pair("propertyidforsale", (uint64_t)omniObj.getProperty()));
    txobj.push_back(Pair("propertyiddesired", (uint64_t)metaObj.getDesProperty()));
    if (extendedDetails) populateRPCExtendedTypeMetaDExCancel(omniObj.getHash(), txobj);
}

void populateRPCTypeMetaDExCancelEcosystem(CMPTransaction& omniObj, UniValue& txobj, bool extendedDetails)
{
    txobj.push_back(Pair("ecosystem", strEcosystem(omniObj.getEcosystem())));
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
    bool tmpValid = getValidMPTX(omniObj.getHash(), &tmpblock, &tmptype, &amountNew);
    if (tmpValid && amountNew > 0) amount = amountNew;

    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, amount)));
}

void populateRPCTypeCreatePropertyFixed(CMPTransaction& omniObj, UniValue& txobj, int confirmations)
{
    LOCK(cs_tally);
    if (confirmations > 0) {
        uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());
        if (propertyId > 0) {
            txobj.push_back(Pair("propertyid", (uint64_t) propertyId));
            txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
        }
    }
    txobj.push_back(Pair("ecosystem", strEcosystem(omniObj.getEcosystem())));
    txobj.push_back(Pair("propertytype", strPropertyType(omniObj.getPropertyType())));
    txobj.push_back(Pair("category", omniObj.getSPCategory()));
    txobj.push_back(Pair("subcategory", omniObj.getSPSubCategory()));
    txobj.push_back(Pair("propertyname", omniObj.getSPName()));
    txobj.push_back(Pair("data", omniObj.getSPData()));
    txobj.push_back(Pair("url", omniObj.getSPUrl()));
    std::string strAmount = FormatByType(omniObj.getAmount(), omniObj.getPropertyType());
    txobj.push_back(Pair("amount", strAmount));
}

void populateRPCTypeCreatePropertyVariable(CMPTransaction& omniObj, UniValue& txobj, int confirmations)
{
    LOCK(cs_tally);
    if (confirmations > 0) {
        uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());
        if (propertyId > 0) {
            txobj.push_back(Pair("propertyid", (uint64_t) propertyId));
            txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
        }
    }
    txobj.push_back(Pair("propertytype", strPropertyType(omniObj.getPropertyType())));
    txobj.push_back(Pair("ecosystem", strEcosystem(omniObj.getEcosystem())));
    txobj.push_back(Pair("category", omniObj.getSPCategory()));
    txobj.push_back(Pair("subcategory", omniObj.getSPSubCategory()));
    txobj.push_back(Pair("propertyname", omniObj.getSPName()));
    txobj.push_back(Pair("data", omniObj.getSPData()));
    txobj.push_back(Pair("url", omniObj.getSPUrl()));
    txobj.push_back(Pair("propertyiddesired", (uint64_t) omniObj.getProperty()));
    std::string strPerUnit = FormatMP(omniObj.getProperty(), omniObj.getAmount());
    txobj.push_back(Pair("tokensperunit", strPerUnit));
    txobj.push_back(Pair("deadline", omniObj.getDeadline()));
    txobj.push_back(Pair("earlybonus", omniObj.getEarlyBirdBonus()));
    txobj.push_back(Pair("percenttoissuer", omniObj.getIssuerBonus()));
    std::string strAmount = FormatByType(0, omniObj.getPropertyType());
    txobj.push_back(Pair("amount", strAmount)); // crowdsale token creations don't issue tokens with the create tx
}

void populateRPCTypeCreatePropertyManual(CMPTransaction& omniObj, UniValue& txobj, int confirmations)
{
    LOCK(cs_tally);
    if (confirmations > 0) {
        uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());
        if (propertyId > 0) {
            txobj.push_back(Pair("propertyid", (uint64_t) propertyId));
            txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
        }
    }
    txobj.push_back(Pair("propertytype", strPropertyType(omniObj.getPropertyType())));
    txobj.push_back(Pair("ecosystem", strEcosystem(omniObj.getEcosystem())));
    txobj.push_back(Pair("category", omniObj.getSPCategory()));
    txobj.push_back(Pair("subcategory", omniObj.getSPSubCategory()));
    txobj.push_back(Pair("propertyname", omniObj.getSPName()));
    txobj.push_back(Pair("data", omniObj.getSPData()));
    txobj.push_back(Pair("url", omniObj.getSPUrl()));
    std::string strAmount = FormatByType(0, omniObj.getPropertyType());
    txobj.push_back(Pair("amount", strAmount)); // managed token creations don't issue tokens with the create tx
}

void populateRPCTypeCloseCrowdsale(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
}

void populateRPCTypeGrant(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
}

void populateRPCTypeRevoke(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
}

void populateRPCTypeChangeIssuer(CMPTransaction& omniObj, UniValue& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
}

void populateRPCTypeActivation(CMPTransaction& omniObj, UniValue& txobj)
{
    txobj.push_back(Pair("featureid", (uint64_t) omniObj.getFeatureId()));
    txobj.push_back(Pair("activationblock", (uint64_t) omniObj.getActivationBlock()));
    txobj.push_back(Pair("minimumversion", (uint64_t) omniObj.getMinClientVersion()));
}

void populateRPCExtendedTypeSendToOwners(const uint256 txid, std::string extendedDetailsFilter, UniValue& txobj, uint16_t version)
{
    UniValue receiveArray(UniValue::VARR);
    uint64_t tmpAmount = 0, stoFee = 0, numRecipients = 0;
    LOCK(cs_tally);
    s_stolistdb->getRecipients(txid, extendedDetailsFilter, &receiveArray, &tmpAmount, &numRecipients);
    if (version == MP_TX_PKT_V0) {
        stoFee = numRecipients * TRANSFER_FEE_PER_OWNER;
    } else {
        stoFee = numRecipients * TRANSFER_FEE_PER_OWNER_V1;
    }
    txobj.push_back(Pair("totalstofee", FormatDivisibleMP(stoFee))); // fee always OMNI so always divisible
    txobj.push_back(Pair("recipients", receiveArray));
}

void populateRPCExtendedTypeMetaDExTrade(const uint256& txid, uint32_t propertyIdForSale, int64_t amountForSale, UniValue& txobj)
{
    UniValue tradeArray(UniValue::VARR);
    int64_t totalReceived = 0, totalSold = 0;
    LOCK(cs_tally);
    t_tradelistdb->getMatchingTrades(txid, propertyIdForSale, tradeArray, totalSold, totalReceived);
    int tradeStatus = MetaDEx_getStatus(txid, propertyIdForSale, amountForSale, totalSold);
    if (tradeStatus == TRADE_OPEN || tradeStatus == TRADE_OPEN_PART_FILLED) {
        const CMPMetaDEx* tradeObj = MetaDEx_RetrieveTrade(txid);
        if (tradeObj != NULL) {
            txobj.push_back(Pair("amountremaining", FormatMP(tradeObj->getProperty(), tradeObj->getAmountRemaining())));
            txobj.push_back(Pair("amounttofill", FormatMP(tradeObj->getDesProperty(), tradeObj->getAmountToFill())));
        }
    }
    txobj.push_back(Pair("status", MetaDEx_getStatusText(tradeStatus)));
    if (tradeStatus == TRADE_CANCELLED || tradeStatus == TRADE_CANCELLED_PART_FILLED) {
        txobj.push_back(Pair("canceltxid", p_txlistdb->findMetaDExCancel(txid).GetHex()));
    }
    txobj.push_back(Pair("matches", tradeArray));
}

void populateRPCExtendedTypeMetaDExCancel(const uint256& txid, UniValue& txobj)
{
    UniValue cancelArray(UniValue::VARR);
    LOCK(cs_tally);
    int numberOfCancels = p_txlistdb->getNumberOfMetaDExCancels(txid);
    if (0<numberOfCancels) {
        for(int refNumber = 1; refNumber <= numberOfCancels; refNumber++) {
            UniValue cancelTx(UniValue::VOBJ);
            std::string strValue = p_txlistdb->getKeyValue(txid.ToString() + "-C" + strprintf("%d",refNumber));
            if (strValue.empty()) continue;
            std::vector<std::string> vstr;
            boost::split(vstr, strValue, boost::is_any_of(":"), boost::token_compress_on);
            if (vstr.size() != 3) {
                PrintToLog("TXListDB Error - trade cancel number of tokens is not as expected (%s)\n", strValue);
                continue;
            }
            uint32_t propId = boost::lexical_cast<uint32_t>(vstr[1]);
            int64_t amountUnreserved = boost::lexical_cast<int64_t>(vstr[2]);
            cancelTx.push_back(Pair("txid", vstr[0]));
            cancelTx.push_back(Pair("propertyid", (uint64_t) propId));
            cancelTx.push_back(Pair("amountunreserved", FormatMP(propId, amountUnreserved)));
            cancelArray.push_back(cancelTx);
        }
    }
    txobj.push_back(Pair("cancelledtransactions", cancelArray));
}

/* Function to enumerate sub sends for a given txid and add to supplied JSON array
 * Note: this function exists as send all has the potential to carry multiple sends in a single transaction.
 */
int populateRPCSendAllSubSends(const uint256& txid, UniValue& subSends)
{
    int numberOfSubSends = 0;
    {
        LOCK(cs_tally);
        numberOfSubSends = p_txlistdb->getNumberOfSubRecords(txid);
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
            p_txlistdb->getSendAllDetails(txid, subSend, propertyId, amount);
        }
        subSendObj.push_back(Pair("propertyid", (uint64_t)propertyId));
        subSendObj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
        subSendObj.push_back(Pair("amount", FormatMP(propertyId, amount)));
        subSends.push_back(subSendObj);
    }
    return subSends.size();
}

/* Function to enumerate DEx purchases for a given txid and add to supplied JSON array
 * Note: this function exists as it is feasible for a single transaction to carry multiple outputs
 *       and thus make multiple purchases from a single transaction
 */
int populateRPCDExPurchases(const CTransaction& wtx, UniValue& purchases, std::string filterAddress)
{
    int numberOfPurchases = 0;
    {
        LOCK(cs_tally);
        numberOfPurchases = p_txlistdb->getNumberOfSubRecords(wtx.GetHash());
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
            p_txlistdb->getPurchaseDetails(wtx.GetHash(), purchaseNumber, &buyer, &seller, &vout, &propertyId, &nValue);
        }
        if (!filterAddress.empty() && buyer != filterAddress && seller != filterAddress) continue; // filter requested & doesn't match
        bool bIsMine = false;
        if (IsMyAddress(buyer) || IsMyAddress(seller)) bIsMine = true;
        int64_t amountPaid = wtx.vout[vout].nValue;
        purchaseObj.push_back(Pair("vout", vout));
        purchaseObj.push_back(Pair("amountpaid", FormatDivisibleMP(amountPaid)));
        purchaseObj.push_back(Pair("ismine", bIsMine));
        purchaseObj.push_back(Pair("referenceaddress", seller));
        purchaseObj.push_back(Pair("propertyid", propertyId));
        purchaseObj.push_back(Pair("amountbought", FormatDivisibleMP(nValue)));
        purchaseObj.push_back(Pair("valid", true)); //only valid purchases are stored, anything else is regular BTC tx
        purchases.push_back(purchaseObj);
    }
    return purchases.size();
}
