// Handler for populating RPC transaction objects
#include "omnicore/rpctxobject.h"

// Bitcoin Core includes
#include "wallet.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader_template.h"

// Omni Core includes
#include "omnicore/rpctxobject.h"
#include "omnicore/omnicore.h"
#include "omnicore/dex.h"
#include "omnicore/mdex.h"
#include "omnicore/errors.h"
#include "omnicore/sp.h"
#include "omnicore/tx.h"
#include "omnicore/pending.h"
#include "omnicore/utilsbitcoin.h"

// Boost includes
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

// Generic includes
#include <string>

// Namespaces
using namespace json_spirit;
using namespace mastercore;

/**
 * Function to standardize RPC output for transactions into a JSON object in either basic or extended mode.
 *
 * Use basic mode for generic calls (e.g. omni_gettransaction/omni_listtransaction etc.)
 * Use extended mode for transaction specific calls (e.g. omni_getsto, omni_gettrade etc.)
 *
 * DEx payments and the extended mode are only available for confirmed transactions.
 */
int populateRPCTransactionObject(const uint256& txid, Object& txobj, std::string filterAddress, bool extendedDetails, std::string extendedDetailsFilter)
{
    // retrieve the transaction from the blockchain and obtain it's height/confs/time
    CTransaction wtx;
    uint256 blockHash;
    if (!GetTransaction(txid, wtx, blockHash, true)) return MP_TX_NOT_FOUND;

    if (0 == blockHash) {
        // is it one of our pending transactions?  if so return pending description
        LOCK(cs_pending);
        PendingMap::iterator it = my_pending.find(txid);
        if (it != my_pending.end()) {
            const CMPPending& pending = it->second;
            if (!filterAddress.empty() && pending.src != filterAddress) return -1;
            Value tempVal;
            json_spirit::read_string(pending.desc, tempVal);
            txobj = tempVal.get_obj();
            return 0;
        }
    }

    int confirmations = 0;
    int64_t blockTime = 0;
    int blockHeight = GetHeight();

    if (blockHash != 0) {
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (NULL != pBlockIndex) {
            confirmations = 1 + blockHeight - pBlockIndex->nHeight;
            blockTime = pBlockIndex->nTime;
            blockHeight = pBlockIndex->nHeight;
        }
    }

    // attempt to parse the transaction
    CMPTransaction mp_obj;
    int parseRC = ParseTransaction(wtx, blockHeight, 0, mp_obj, blockTime);
    if (parseRC < 0) return MP_TX_IS_NOT_MASTER_PROTOCOL;

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
        Array purchases;
        if (populateRPCDExPurchases(wtx, purchases, filterAddress) <= 0) return -1;
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
    txobj.push_back(Pair("type", mp_obj.getTypeString()));

    // populate type specific info and extended details if requested
    // extended details are not available for unconfirmed transactions
    if (confirmations <= 0) extendedDetails = false;
    populateRPCTypeInfo(mp_obj, txobj, mp_obj.getType(), extendedDetails, extendedDetailsFilter);

    // state and chain related information
    if (confirmations > 0) txobj.push_back(Pair("valid", valid));
    if (confirmations > 0) txobj.push_back(Pair("blockhash", blockHash.GetHex()));
    if (confirmations > 0) txobj.push_back(Pair("blocktime", blockTime));
    if (confirmations > 0) txobj.push_back(Pair("block", blockHeight));
    txobj.push_back(Pair("confirmations", confirmations));

    // finished
    return 0;
}

/* Function to call respective populators based on message type
 */
void populateRPCTypeInfo(CMPTransaction& mp_obj, Object& txobj, uint32_t txType, bool extendedDetails, std::string extendedDetailsFilter)
{
    switch (txType) {
        case MSC_TYPE_SIMPLE_SEND:
            populateRPCTypeSimpleSend(mp_obj, txobj);
            break;
        case MSC_TYPE_SEND_TO_OWNERS:
            populateRPCTypeSendToOwners(mp_obj, txobj, extendedDetails, extendedDetailsFilter);
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
            populateRPCTypeCreatePropertyFixed(mp_obj, txobj);
            break;
        case MSC_TYPE_CREATE_PROPERTY_VARIABLE:
            populateRPCTypeCreatePropertyVariable(mp_obj, txobj);
            break;
        case MSC_TYPE_CREATE_PROPERTY_MANUAL:
            populateRPCTypeCreatePropertyManual(mp_obj, txobj);
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
    }
    return true; // default to true, shouldn't be needed but just in case
}

void populateRPCTypeSimpleSend(CMPTransaction& omniObj, Object& txobj)
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
        if (!txobj.empty()) txobj.pop_back(); // strip last element (which will be "type:Simple Send") & replace
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
        txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
        txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
        txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
    }
}

void populateRPCTypeSendToOwners(CMPTransaction& omniObj, Object& txobj, bool extendedDetails, std::string extendedDetailsFilter)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
    if (extendedDetails) populateRPCExtendedTypeSendToOwners(omniObj.getHash(), extendedDetailsFilter, txobj);
}

