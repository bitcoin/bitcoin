#include "omnicore/mdex.h"

#include "omnicore/errors.h"
#include "omnicore/fees.h"
#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/rules.h"
#include "omnicore/sp.h"
#include "omnicore/tx.h"
#include "omnicore/uint256_extensions.h"

#include "arith_uint256.h"
#include "chain.h"
#include "main.h"
#include "tinyformat.h"
#include "uint256.h"

#include <univalue.h>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

#include <openssl/sha.h>

#include <assert.h>
#include <stdint.h>

#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <string>

typedef boost::multiprecision::cpp_dec_float_100 dec_float;
typedef boost::multiprecision::checked_int128_t int128_t;

using namespace mastercore;

//! Number of digits of unit price
#define DISPLAY_PRECISION_LEN  50

//! Global map for price and order data
md_PropertiesMap mastercore::metadex;

md_PricesMap* mastercore::get_Prices(uint32_t prop)
{
    md_PropertiesMap::iterator it = metadex.find(prop);

    if (it != metadex.end()) return &(it->second);

    return (md_PricesMap*) NULL;
}

md_Set* mastercore::get_Indexes(md_PricesMap* p, rational_t price)
{
    md_PricesMap::iterator it = p->find(price);

    if (it != p->end()) return &(it->second);

    return (md_Set*) NULL;
}

enum MatchReturnType
{
    NOTHING = 0,
    TRADED = 1,
    TRADED_MOREINSELLER,
    TRADED_MOREINBUYER,
    ADDED,
    CANCELLED,
};

static const std::string getTradeReturnType(MatchReturnType ret)
{
    switch (ret) {
        case NOTHING: return "NOTHING";
        case TRADED: return "TRADED";
        case TRADED_MOREINSELLER: return "TRADED_MOREINSELLER";
        case TRADED_MOREINBUYER: return "TRADED_MOREINBUYER";
        case ADDED: return "ADDED";
        case CANCELLED: return "CANCELLED";
        default: return "* unknown *";
    }
}

// Used by rangeInt64, xToInt64
static bool rangeInt64(const int128_t& value)
{
    return (std::numeric_limits<int64_t>::min() <= value && value <= std::numeric_limits<int64_t>::max());
}

// Used by xToString
static bool rangeInt64(const rational_t& value)
{
    return (rangeInt64(value.numerator()) && rangeInt64(value.denominator()));
}

// Used by CMPMetaDEx::displayUnitPrice
static int64_t xToRoundUpInt64(const rational_t& value)
{
    // for integer rounding up: ceil(num / denom) => 1 + (num - 1) / denom
    int128_t result = int128_t(1) + (value.numerator() - int128_t(1)) / value.denominator();

    assert(rangeInt64(result));

    return result.convert_to<int64_t>();
}

std::string xToString(const dec_float& value)
{
    return value.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed);
}

std::string xToString(const int128_t& value)
{
    return strprintf("%s", boost::lexical_cast<std::string>(value));
}

std::string xToString(const rational_t& value)
{
    if (rangeInt64(value)) {
        int64_t num = value.numerator().convert_to<int64_t>();
        int64_t denom = value.denominator().convert_to<int64_t>();
        dec_float x = dec_float(num) / dec_float(denom);
        return xToString(x);
    } else {
        return strprintf("%s / %s", xToString(value.numerator()), xToString(value.denominator()));
    }
}

