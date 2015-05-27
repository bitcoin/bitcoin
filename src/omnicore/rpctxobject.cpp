// Handler for populating RPC transaction objects

// Bitcoin Core includes
#include "wallet.h"
#include "json/json_spirit_value.h"

// Omni Core includes
#include "omnicore/rpctxobject.h"
#include "omnicore/omnicore.h"
#include "omnicore/dex.h"
#include "omnicore/mdex.h"
#include "omnicore/errors.h"
#include "omnicore/sp.h"
#include "omnicore/tx.h"

// Boost includes
#include <boost/exception/to_string.hpp>
#include <boost/lexical_cast.hpp>

// Generic includes
#include <string>

// Namespaces
using namespace json_spirit;
using namespace mastercore;

/* Function to standardize RPC output for transactions into a JSON object in either basic or extended mode.
 * Use basic mode for generic calls (eg gettransaction_MP/listtransaction_MP etc)
 * Use extended mode for transaction specific calls (eg getsto_MP, gettrade_MP etc)
 */
int populateRPCTransactionObject(const uint256& txid, Object& txobj, std::string filterAddress, bool extendedDetails, std::string extendedDetailsFilter)
{
    // retrieve the transaction from the blockchain and obtain it's height/confs/time
    CTransaction wtx;
    uint256 blockHash = 0;
    if (!GetTransaction(txid, wtx, blockHash, true)) return MP_TX_NOT_FOUND;
    if (0 == blockHash) return MP_TX_UNCONFIRMED;
    CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
    if (NULL == pBlockIndex) return MP_BLOCK_NOT_IN_CHAIN;
    int blockHeight = pBlockIndex->nHeight;
    int confirmations =  1 + GetHeight() - pBlockIndex->nHeight;
    int64_t blockTime = pBlockIndex->nTime;

    // lookup transaction in levelDB to see if it's an Omni transaction
    if (!p_txlistdb->exists(txid)) return MP_TX_IS_NOT_MASTER_PROTOCOL;

    // attempt to parse the transaction
    CMPTransaction mp_obj;
    mp_obj.SetNull();
    int parseRC = ParseTransaction(wtx, blockHeight, 0, mp_obj);
    if (parseRC < 0) return MP_INVALID_TX_IN_DB_FOUND;

    // DEx BTC payment needs special handling since it's not actually an Omni message - handle & return
    if (parseRC > 0) {
        std::string tmpBuyer, tmpSeller;
        uint64_t tmpVout, tmpNValue, tmpPropertyId;
        p_txlistdb->getPurchaseDetails(txid, 1, &tmpBuyer, &tmpSeller, &tmpVout, &tmpPropertyId, &tmpNValue);
        Array purchases;
        if (populateRPCDExPurchases(wtx, purchases, filterAddress) <= 0) return -1;
        txobj.push_back(Pair("txid", txid.GetHex()));
        txobj.push_back(Pair("confirmations", confirmations));
        txobj.push_back(Pair("blocktime", blockTime));
        txobj.push_back(Pair("type", "DEx Purchase"));
        txobj.push_back(Pair("sendingaddress", tmpBuyer));
        txobj.push_back(Pair("purchases", purchases));
        return 0;
    }

    // call step1 to populate mp_obj
    if (mp_obj.step1() < 0) return MP_INVALID_TX_IN_DB_FOUND;

    // check if we're filtering from listtransactions_MP, and if so whether we have a non-match we want to skip
    if (!filterAddress.empty() && mp_obj.getSender() != filterAddress && mp_obj.getReceiver() != filterAddress) return -1;

    // obtain validity
    bool valid = getValidMPTX(txid);

    // populate some initial info for the transaction
    bool bMine = false;
    if (IsMyAddress(mp_obj.getSender()) || IsMyAddress(mp_obj.getReceiver())) bMine = true;
    txobj.push_back(Pair("txid", txid.GetHex()));
    txobj.push_back(Pair("sendingaddress", mp_obj.getSender()));
    if (showRefForTx(mp_obj.getType())) txobj.push_back(Pair("referenceaddress", mp_obj.getReceiver()));
    txobj.push_back(Pair("ismine", bMine));
    txobj.push_back(Pair("confirmations", confirmations));
    txobj.push_back(Pair("fee", FormatDivisibleMP(mp_obj.getFeePaid())));
    txobj.push_back(Pair("blocktime", blockTime));
    txobj.push_back(Pair("valid", valid));
    txobj.push_back(Pair("version", (int64_t)mp_obj.getVersion()));
    txobj.push_back(Pair("type_int", (int64_t)mp_obj.getType()));
    txobj.push_back(Pair("type", mp_obj.getTypeString()));

    // populate type specific info & extended details if requested
    populateRPCTypeInfo(mp_obj, txobj, mp_obj.getType(), extendedDetails, extendedDetailsFilter);

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
        case MSC_TYPE_METADEX:
            populateRPCTypeMetaDEx(mp_obj, txobj, extendedDetails);
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
        case MSC_TYPE_METADEX: return false;
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
    if (0 != omniObj.step2_Value()) return;
    uint32_t propertyId = omniObj.getProperty();
    int64_t crowdPropertyId = 0, crowdTokens = 0, issuerTokens = 0;
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
    if (0 != omniObj.step2_Value()) return;
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
    if (extendedDetails) populateRPCExtendedTypeSendToOwners(omniObj.getHash(), extendedDetailsFilter, txobj);
}