void populateRPCTypeTradeOffer(CMPTransaction& omniObj, Object& txobj)
{
    CMPOffer temp_offer(omniObj);
    uint32_t propertyId = omniObj.getProperty();
    int64_t amount = omniObj.getAmount();

    // NOTE: some manipulation of sell_subaction is needed here
    // TODO: interpretPacket should provide reliable data, cleanup at RPC layer is not cool
    uint8_t sell_subaction = temp_offer.getSubaction();
    if (sell_subaction > 3) sell_subaction = 0; // case where subaction byte >3, to have been allowed must be a v0 sell, flip byte to 0
    if (sell_subaction == 0 && amount > 0) sell_subaction = 1; // case where subaction byte=0, must be a v0 sell, amount > 0 means a new sell
    if (sell_subaction == 0 && amount == 0) sell_subaction = 3; // case where subaction byte=0. must be a v0 sell, amount of 0 means a cancel

    // Check levelDB to see if the amount for sale has been amended due to a partial purchase
    // TODO: DEx phase 1 really needs an overhaul to work like MetaDEx with original amounts for sale and amounts remaining etc
    // TODO: if the amount of the transaction is not used here, then the BTC amount should be recalculated (?)
    int tmpblock = 0;
    unsigned int tmptype = 0;
    uint64_t amountNew = 0;
    LOCK(cs_tally);
    bool tmpValid = getValidMPTX(omniObj.getHash(), &tmpblock, &tmptype, &amountNew);
    if (tmpValid && amountNew > 0) amount = amountNew;

    // Populate
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, amount)));
    txobj.push_back(Pair("bitcoindesired", FormatDivisibleMP(temp_offer.getBTCDesiredOriginal())));
    txobj.push_back(Pair("timelimit",  temp_offer.getBlockTimeLimit()));
    txobj.push_back(Pair("feerequired", FormatDivisibleMP(temp_offer.getMinFee())));
    if (sell_subaction == 1) txobj.push_back(Pair("action", "new"));
    if (sell_subaction == 2) txobj.push_back(Pair("action", "update"));
    if (sell_subaction == 3) txobj.push_back(Pair("action", "cancel"));
}