// find the best match on the market
// NOTE: sometimes I refer to the older order as seller & the newer order as buyer, in this trade
// INPUT: property, desprop, desprice = of the new order being inserted; the new object being processed
// RETURN: 
static MatchReturnType x_Trade(CMPMetaDEx* const pnew)
{
    const uint32_t propertyForSale = pnew->getProperty();
    const uint32_t propertyDesired = pnew->getDesProperty();
    MatchReturnType NewReturn = NOTHING;
    bool bBuyerSatisfied = false;

    if (msc_debug_metadex1) PrintToLog("%s(%s: prop=%d, desprop=%d, desprice= %s);newo: %s\n",
        __FUNCTION__, pnew->getAddr(), propertyForSale, propertyDesired, xToString(pnew->inversePrice()), pnew->ToString());

    md_PricesMap* const ppriceMap = get_Prices(propertyDesired);

    // nothing for the desired property exists in the market, sorry!
    if (!ppriceMap) {
        PrintToLog("%s()=%d:%s NOT FOUND ON THE MARKET\n", __FUNCTION__, NewReturn, getTradeReturnType(NewReturn));
        return NewReturn;
    }

    // within the desired property map (given one property) iterate over the items looking at prices
    for (md_PricesMap::iterator priceIt = ppriceMap->begin(); priceIt != ppriceMap->end(); ++priceIt) { // check all prices
        const rational_t sellersPrice = priceIt->first;

        if (msc_debug_metadex2) PrintToLog("comparing prices: desprice %s needs to be GREATER THAN OR EQUAL TO %s\n",
            xToString(pnew->inversePrice()), xToString(sellersPrice));

        // Is the desired price check satisfied? The buyer's inverse price must be larger than that of the seller.
        if (pnew->inversePrice() < sellersPrice) {
            continue;
        }

        md_Set* const pofferSet = &(priceIt->second);

        // at good (single) price level and property iterate over offers looking at all parameters to find the match
        md_Set::iterator offerIt = pofferSet->begin();
        while (offerIt != pofferSet->end()) { // specific price, check all properties
            const CMPMetaDEx* const pold = &(*offerIt);
            assert(pold->unitPrice() == sellersPrice);

            if (msc_debug_metadex1) PrintToLog("Looking at existing: %s (its prop= %d, its des prop= %d) = %s\n",
                xToString(sellersPrice), pold->getProperty(), pold->getDesProperty(), pold->ToString());

            // does the desired property match?
            if (pold->getDesProperty() != propertyForSale) {
                ++offerIt;
                continue;
            }

            if (msc_debug_metadex1) PrintToLog("MATCH FOUND, Trade: %s = %s\n", xToString(sellersPrice), pold->ToString());

            // match found, execute trade now!
            const int64_t seller_amountForSale = pold->getAmountRemaining();
            const int64_t buyer_amountOffered = pnew->getAmountRemaining();

            if (msc_debug_metadex1) PrintToLog("$$ trading using price: %s; seller: forsale=%d, desired=%d, remaining=%d, buyer amount offered=%d\n",
                xToString(sellersPrice), pold->getAmountForSale(), pold->getAmountDesired(), pold->getAmountRemaining(), pnew->getAmountRemaining());
            if (msc_debug_metadex1) PrintToLog("$$ old: %s\n", pold->ToString());
            if (msc_debug_metadex1) PrintToLog("$$ new: %s\n", pnew->ToString());

            ///////////////////////////

            // preconditions
            assert(0 < pold->getAmountRemaining());
            assert(0 < pnew->getAmountRemaining());
            assert(pnew->getProperty() != pnew->getDesProperty());
            assert(pnew->getProperty() == pold->getDesProperty());
            assert(pold->getProperty() == pnew->getDesProperty());
            assert(pold->unitPrice() <= pnew->inversePrice());
            assert(pnew->unitPrice() <= pold->inversePrice());

            ///////////////////////////

            // First determine how many representable (indivisible) tokens Alice can
            // purchase from Bob, using Bob's unit price
            // This implies rounding down, since rounding up is impossible, and would
            // require more tokens than Alice has
            arith_uint256 iCouldBuy = (ConvertTo256(pnew->getAmountRemaining()) * ConvertTo256(pold->getAmountForSale())) / ConvertTo256(pold->getAmountDesired());

            int64_t nCouldBuy = 0;
            if (iCouldBuy < ConvertTo256(pold->getAmountRemaining())) {
                nCouldBuy = ConvertTo64(iCouldBuy);
            } else {
                nCouldBuy = pold->getAmountRemaining();
            }

            if (nCouldBuy == 0) {
                if (msc_debug_metadex1) PrintToLog(
                        "-- buyer has not enough tokens for sale to purchase one unit!\n");
                ++offerIt;
                continue;
            }

            // If the amount Alice would have to pay to buy Bob's tokens at his price
            // is fractional, always round UP the amount Alice has to pay
            // This will always be better for Bob. Rounding in the other direction
            // will always be impossible, because ot would violate Bob's accepted price
            arith_uint256 iWouldPay = DivideAndRoundUp((ConvertTo256(nCouldBuy) * ConvertTo256(pold->getAmountDesired())), ConvertTo256(pold->getAmountForSale()));
            int64_t nWouldPay = ConvertTo64(iWouldPay);

            // If the resulting adjusted unit price is higher than Alice' price, the
            // orders shall not execute, and no representable fill is made
            const rational_t xEffectivePrice(nWouldPay, nCouldBuy);

            if (xEffectivePrice > pnew->inversePrice()) {
                if (msc_debug_metadex1) PrintToLog(
                        "-- effective price is too expensive: %s\n", xToString(xEffectivePrice));
                ++offerIt;
                continue;
            }

            const int64_t buyer_amountGot = nCouldBuy;
            const int64_t seller_amountGot = nWouldPay;
            const int64_t buyer_amountLeft = pnew->getAmountRemaining() - seller_amountGot;
            const int64_t seller_amountLeft = pold->getAmountRemaining() - buyer_amountGot;

            if (msc_debug_metadex1) PrintToLog("$$ buyer_got= %d, seller_got= %d, seller_left_for_sale= %d, buyer_still_for_sale= %d\n",
                buyer_amountGot, seller_amountGot, seller_amountLeft, buyer_amountLeft);

            ///////////////////////////

            // postconditions
            assert(xEffectivePrice >= pold->unitPrice());
            assert(xEffectivePrice <= pnew->inversePrice());
            assert(0 <= seller_amountLeft);
            assert(0 <= buyer_amountLeft);
            assert(seller_amountForSale == seller_amountLeft + buyer_amountGot);
            assert(buyer_amountOffered == buyer_amountLeft + seller_amountGot);

            ///////////////////////////

            int64_t buyer_amountGotAfterFee = buyer_amountGot;
            int64_t tradingFee = 0;

            // strip a 0.05% fee from non-OMNI pairs if fees are activated
            if (IsFeatureActivated(FEATURE_FEES, pnew->getBlock())) {
                if (pold->getProperty() > OMNI_PROPERTY_TMSC && pold->getDesProperty() > OMNI_PROPERTY_TMSC) {
                    int64_t feeDivider = 2000; // 0.05%
                    tradingFee = buyer_amountGot / feeDivider;

                    // subtract the fee from the amount the seller will receive
                    buyer_amountGotAfterFee = buyer_amountGot - tradingFee;

                    // add the fee to the fee cache
                    p_feecache->AddFee(pnew->getDesProperty(), pnew->getBlock(), tradingFee);
                } else {
                    if (msc_debug_fees) PrintToLog("Skipping fee reduction for trade match %s:%s as one of the properties is Omni\n", pold->getHash().GetHex(), pnew->getHash().GetHex());
                }
            }

            // transfer the payment property from buyer to seller
            assert(update_tally_map(pnew->getAddr(), pnew->getProperty(), -seller_amountGot, BALANCE));
            assert(update_tally_map(pold->getAddr(), pold->getDesProperty(), seller_amountGot, BALANCE));

            // transfer the market (the one being sold) property from seller to buyer
            assert(update_tally_map(pold->getAddr(), pold->getProperty(), -buyer_amountGot, METADEX_RESERVE));
            assert(update_tally_map(pnew->getAddr(), pnew->getDesProperty(), buyer_amountGotAfterFee, BALANCE));

            NewReturn = TRADED;

            CMPMetaDEx seller_replacement = *pold; // < can be moved into last if block
            seller_replacement.setAmountRemaining(seller_amountLeft, "seller_replacement");

            pnew->setAmountRemaining(buyer_amountLeft, "buyer");

            if (0 < buyer_amountLeft) {
                NewReturn = TRADED_MOREINBUYER;
            }

            if (0 == buyer_amountLeft) {
                bBuyerSatisfied = true;
            }

            if (0 < seller_amountLeft) {
                NewReturn = TRADED_MOREINSELLER;
            }

            if (msc_debug_metadex1) PrintToLog("==== TRADED !!! %u=%s\n", NewReturn, getTradeReturnType(NewReturn));

            // record the trade in MPTradeList
            t_tradelistdb->recordMatchedTrade(pold->getHash(), pnew->getHash(), // < might just pass pold, pnew
                pold->getAddr(), pnew->getAddr(), pold->getDesProperty(), pnew->getDesProperty(), seller_amountGot, buyer_amountGotAfterFee, pnew->getBlock(), tradingFee);

            if (msc_debug_metadex1) PrintToLog("++ erased old: %s\n", offerIt->ToString());
            // erase the old seller element
            pofferSet->erase(offerIt++);

            // insert the updated one in place of the old
            if (0 < seller_replacement.getAmountRemaining()) {
                PrintToLog("++ inserting seller_replacement: %s\n", seller_replacement.ToString());
                pofferSet->insert(seller_replacement);
            }

            if (bBuyerSatisfied) {
                assert(buyer_amountLeft == 0);
                break;
            }
        } // specific price, check all properties

        if (bBuyerSatisfied) break;
    } // check all prices

    PrintToLog("%s()=%d:%s\n", __FUNCTION__, NewReturn, getTradeReturnType(NewReturn));

    return NewReturn;
}