void populateRPCTypeTradeOffer(CMPTransaction& omniObj, Object& txobj)
{
    CMPOffer temp_offer;
    if (0 != omniObj.step2_Value()) return;
    if (0 > omniObj.interpretPacket(&temp_offer)) return;
    uint32_t propertyId = omniObj.getProperty();
    int64_t amount = omniObj.getAmount();

    // NOTE: some manipulation of sell_subaction is needed here
    // TODO: interpretPacket should provide reliable data, cleanup at RPC layer is not cool
    unsigned char sell_subaction = temp_offer.getSubaction();
    if (sell_subaction > 3) sell_subaction = 0; // case where subaction byte >3, to have been allowed must be a v0 sell, flip byte to 0
    if (sell_subaction == 0 && amount > 0) sell_subaction = 1; // case where subaction byte=0, must be a v0 sell, amount > 0 means a new sell
    if (sell_subaction == 0 && amount == 0) sell_subaction = 3; // case where subaction byte=0. must be a v0 sell, amount of 0 means a cancel

    // Check levelDB to see if the amount for sale has been amended due to a partial purchase
    // TODO: DEx phase 1 really needs an overhaul to work like MetaDEx with original amounts for sale and amounts remaining etc
    int tmpblock = 0;
    uint32_t tmptype = 0;
    uint64_t amountNew = 0;
    bool tmpValid = getValidMPTX(omniObj.getHash(), &tmpblock, &tmptype, &amountNew);
    if (tmpValid && amountNew > 0) amount = amountNew;

    // Populate
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, amount)));
    txobj.push_back(Pair("feerequired", FormatDivisibleMP(temp_offer.getMinFee())));
    txobj.push_back(Pair("timelimit",  temp_offer.getBlockTimeLimit()));
    if (sell_subaction == 1) txobj.push_back(Pair("action", "new"));
    if (sell_subaction == 2) txobj.push_back(Pair("action", "update"));
    if (sell_subaction == 3) txobj.push_back(Pair("action", "cancel"));
    txobj.push_back(Pair("bitcoindesired", FormatDivisibleMP(temp_offer.getBTCDesiredOriginal())));
}