void populateRPCTypeMetaDExTrade(CMPTransaction& omniObj, Object& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    // unit price display adjustment based on divisibility and always showing prices in MSC/TMSC
    bool propertyIdForSaleIsDivisible = isPropertyDivisible(omniObj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(metaObj.getDesProperty());
    rational_t tempUnitPrice(int128_t(0));
    if ((omniObj.getProperty() == OMNI_PROPERTY_MSC) || (omniObj.getProperty() == OMNI_PROPERTY_TMSC)) {
        tempUnitPrice = metaObj.inversePrice();
        if (!propertyIdDesiredIsDivisible) tempUnitPrice = tempUnitPrice/COIN;
    } else {
        tempUnitPrice = metaObj.unitPrice();
        if (!propertyIdForSaleIsDivisible) tempUnitPrice = tempUnitPrice/COIN;
    }
    std::string unitPriceStr = xToString(tempUnitPrice);

    // populate
    txobj.push_back(Pair("propertyidforsale", (uint64_t)omniObj.getProperty()));
    txobj.push_back(Pair("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible));
    txobj.push_back(Pair("amountforsale", FormatMP(omniObj.getProperty(), omniObj.getAmount())));
    txobj.push_back(Pair("amountremaining", FormatMP(omniObj.getProperty(), metaObj.getAmountRemaining())));
    txobj.push_back(Pair("propertyiddesired", (uint64_t)metaObj.getDesProperty()));
    txobj.push_back(Pair("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible));
    txobj.push_back(Pair("amountdesired", FormatMP(metaObj.getDesProperty(), metaObj.getAmountDesired())));
    txobj.push_back(Pair("amounttofill", FormatMP(metaObj.getDesProperty(), metaObj.getAmountToFill())));
    txobj.push_back(Pair("unitprice", unitPriceStr));
    if (extendedDetails) populateRPCExtendedTypeMetaDExTrade(omniObj.getHash(), omniObj.getProperty(), omniObj.getAmount(), txobj);
}

void populateRPCTypeMetaDExCancelPrice(CMPTransaction& omniObj, Object& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    // unit price display adjustment based on divisibility and always showing prices in MSC/TMSC
    bool propertyIdForSaleIsDivisible = isPropertyDivisible(omniObj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(metaObj.getDesProperty());
    rational_t tempUnitPrice(int128_t(0));
    if ((omniObj.getProperty() == OMNI_PROPERTY_MSC) || (omniObj.getProperty() == OMNI_PROPERTY_TMSC)) {
        tempUnitPrice = metaObj.inversePrice();
        if (!propertyIdDesiredIsDivisible) tempUnitPrice = tempUnitPrice/COIN;
    } else {
        tempUnitPrice = metaObj.unitPrice();
        if (!propertyIdForSaleIsDivisible) tempUnitPrice = tempUnitPrice/COIN;
    }
    std::string unitPriceStr = xToString(tempUnitPrice);

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

void populateRPCTypeMetaDExCancelPair(CMPTransaction& omniObj, Object& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj(omniObj);

    // populate
    txobj.push_back(Pair("propertyidforsale", (uint64_t)omniObj.getProperty()));
    txobj.push_back(Pair("propertyiddesired", (uint64_t)metaObj.getDesProperty()));
    if (extendedDetails) populateRPCExtendedTypeMetaDExCancel(omniObj.getHash(), txobj);
}

void populateRPCTypeMetaDExCancelEcosystem(CMPTransaction& omniObj, Object& txobj, bool extendedDetails)
{
    txobj.push_back(Pair("ecosystem", strEcosystem(omniObj.getEcosystem())));
    if (extendedDetails) populateRPCExtendedTypeMetaDExCancel(omniObj.getHash(), txobj);
}

void populateRPCTypeAcceptOffer(CMPTransaction& omniObj, Object& txobj)
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

void populateRPCTypeCreatePropertyFixed(CMPTransaction& omniObj, Object& txobj)
{
    LOCK(cs_tally);
    uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());
    if (propertyId > 0) txobj.push_back(Pair("propertyid", (uint64_t) propertyId));
    if (propertyId > 0) txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));

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

void populateRPCTypeCreatePropertyVariable(CMPTransaction& omniObj, Object& txobj)
{
    LOCK(cs_tally);
    uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());
    if (propertyId > 0) txobj.push_back(Pair("propertyid", (uint64_t) propertyId));
    if (propertyId > 0) txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));

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

void populateRPCTypeCreatePropertyManual(CMPTransaction& omniObj, Object& txobj)
{
    LOCK(cs_tally);
    uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());
    if (propertyId > 0) txobj.push_back(Pair("propertyid", (uint64_t) propertyId));
    if (propertyId > 0) txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));

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

void populateRPCTypeCloseCrowdsale(CMPTransaction& omniObj, Object& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
}

void populateRPCTypeGrant(CMPTransaction& omniObj, Object& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
}

void populateRPCTypeRevoke(CMPTransaction& omniObj, Object& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
}

void populateRPCTypeChangeIssuer(CMPTransaction& omniObj, Object& txobj)
{
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
}

void populateRPCExtendedTypeSendToOwners(const uint256 txid, std::string extendedDetailsFilter, Object& txobj)
{
    Array receiveArray;
    uint64_t tmpAmount = 0, stoFee = 0;
    LOCK(cs_tally);
    s_stolistdb->getRecipients(txid, extendedDetailsFilter, &receiveArray, &tmpAmount, &stoFee);
    txobj.push_back(Pair("totalstofee", FormatDivisibleMP(stoFee))); // fee always MSC so always divisible
    txobj.push_back(Pair("recipients", receiveArray));
}

void populateRPCExtendedTypeMetaDExTrade(const uint256& txid, uint32_t propertyIdForSale, int64_t amountForSale, Object& txobj)
{
    Array tradeArray;
    int64_t totalReceived = 0, totalSold = 0;
    LOCK(cs_tally);
    t_tradelistdb->getMatchingTrades(txid, propertyIdForSale, tradeArray, totalSold, totalReceived);
    std::string statusText = MetaDEx_getStatus(txid, propertyIdForSale, amountForSale, totalSold, totalReceived);
    txobj.push_back(Pair("status", statusText));
    if (statusText == "cancelled" || statusText == "cancelled part filled") {
        txobj.push_back(Pair("canceltxid", p_txlistdb->findMetaDExCancel(txid).GetHex()));
    }
    txobj.push_back(Pair("matches", tradeArray));
}

void populateRPCExtendedTypeMetaDExCancel(const uint256& txid, Object& txobj)
{
    Array cancelArray;
    LOCK(cs_tally);
    int numberOfCancels = p_txlistdb->getNumberOfMetaDExCancels(txid);
    if (0<numberOfCancels) {
        for(int refNumber = 1; refNumber <= numberOfCancels; refNumber++) {
            Object cancelTx;
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

/* Function to enumerate DEx purchases for a given txid and add to supplied JSON array
 * Note: this function exists as it is feasible for a single transaction to carry multiple outputs
 *       and thus make multiple purchases from a single transaction
 */
int populateRPCDExPurchases(const CTransaction& wtx, Array& purchases, std::string filterAddress)
{
    int numberOfPurchases = 0;
    {
        LOCK(cs_tally);
        numberOfPurchases = p_txlistdb->getNumberOfPurchases(wtx.GetHash());
    }
    if (numberOfPurchases <= 0) {
        PrintToLog("TXLISTDB Error: Transaction %s parsed as a DEx payment but could not locate purchases in txlistdb.\n", wtx.GetHash().GetHex());
        return -1;
    }
    for (int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++) {
        Object purchaseObj;
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