/**
 * Used for display of unit prices to 8 decimal places at UI layer.
 *
 * Automatically returns unit or inverse price as needed.
 */
std::string CMPMetaDEx::displayUnitPrice() const
{
     rational_t tmpDisplayPrice;
     if (getDesProperty() == OMNI_PROPERTY_MSC || getDesProperty() == OMNI_PROPERTY_TMSC) {
         tmpDisplayPrice = unitPrice();
         if (isPropertyDivisible(getProperty())) tmpDisplayPrice = tmpDisplayPrice * COIN;
     } else {
         tmpDisplayPrice = inversePrice();
         if (isPropertyDivisible(getDesProperty())) tmpDisplayPrice = tmpDisplayPrice * COIN;
     }

     // offers with unit prices under 0.00000001 will be excluded from UI layer - TODO: find a better way to identify sub 0.00000001 prices
     std::string tmpDisplayPriceStr = xToString(tmpDisplayPrice);
     if (!tmpDisplayPriceStr.empty()) { if (tmpDisplayPriceStr.substr(0,1) == "0") return "0.00000000"; }

     // we must always round up here - for example if the actual price required is 0.3333333344444
     // round: 0.33333333 - price is insufficient and thus won't result in a trade
     // round: 0.33333334 - price will be sufficient to result in a trade
     std::string displayValue = FormatDivisibleMP(xToRoundUpInt64(tmpDisplayPrice));
     return displayValue;
}