void populateRPCTypeMetaDEx(CMPTransaction& omniObj, Object& txobj, bool extendedDetails)
{
    CMPMetaDEx metaObj;
    if (0 != omniObj.step2_Value()) return;
    if (0 > omniObj.interpretPacket(NULL, &metaObj)) return;

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

    // action byte handling
    std::string actionStr;
    if (metaObj.getAction() == 1) actionStr = "new";
    if (metaObj.getAction() == 2) actionStr = "cancel price";
    if (metaObj.getAction() == 3) actionStr = "cancel pair";
    if (metaObj.getAction() == 4) actionStr = "cancel all";

    // populate
    txobj.push_back(Pair("propertyidforsale", (uint64_t)omniObj.getProperty()));
    txobj.push_back(Pair("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible));
    txobj.push_back(Pair("amountforsale", FormatMP(omniObj.getProperty(), omniObj.getAmount())));
    txobj.push_back(Pair("amountremaining", FormatMP(omniObj.getProperty(), metaObj.getAmountRemaining())));
    txobj.push_back(Pair("propertyiddesired", (uint64_t)metaObj.getDesProperty()));
    txobj.push_back(Pair("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible));
    txobj.push_back(Pair("amountdesired", FormatMP(propertyIdDesiredIsDivisible, metaObj.getAmountDesired())));
    txobj.push_back(Pair("unitprice", unitPriceStr));
    txobj.push_back(Pair("action", actionStr));
    if (metaObj.getAction() == 4) txobj.push_back(Pair("ecosystem", isTestEcosystemProperty(omniObj.getProperty()) ? "test" : "main"));
    if (extendedDetails) populateRPCExtendedTypeMetaDEx(omniObj.getHash(), metaObj.getAction(), omniObj.getProperty(), omniObj.getAmount(), txobj);
}

void populateRPCTypeAcceptOffer(CMPTransaction& omniObj, Object& txobj)
{
    if (0 != omniObj.step2_Value()) return;
    uint32_t propertyId = omniObj.getProperty();
    int64_t amount = omniObj.getAmount();

    // Check levelDB to see if the amount accepted has been amended due to over accepting amount available
    // TODO: DEx phase 1 really needs an overhaul to work like MetaDEx with original amounts for sale and amounts remaining etc
    int tmpblock = 0;
    uint32_t tmptype = 0;
    uint64_t amountNew = 0;
    bool tmpValid = getValidMPTX(omniObj.getHash(), &tmpblock, &tmptype, &amountNew);
    if (tmpValid && amountNew > 0) amount = amountNew;

    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, amount)));
}

void populateRPCTypeCreatePropertyFixed(CMPTransaction& omniObj, Object& txobj)
{
    int rc = -1;
    omniObj.step2_SmartProperty(rc);
    if (0 != rc) return;
    uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());

    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("propertyname", omniObj.getSPName()));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, getTotalTokens(propertyId))));
}

void populateRPCTypeCreatePropertyVariable(CMPTransaction& omniObj, Object& txobj)
{
    int rc = -1;
    omniObj.step2_SmartProperty(rc);
    if (0 != rc) return;
    uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());

    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("propertyname", omniObj.getSPName()));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, 0))); // crowdsale token creations don't issue tokens with the create tx
}

void populateRPCTypeCreatePropertyManual(CMPTransaction& omniObj, Object& txobj)
{
    int rc = -1;
    omniObj.step2_SmartProperty(rc);
    if (0 != rc) return;
    uint32_t propertyId = _my_sps->findSPByTX(omniObj.getHash());

    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("propertyname", omniObj.getSPName()));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, 0))); // managed token creations don't issue tokens with the create tx
}

void populateRPCTypeCloseCrowdsale(CMPTransaction& omniObj, Object& txobj)
{
    if (0 != omniObj.step2_Value()) return;
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
}

void populateRPCTypeGrant(CMPTransaction& omniObj, Object& txobj)
{
    if (0 != omniObj.step2_Value()) return;
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
}

void populateRPCTypeRevoke(CMPTransaction& omniObj, Object& txobj)
{
    if (0 != omniObj.step2_Value()) return;
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
    txobj.push_back(Pair("amount", FormatMP(propertyId, omniObj.getAmount())));
}

void populateRPCTypeChangeIssuer(CMPTransaction& omniObj, Object& txobj)
{
    if (0 != omniObj.step2_Value()) return;
    uint32_t propertyId = omniObj.getProperty();
    txobj.push_back(Pair("propertyid", (uint64_t)propertyId));
    txobj.push_back(Pair("divisible", isPropertyDivisible(propertyId)));
}