/**
 * Used for display of unit prices with 50 decimal places at RPC layer.
 *
 * Note: unit price is no longer always shown in OMNI and/or inverted
 */
std::string CMPMetaDEx::displayFullUnitPrice() const
{
    rational_t tempUnitPrice = unitPrice();

    /* Matching types require no action (divisible/divisible or indivisible/indivisible)
       Non-matching types require adjustment for display purposes
           divisible/indivisible   : *COIN
           indivisible/divisible   : /COIN
    */
    if ( isPropertyDivisible(getProperty()) && !isPropertyDivisible(getDesProperty()) ) tempUnitPrice = tempUnitPrice*COIN;
    if ( !isPropertyDivisible(getProperty()) && isPropertyDivisible(getDesProperty()) ) tempUnitPrice = tempUnitPrice/COIN;

    std::string unitPriceStr = xToString(tempUnitPrice);
    return unitPriceStr;
}

rational_t CMPMetaDEx::unitPrice() const
{
    rational_t effectivePrice;
    if (amount_forsale) effectivePrice = rational_t(amount_desired, amount_forsale);
    return effectivePrice;
}

rational_t CMPMetaDEx::inversePrice() const
{
    rational_t inversePrice;
    if (amount_desired) inversePrice = rational_t(amount_forsale, amount_desired);
    return inversePrice;
}

int64_t CMPMetaDEx::getAmountToFill() const
{
    // round up to ensure that the amount we present will actually result in buying all available tokens
    arith_uint256 iAmountNeededToFill = DivideAndRoundUp((ConvertTo256(amount_remaining) * ConvertTo256(amount_desired)), ConvertTo256(amount_forsale));
    int64_t nAmountNeededToFill = ConvertTo64(iAmountNeededToFill);
    return nAmountNeededToFill;
}

int64_t CMPMetaDEx::getBlockTime() const
{
    CBlockIndex* pblockindex = chainActive[block];
    return pblockindex->GetBlockTime();
}

void CMPMetaDEx::setAmountRemaining(int64_t amount, const std::string& label)
{
    amount_remaining = amount;
    PrintToLog("update remaining amount still up for sale (%ld %s):%s\n", amount, label, ToString());
}

std::string CMPMetaDEx::ToString() const
{
    return strprintf("%s:%34s in %d/%03u, txid: %s , trade #%u %s for #%u %s",
        xToString(unitPrice()), addr, block, idx, txid.ToString().substr(0, 10),
        property, FormatMP(property, amount_forsale), desired_property, FormatMP(desired_property, amount_desired));
}

void CMPMetaDEx::saveOffer(std::ofstream& file, SHA256_CTX* shaCtx) const
{
    std::string lineOut = strprintf("%s,%d,%d,%d,%d,%d,%d,%d,%s,%d",
        addr,
        block,
        amount_forsale,
        property,
        amount_desired,
        desired_property,
        subaction,
        idx,
        txid.ToString(),
        amount_remaining
    );

    // add the line to the hash
    SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << std::endl;
}

bool MetaDEx_compare::operator()(const CMPMetaDEx &lhs, const CMPMetaDEx &rhs) const
{
    if (lhs.getBlock() == rhs.getBlock()) return lhs.getIdx() < rhs.getIdx();
    else return lhs.getBlock() < rhs.getBlock();
}

bool mastercore::MetaDEx_INSERT(const CMPMetaDEx& objMetaDEx)
{
    // Create an empty price map (to use in case price map for this property does not already exist)
    md_PricesMap temp_prices;
    // Attempt to obtain the price map for the property
    md_PricesMap *p_prices = get_Prices(objMetaDEx.getProperty());

    // Create an empty set of metadex objects (to use in case no set currently exists at this price)
    md_Set temp_indexes;
    md_Set *p_indexes = NULL;

    // Prepare for return code
    std::pair <md_Set::iterator, bool> ret;

    // Attempt to obtain a set of metadex objects for this price from the price map
    if (p_prices) p_indexes = get_Indexes(p_prices, objMetaDEx.unitPrice());
    // See if the set was populated, if not no set exists at this price level, use the empty set that we created earlier
    if (!p_indexes) p_indexes = &temp_indexes;

    // Attempt to insert the metadex object into the set
    ret = p_indexes->insert(objMetaDEx);
    if (false == ret.second) return false;

    // If a prices map did not exist for this property, set p_prices to the temp empty price map
    if (!p_prices) p_prices = &temp_prices;

    // Update the prices map with the new set at this price
    (*p_prices)[objMetaDEx.unitPrice()] = *p_indexes;

    // Set the metadex map for the property to the updated (or new if it didn't exist) price map
    metadex[objMetaDEx.getProperty()] = *p_prices;

    return true;
}