void populateRPCExtendedTypeSendToOwners(const uint256 txid, std::string extendedDetailsFilter, Object& txobj)
{
    Array receiveArray;
    uint64_t tmpAmount = 0, stoFee = 0;
    s_stolistdb->getRecipients(txid, extendedDetailsFilter, &receiveArray, &tmpAmount, &stoFee);
    txobj.push_back(Pair("totalstofee", FormatDivisibleMP(stoFee))); // fee always MSC so always divisible
    txobj.push_back(Pair("recipients", receiveArray));
}

void populateRPCExtendedTypeMetaDEx(const uint256& txid, unsigned char action, uint32_t propertyIdForSale, int64_t amountForSale, Object& txobj)
{
    if (action == 1) {
        Array tradeArray;
        int64_t totalBought = 0, totalSold = 0;
        t_tradelistdb->getMatchingTrades(txid, propertyIdForSale, &tradeArray, &totalSold, &totalBought);
        std::string statusText = MetaDEx_getStatus(txid, propertyIdForSale, amountForSale, totalSold, totalBought);
        txobj.push_back(Pair("status", statusText));
        if(statusText == "cancelled" || statusText == "cancelled part filled") txobj.push_back(Pair("canceltxid", p_txlistdb->findMetaDExCancel(txid).GetHex()));
        txobj.push_back(Pair("matches", tradeArray));
    } else {
        Array cancelArray;
        int numberOfCancels = p_txlistdb->getNumberOfMetaDExCancels(txid);
        if (0<numberOfCancels) {
            for(int refNumber = 1; refNumber <= numberOfCancels; refNumber++) {
                Object cancelTx;
                std::string strValue = p_txlistdb->getKeyValue(txid.ToString() + "-C" + boost::to_string(refNumber));
                if (strValue.empty()) continue;
                std::vector<std::string> vstr;
                boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
                if (vstr.size() != 3) {
                    PrintToLog("TXListDB Error - trade cancel number of tokens is not as expected (%s)\n", strValue);
                    continue;
                }
                uint64_t propId = boost::lexical_cast<uint64_t>(vstr[1]);
                uint64_t amountUnreserved = boost::lexical_cast<uint64_t>(vstr[2]);
                cancelTx.push_back(Pair("txid", vstr[0]));
                cancelTx.push_back(Pair("propertyid", propId));
                cancelTx.push_back(Pair("amountunreserved", FormatMP(propId, amountUnreserved)));
                cancelArray.push_back(cancelTx);
            }
        }
        txobj.push_back(Pair("cancelledtransactions", cancelArray));
    }
}

/* Function to enumerate DEx purchases for a given txid and add to supplied JSON array
 * Note: this function exists as it is feasible for a single transaction to carry multiple outputs
 *       and thus make multiple purchases from a single transaction
 */
int populateRPCDExPurchases(const CTransaction& wtx, Array& purchases, std::string filterAddress)
{
    int numberOfPurchases=p_txlistdb->getNumberOfPurchases(wtx.GetHash());
    if (numberOfPurchases <= 0) {
        PrintToLog("TXLISTDB Error: Transaction %s parsed as a DEx payment but could not locate purchases in txlistdb.\n", wtx.GetHash().GetHex());
        return -1;
    }
    for (int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++) {
        Object purchaseObj;
        std::string buyer, seller;
        uint64_t vout, nValue, propertyId;
        p_txlistdb->getPurchaseDetails(wtx.GetHash(), purchaseNumber, &buyer, &seller, &vout, &propertyId, &nValue);
        if (!filterAddress.empty() && buyer != filterAddress && seller != filterAddress) continue; // filter requested & doesn't match
        bool bIsMine = false;
        if (IsMyAddress(buyer) || IsMyAddress(seller)) bIsMine = true;
        uint64_t amountPaid = wtx.vout[vout].nValue;
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