// pretty much directly linked to the ADD TX21 command off the wire
int mastercore::MetaDEx_ADD(const std::string& sender_addr, uint32_t prop, int64_t amount, int block, uint32_t property_desired, int64_t amount_desired, const uint256& txid, unsigned int idx)
{
    int rc = METADEX_ERROR -1;

    // Create a MetaDEx object from paremeters
    CMPMetaDEx new_mdex(sender_addr, block, prop, amount, property_desired, amount_desired, txid, idx, CMPTransaction::ADD);
    if (msc_debug_metadex1) PrintToLog("%s(); buyer obj: %s\n", __FUNCTION__, new_mdex.ToString());

    // Ensure this is not a badly priced trade (for example due to zero amounts)
    if (0 >= new_mdex.unitPrice()) return METADEX_ERROR -66;

    // Match against existing trades, remainder of the order will be put into the order book
    if (msc_debug_metadex3) MetaDEx_debug_print();
    x_Trade(&new_mdex);
    if (msc_debug_metadex3) MetaDEx_debug_print();

    // Insert the remaining order into the MetaDEx maps
    if (0 < new_mdex.getAmountRemaining()) { //switch to getAmountRemaining() when ready
        if (!MetaDEx_INSERT(new_mdex)) {
            PrintToLog("%s() ERROR: ALREADY EXISTS, line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
            return METADEX_ERROR -70;
        } else {
            // move tokens into reserve
            assert(update_tally_map(sender_addr, prop, -new_mdex.getAmountRemaining(), BALANCE));
            assert(update_tally_map(sender_addr, prop, new_mdex.getAmountRemaining(), METADEX_RESERVE));

            if (msc_debug_metadex1) PrintToLog("==== INSERTED: %s= %s\n", xToString(new_mdex.unitPrice()), new_mdex.ToString());
            if (msc_debug_metadex3) MetaDEx_debug_print();
        }
    }

    rc = 0;
    return rc;
}

int mastercore::MetaDEx_CANCEL_AT_PRICE(const uint256& txid, unsigned int block, const std::string& sender_addr, uint32_t prop, int64_t amount, uint32_t property_desired, int64_t amount_desired)
{
    int rc = METADEX_ERROR -20;
    CMPMetaDEx mdex(sender_addr, 0, prop, amount, property_desired, amount_desired, uint256(), 0, CMPTransaction::CANCEL_AT_PRICE);
    md_PricesMap* prices = get_Prices(prop);
    const CMPMetaDEx* p_mdex = NULL;

    if (msc_debug_metadex1) PrintToLog("%s():%s\n", __FUNCTION__, mdex.ToString());

    if (msc_debug_metadex2) MetaDEx_debug_print();

    if (!prices) {
        PrintToLog("%s() NOTHING FOUND for %s\n", __FUNCTION__, mdex.ToString());
        return rc -1;
    }

    // within the desired property map (given one property) iterate over the items
    for (md_PricesMap::iterator my_it = prices->begin(); my_it != prices->end(); ++my_it) {
        rational_t sellers_price = my_it->first;

        if (mdex.unitPrice() != sellers_price) continue;

        md_Set* indexes = &(my_it->second);

        for (md_Set::iterator iitt = indexes->begin(); iitt != indexes->end();) {
            p_mdex = &(*iitt);

            if (msc_debug_metadex3) PrintToLog("%s(): %s\n", __FUNCTION__, p_mdex->ToString());

            if ((p_mdex->getDesProperty() != property_desired) || (p_mdex->getAddr() != sender_addr)) {
                ++iitt;
                continue;
            }

            rc = 0;
            PrintToLog("%s(): REMOVING %s\n", __FUNCTION__, p_mdex->ToString());

            // move from reserve to main
            assert(update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), -p_mdex->getAmountRemaining(), METADEX_RESERVE));
            assert(update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), p_mdex->getAmountRemaining(), BALANCE));

            // record the cancellation
            bool bValid = true;
            p_txlistdb->recordMetaDExCancelTX(txid, p_mdex->getHash(), bValid, block, p_mdex->getProperty(), p_mdex->getAmountRemaining());

            indexes->erase(iitt++);
        }
    }

    if (msc_debug_metadex2) MetaDEx_debug_print();

    return rc;
}

int mastercore::MetaDEx_CANCEL_ALL_FOR_PAIR(const uint256& txid, unsigned int block, const std::string& sender_addr, uint32_t prop, uint32_t property_desired)
{
    int rc = METADEX_ERROR -30;
    md_PricesMap* prices = get_Prices(prop);
    const CMPMetaDEx* p_mdex = NULL;

    PrintToLog("%s(%d,%d)\n", __FUNCTION__, prop, property_desired);

    if (msc_debug_metadex3) MetaDEx_debug_print();

    if (!prices) {
        PrintToLog("%s() NOTHING FOUND\n", __FUNCTION__);
        return rc -1;
    }

    // within the desired property map (given one property) iterate over the items
    for (md_PricesMap::iterator my_it = prices->begin(); my_it != prices->end(); ++my_it) {
        md_Set* indexes = &(my_it->second);

        for (md_Set::iterator iitt = indexes->begin(); iitt != indexes->end();) {
            p_mdex = &(*iitt);

            if (msc_debug_metadex3) PrintToLog("%s(): %s\n", __FUNCTION__, p_mdex->ToString());

            if ((p_mdex->getDesProperty() != property_desired) || (p_mdex->getAddr() != sender_addr)) {
                ++iitt;
                continue;
            }

            rc = 0;
            PrintToLog("%s(): REMOVING %s\n", __FUNCTION__, p_mdex->ToString());

            // move from reserve to main
            assert(update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), -p_mdex->getAmountRemaining(), METADEX_RESERVE));
            assert(update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), p_mdex->getAmountRemaining(), BALANCE));

            // record the cancellation
            bool bValid = true;
            p_txlistdb->recordMetaDExCancelTX(txid, p_mdex->getHash(), bValid, block, p_mdex->getProperty(), p_mdex->getAmountRemaining());

            indexes->erase(iitt++);
        }
    }

    if (msc_debug_metadex3) MetaDEx_debug_print();

    return rc;
}

/**
 * Scans the orderbook and remove everything for an address.
 */
int mastercore::MetaDEx_CANCEL_EVERYTHING(const uint256& txid, unsigned int block, const std::string& sender_addr, unsigned char ecosystem)
{
    int rc = METADEX_ERROR -40;

    PrintToLog("%s()\n", __FUNCTION__);

    if (msc_debug_metadex2) MetaDEx_debug_print();

    PrintToLog("<<<<<<\n");

    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        unsigned int prop = my_it->first;

        // skip property, if it is not in the expected ecosystem
        if (isMainEcosystemProperty(ecosystem) && !isMainEcosystemProperty(prop)) continue;
        if (isTestEcosystemProperty(ecosystem) && !isTestEcosystemProperty(prop)) continue;

        PrintToLog(" ## property: %u\n", prop);
        md_PricesMap& prices = my_it->second;

        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            rational_t price = it->first;
            md_Set& indexes = it->second;

            PrintToLog("  # Price Level: %s\n", xToString(price));

            for (md_Set::iterator it = indexes.begin(); it != indexes.end();) {
                PrintToLog("%s= %s\n", xToString(price), it->ToString());

                if (it->getAddr() != sender_addr) {
                    ++it;
                    continue;
                }

                rc = 0;
                PrintToLog("%s(): REMOVING %s\n", __FUNCTION__, it->ToString());

                // move from reserve to balance
                assert(update_tally_map(it->getAddr(), it->getProperty(), -it->getAmountRemaining(), METADEX_RESERVE));
                assert(update_tally_map(it->getAddr(), it->getProperty(), it->getAmountRemaining(), BALANCE));

                // record the cancellation
                bool bValid = true;
                p_txlistdb->recordMetaDExCancelTX(txid, it->getHash(), bValid, block, it->getProperty(), it->getAmountRemaining());

                indexes.erase(it++);
            }
        }
    }
    PrintToLog(">>>>>>\n");

    if (msc_debug_metadex2) MetaDEx_debug_print();

    return rc;
}

/**
 * Scans the orderbook and removes every all-pair order
 */
int mastercore::MetaDEx_SHUTDOWN_ALLPAIR()
{
    int rc = 0;
    PrintToLog("%s()\n", __FUNCTION__);
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap& prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            md_Set& indexes = it->second;
            for (md_Set::iterator it = indexes.begin(); it != indexes.end();) {
                if (it->getDesProperty() > OMNI_PROPERTY_TMSC && it->getProperty() > OMNI_PROPERTY_TMSC) { // no OMNI/TOMNI side to the trade
                    PrintToLog("%s(): REMOVING %s\n", __FUNCTION__, it->ToString());
                    // move from reserve to balance
                    assert(update_tally_map(it->getAddr(), it->getProperty(), -it->getAmountRemaining(), METADEX_RESERVE));
                    assert(update_tally_map(it->getAddr(), it->getProperty(), it->getAmountRemaining(), BALANCE));
                    indexes.erase(it++);
                }
            }
        }
    }
    return rc;
}

/**
 * Scans the orderbook and removes every order
 */
int mastercore::MetaDEx_SHUTDOWN()
{
    int rc = 0;
    PrintToLog("%s()\n", __FUNCTION__);
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap& prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            md_Set& indexes = it->second;
            for (md_Set::iterator it = indexes.begin(); it != indexes.end();) {
                PrintToLog("%s(): REMOVING %s\n", __FUNCTION__, it->ToString());
                // move from reserve to balance
                assert(update_tally_map(it->getAddr(), it->getProperty(), -it->getAmountRemaining(), METADEX_RESERVE));
                assert(update_tally_map(it->getAddr(), it->getProperty(), it->getAmountRemaining(), BALANCE));
                indexes.erase(it++);
            }
        }
    }
    return rc;
}

// searches the metadex maps to see if a trade is still open
// allows search to be optimized if propertyIdForSale is specified
bool mastercore::MetaDEx_isOpen(const uint256& txid, uint32_t propertyIdForSale)
{
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        if (propertyIdForSale != 0 && propertyIdForSale != my_it->first) continue;
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            md_Set & indexes = (it->second);
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                CMPMetaDEx obj = *it;
                if( obj.getHash().GetHex() == txid.GetHex() ) return true;
            }
        }
    }
    return false;
}

/**
 * Returns a string describing the status of a trade
 *
 */
std::string mastercore::MetaDEx_getStatusText(int tradeStatus)
{
    switch (tradeStatus) {
        case TRADE_OPEN: return "open";
        case TRADE_OPEN_PART_FILLED: return "open part filled";
        case TRADE_FILLED: return "filled";
        case TRADE_CANCELLED: return "cancelled";
        case TRADE_CANCELLED_PART_FILLED: return "cancelled part filled";
        case TRADE_INVALID: return "trade invalid";
        default: return "unknown";
    }
}

/**
 * Returns the status of a MetaDEx trade
 *
 */
int mastercore::MetaDEx_getStatus(const uint256& txid, uint32_t propertyIdForSale, int64_t amountForSale, int64_t totalSold)
{
    // NOTE: If the calling code is already aware of the total amount sold, pass the value in to this function to avoid duplication of
    //       work.  If the calling code doesn't know the amount, leave default (-1) and we will calculate it from levelDB lookups.
    if (totalSold == -1) {
        UniValue tradeArray(UniValue::VARR);
        int64_t totalReceived;
        t_tradelistdb->getMatchingTrades(txid, propertyIdForSale, tradeArray, totalSold, totalReceived);
    }

    // Return a "trade invalid" status if the trade was invalidated at parsing/interpretation (eg insufficient funds)
    if (!getValidMPTX(txid)) return TRADE_INVALID;

    // Calculate and return the status of the trade via the amount sold and open/closed attributes.
    if (MetaDEx_isOpen(txid, propertyIdForSale)) {
        if (totalSold == 0) {
            return TRADE_OPEN;
        } else {
            return TRADE_OPEN_PART_FILLED;
        }
    } else {
        if (totalSold == 0) {
            return TRADE_CANCELLED;
        } else if (totalSold < amountForSale) {
            return TRADE_CANCELLED_PART_FILLED;
        } else {
            return TRADE_FILLED;
        }
    }
}

void mastercore::MetaDEx_debug_print(bool bShowPriceLevel, bool bDisplay)
{
    PrintToLog("<<<\n");
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        uint32_t prop = my_it->first;

        PrintToLog(" ## property: %u\n", prop);
        md_PricesMap& prices = my_it->second;

        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            rational_t price = it->first;
            md_Set& indexes = it->second;

            if (bShowPriceLevel) PrintToLog("  # Price Level: %s\n", xToString(price));

            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                const CMPMetaDEx& obj = *it;

                if (bDisplay) PrintToConsole("%s= %s\n", xToString(price), obj.ToString());
                else PrintToLog("%s= %s\n", xToString(price), obj.ToString());
            }
        }
    }
    PrintToLog(">>>\n");
}

/**
 * Locates a trade in the MetaDEx maps via txid and returns the trade object
 *
 */
const CMPMetaDEx* mastercore::MetaDEx_RetrieveTrade(const uint256& txid)
{
    for (md_PropertiesMap::iterator propIter = metadex.begin(); propIter != metadex.end(); ++propIter) {
        md_PricesMap & prices = propIter->second;
        for (md_PricesMap::iterator pricesIter = prices.begin(); pricesIter != prices.end(); ++pricesIter) {
            md_Set & indexes = pricesIter->second;
            for (md_Set::iterator tradesIter = indexes.begin(); tradesIter != indexes.end(); ++tradesIter) {
                if (txid == (*tradesIter).getHash()) return &(*tradesIter);
            }
        }
    }
    return (CMPMetaDEx*) NULL;
}
