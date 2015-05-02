#include "mastercore_mdex.h"

#include "mastercore.h"
#include "mastercore_errors.h"
#include "mastercore_log.h"
#include "mastercore_tx.h"

#include "tinyformat.h"
#include "uint256.h"

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#include <openssl/sha.h>

#include <stdint.h>

#include <fstream>
#include <map>
#include <set>
#include <string>

using namespace mastercore;

md_PropertiesMap mastercore::metadex;

md_PricesMap* mastercore::get_Prices(uint32_t prop)
{
    md_PropertiesMap::iterator it = metadex.find(prop);

    if (it != metadex.end()) return &(it->second);

    return (md_PricesMap*) NULL;
}

md_Set* mastercore::get_Indexes(md_PricesMap* p, XDOUBLE price)
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

const std::string getTradeReturnType(MatchReturnType ret)
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

static inline std::string xToString(XDOUBLE value)
{
    return value.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed);
}

static inline int64_t xToInt64(XDOUBLE value, bool fRoundUp = true)
{
    if (fRoundUp) value += (XDOUBLE) 0.5; // ROUND UP
    std::string str_value = value.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed);
    std::string str_value_int_part = str_value.substr(0, str_value.find_first_of("."));

    return boost::lexical_cast<int64_t>(str_value_int_part);
}

static void PriceCheck(const std::string& label, XDOUBLE left, XDOUBLE right)
{
    const bool bOK = (left == right);

    file_log("PRICE CHECK %s: buyer = %s , inserted = %s : %s\n", label,
        xToString(left), xToString(right), bOK ? "good" : "PROBLEM!");
}

// find the best match on the market
// NOTE: sometimes I refer to the older order as seller & the newer order as buyer, in this trade
// INPUT: property, desprop, desprice = of the new order being inserted; the new object being processed
// RETURN: 
static MatchReturnType x_Trade(CMPMetaDEx* newo)
{
    const CMPMetaDEx* p_older = NULL;
    md_PricesMap* prices = NULL;
    const uint32_t prop = newo->getProperty();
    const uint32_t desprop = newo->getDesProperty();
    MatchReturnType NewReturn = NOTHING;
    bool bBuyerSatisfied = false;

    if (msc_debug_metadex1) file_log("%s(%s: prop=%u, desprop=%u, desprice= %s);newo: %s\n",
        __FUNCTION__, newo->getAddr(), prop, desprop, xToString(newo->inversePrice()), newo->ToString());

    prices = get_Prices(desprop);

    // nothing for the desired property exists in the market, sorry!
    if (!prices) {
        file_log("%s()=%u:%s NOT FOUND ON THE MARKET\n", __FUNCTION__, NewReturn, getTradeReturnType(NewReturn));
        return NewReturn;
    }

    // within the desired property map (given one property) iterate over the items looking at prices
    for (md_PricesMap::iterator my_it = prices->begin(); my_it != prices->end(); ++my_it) { // check all prices
        XDOUBLE sellers_price = my_it->first;

        if (msc_debug_metadex2) file_log("comparing prices: desprice %s needs to be GREATER THAN OR EQUAL TO %s\n",
            xToString(newo->inversePrice()), xToString(sellers_price));

        // Is the desired price check satisfied? The buyer's inverse price must be larger than that of the seller.
        if (newo->inversePrice() < sellers_price) continue;

        md_Set* indexes = &(my_it->second);
        
        // at good (single) price level and property iterate over offers looking at all parameters to find the match
        md_Set::iterator iitt;
        for (iitt = indexes->begin(); iitt != indexes->end();) { // specific price, check all properties
            p_older = &(*iitt);

            if (msc_debug_metadex1) file_log("Looking at existing: %s (its prop= %u, its des prop= %u) = %s\n",
                xToString(sellers_price), p_older->getProperty(), p_older->getDesProperty(), p_older->ToString());

            // is the desired property correct?
            if (p_older->getDesProperty() != prop) {
                ++iitt;
                continue;
            }

            if (msc_debug_metadex1) file_log("MATCH FOUND, Trade: %s = %s\n", xToString(sellers_price), p_older->ToString());

            // All Matched ! Trade now.
            // p_older is the old order pointer
            // newo is the new order pointer
            // the price in the older order is used
            const int64_t seller_amountForSale = p_older->getAmountRemaining();
            const int64_t seller_amountWanted = p_older->getAmountDesired();
            const int64_t buyer_amountOffered = newo->getAmountRemaining();

            if (msc_debug_metadex1) file_log("$$ trading using price: %s; seller: forsale= %ld, wanted= %ld, buyer amount offered= %ld\n",
                xToString(sellers_price), seller_amountForSale, seller_amountWanted, buyer_amountOffered);
            if (msc_debug_metadex1) file_log("$$ old: %s\n", p_older->ToString());
            if (msc_debug_metadex1) file_log("$$ new: %s\n", newo->ToString());

            int64_t seller_amountGot = seller_amountWanted;

            if (buyer_amountOffered < seller_amountWanted) {
                seller_amountGot = buyer_amountOffered;
            }

            const int64_t buyer_amountStillForSale = buyer_amountOffered - seller_amountGot;

            ///////////////////////////
            XDOUBLE x_buyer_got = (XDOUBLE) seller_amountGot / sellers_price;
            const int64_t buyer_amountGot = xToInt64(x_buyer_got);

            const int64_t seller_amountLeft = p_older->getAmountRemaining() - buyer_amountGot;

            if (msc_debug_metadex1) file_log("$$ buyer_got= %ld, seller_got= %ld, seller_left_for_sale= %ld, buyer_still_for_sale= %ld\n",
                buyer_amountGot, seller_amountGot, seller_amountLeft, buyer_amountStillForSale);

            ///////////////////////////
            CMPMetaDEx seller_replacement = *p_older;
            seller_replacement.setAmountRemaining(seller_amountLeft, "seller_replacement");

            // transfer the payment property from buyer to seller
            // TODO: do something when failing here............
            // FIXME
            // ...
            if (update_tally_map(newo->getAddr(), newo->getProperty(), -seller_amountGot, BALANCE)) {
                if (update_tally_map(p_older->getAddr(), p_older->getDesProperty(), seller_amountGot, BALANCE)) {
                }
            }

            // transfer the market (the one being sold) property from seller to buyer
            // TODO: do something when failing here............
            // FIXME
            // ...
            if (update_tally_map(p_older->getAddr(), p_older->getProperty(), -buyer_amountGot, METADEX_RESERVE)) {
                update_tally_map(newo->getAddr(), newo->getDesProperty(), buyer_amountGot, BALANCE);
            }

            NewReturn = TRADED;

            newo->setAmountRemaining(buyer_amountStillForSale, "buyer");

            if (0 < buyer_amountStillForSale) {
                NewReturn = TRADED_MOREINBUYER;

                PriceCheck(getTradeReturnType(NewReturn), newo->effectivePrice(), newo->effectivePrice());
            } else {
                bBuyerSatisfied = true;
            }

            if (0 < seller_amountLeft) // done with all loops, update the seller, buyer is fully satisfied
            {
                NewReturn = TRADED_MOREINSELLER;
                bBuyerSatisfied = true;

                PriceCheck(getTradeReturnType(NewReturn), p_older->effectivePrice(), seller_replacement.effectivePrice());
            }

            if (msc_debug_metadex1) file_log("==== TRADED !!! %u=%s\n", NewReturn, getTradeReturnType(NewReturn));

            // record the trade in MPTradeList
            t_tradelistdb->recordTrade(p_older->getHash(), newo->getHash(),
                p_older->getAddr(), newo->getAddr(), p_older->getDesProperty(), newo->getDesProperty(), seller_amountGot, buyer_amountGot, newo->getBlock());

            if (msc_debug_metadex1) file_log("++ erased old: %s\n", iitt->ToString());
            // erase the old seller element
            indexes->erase(iitt++);

            if (bBuyerSatisfied) {
                // insert the updated one in place of the old
                if (0 < seller_replacement.getAmountRemaining()) {
                    file_log("++ inserting seller_replacement: %s\n", seller_replacement.ToString());
                    indexes->insert(seller_replacement);
                }
                break;
            }
        } // specific price, check all properties

        if (bBuyerSatisfied) break;
    } // check all prices

    file_log("%s()=%u:%s\n", __FUNCTION__, NewReturn, getTradeReturnType(NewReturn));

    return NewReturn;
}

XDOUBLE CMPMetaDEx::effectivePrice() const
{
    XDOUBLE effective_price = 0;

    // I am the seller
    if (amount_forsale) effective_price = (XDOUBLE) amount_desired / (XDOUBLE) amount_forsale; // division by 0 check

    return (effective_price);
}

XDOUBLE CMPMetaDEx::inversePrice() const
{
    XDOUBLE inverse_price = 0;
    if (amount_desired) inverse_price = (XDOUBLE) amount_forsale / (XDOUBLE) amount_desired;
    return inverse_price;
}

int64_t CMPMetaDEx::getAmountDesired() const
{
    XDOUBLE xStillDesired = (XDOUBLE) getAmountRemaining() * effectivePrice();

    return xToInt64(xStillDesired);
}

void CMPMetaDEx::setAmountRemaining(int64_t ar, const std::string& label)
{
    amount_remaining = ar;
    file_log("setAmountRemaining(%ld %s):%s\n", ar, label, ToString());

    int64_t ad = getAmountDesired();
    file_log("setAmountDesired(%ld %s):%s\n", ad, label, ToString());
}

void CMPMetaDEx::Set(const std::string& sa, int b, uint32_t c, int64_t nValue, uint32_t cd, int64_t ad, const uint256& tx, uint32_t i, unsigned char suba)
{
    addr = sa;
    block = b;
    txid = tx;
    property = c;
    amount_forsale = nValue;
    desired_property = cd;
    amount_desired = ad;
    idx = i;
    subaction = suba;
}

std::string CMPMetaDEx::ToString() const
{
    return strprintf("%s:%34s in %d/%03u, txid: %s , trade #%u %s for #%u %s",
        xToString(effectivePrice()), addr, block, idx, txid.ToString().substr(0, 10),
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
        (unsigned int) subaction,
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
    if (p_prices) p_indexes = get_Indexes(p_prices, objMetaDEx.effectivePrice());
    // See if the set was populated, if not no set exists at this price level, use the empty set that we created earlier
    if (!p_indexes) p_indexes = &temp_indexes;

    // Attempt to insert the metadex object into the set
    ret = p_indexes->insert(objMetaDEx);
    if (false == ret.second) return false;

    // If a prices map did not exist for this property, set p_prices to the temp empty price map
    if (!p_prices) p_prices = &temp_prices;

    // Update the prices map with the new set at this price
    (*p_prices)[objMetaDEx.effectivePrice()] = *p_indexes;

    // Set the metadex map for the property to the updated (or new if it didn't exist) price map
    metadex[objMetaDEx.getProperty()] = *p_prices;

    return true;
}

// pretty much directly linked to the ADD TX21 command off the wire
int mastercore::MetaDEx_ADD(const std::string& sender_addr, uint32_t prop, int64_t amount, int block, uint32_t property_desired, int64_t amount_desired, const uint256& txid, unsigned int idx)
{
    int rc = METADEX_ERROR -1;

    // Ensure that one side of the trade is MSC/TMSC (phase 1 check)
    if ((prop != OMNI_PROPERTY_MSC) && (property_desired != OMNI_PROPERTY_MSC) &&
        (prop != OMNI_PROPERTY_TMSC) && (property_desired != OMNI_PROPERTY_TMSC)) return METADEX_ERROR -800;

    // Create a MetaDEx object from paremeters
    CMPMetaDEx new_mdex(sender_addr, block, prop, amount, property_desired, amount_desired, txid, idx, CMPTransaction::ADD);
    if (msc_debug_metadex1) file_log("%s(); buyer obj: %s\n", __FUNCTION__, new_mdex.ToString());

    // Ensure this is not a badly priced trade (for example due to zero amounts)
    if (0 >= new_mdex.effectivePrice()) return METADEX_ERROR -66;

    // Match against existing trades, remainder of the order will be put into the order book
    if (msc_debug_metadex3) MetaDEx_debug_print();
    x_Trade(&new_mdex);
    if (msc_debug_metadex3) MetaDEx_debug_print();

    // Insert the remaining order into the MetaDEx maps
    if (0 < new_mdex.getAmountRemaining()) { //switch to getAmountRemaining() when ready
        if (!MetaDEx_INSERT(new_mdex)) {
            file_log("%s() ERROR: ALREADY EXISTS, line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
            return METADEX_ERROR -70;
        } else {
            // TODO: think about failure scenarios // FIXME
            if (update_tally_map(sender_addr, prop, -new_mdex.getAmountRemaining(), BALANCE)) { // move from balance to reserve
                update_tally_map(sender_addr, prop, new_mdex.getAmountRemaining(), METADEX_RESERVE);
            }
            if (msc_debug_metadex1) file_log("==== INSERTED: %s= %s\n", xToString(new_mdex.effectivePrice()), new_mdex.ToString());
            if (msc_debug_metadex3) MetaDEx_debug_print();
        }
    }

    rc = 0;
    return rc;
}

int mastercore::MetaDEx_CANCEL_AT_PRICE(const uint256& txid, unsigned int block, const std::string& sender_addr, uint32_t prop, int64_t amount, uint32_t property_desired, int64_t amount_desired)
{
    int rc = METADEX_ERROR -20;
    CMPMetaDEx mdex(sender_addr, 0, prop, amount, property_desired, amount_desired, 0, 0, CMPTransaction::CANCEL_AT_PRICE);
    md_PricesMap* prices = get_Prices(prop);
    const CMPMetaDEx* p_mdex = NULL;

    if (msc_debug_metadex1) file_log("%s():%s\n", __FUNCTION__, mdex.ToString());

    if (msc_debug_metadex2) MetaDEx_debug_print();

    if (!prices) {
        file_log("%s() NOTHING FOUND for %s\n", __FUNCTION__, mdex.ToString());
        return rc -1;
    }

    // within the desired property map (given one property) iterate over the items
    for (md_PricesMap::iterator my_it = prices->begin(); my_it != prices->end(); ++my_it) {
        XDOUBLE sellers_price = my_it->first;

        if (mdex.effectivePrice() != sellers_price) continue;

        md_Set* indexes = &(my_it->second);

        for (md_Set::iterator iitt = indexes->begin(); iitt != indexes->end();) {
            p_mdex = &(*iitt);

            if (msc_debug_metadex3) file_log("%s(): %s\n", __FUNCTION__, p_mdex->ToString());

            if ((p_mdex->getDesProperty() != property_desired) || (p_mdex->getAddr() != sender_addr)) {
                ++iitt;
                continue;
            }

            rc = 0;
            file_log("%s(): REMOVING %s\n", __FUNCTION__, p_mdex->ToString());

            // move from reserve to main
            update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), -p_mdex->getAmountRemaining(), METADEX_RESERVE);
            update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), p_mdex->getAmountRemaining(), BALANCE);

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

    file_log("%s(%d,%d)\n", __FUNCTION__, prop, property_desired);

    if (msc_debug_metadex3) MetaDEx_debug_print();

    if (!prices) {
        file_log("%s() NOTHING FOUND\n", __FUNCTION__);
        return rc -1;
    }

    // within the desired property map (given one property) iterate over the items
    for (md_PricesMap::iterator my_it = prices->begin(); my_it != prices->end(); ++my_it) {
        md_Set* indexes = &(my_it->second);

        for (md_Set::iterator iitt = indexes->begin(); iitt != indexes->end();) {
            p_mdex = &(*iitt);

            if (msc_debug_metadex3) file_log("%s(): %s\n", __FUNCTION__, p_mdex->ToString());

            if ((p_mdex->getDesProperty() != property_desired) || (p_mdex->getAddr() != sender_addr)) {
                ++iitt;
                continue;
            }

            rc = 0;
            file_log("%s(): REMOVING %s\n", __FUNCTION__, p_mdex->ToString());

            // move from reserve to main
            update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), -p_mdex->getAmountRemaining(), METADEX_RESERVE);
            update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), p_mdex->getAmountRemaining(), BALANCE);

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

    file_log("%s()\n", __FUNCTION__);

    if (msc_debug_metadex2) MetaDEx_debug_print();

    file_log("<<<<<<\n");

    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        unsigned int prop = my_it->first;

        // skip property, if it is not in the expected ecosystem
        if (isMainEcosystemProperty(ecosystem) && !isMainEcosystemProperty(prop)) continue;
        if (isTestEcosystemProperty(ecosystem) && !isTestEcosystemProperty(prop)) continue;

        file_log(" ## property: %u\n", prop);
        md_PricesMap& prices = my_it->second;

        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            XDOUBLE price = it->first;
            md_Set& indexes = it->second;

            file_log("  # Price Level: %s\n", xToString(price));

            for (md_Set::iterator it = indexes.begin(); it != indexes.end();) {
                file_log("%s= %s\n", xToString(price), it->ToString());

                if (it->getAddr() != sender_addr) {
                    ++it;
                    continue;
                }

                rc = 0;
                file_log("%s(): REMOVING %s\n", __FUNCTION__, it->ToString());

                // move from reserve to balance
                update_tally_map(it->getAddr(), it->getProperty(), -it->getAmountRemaining(), METADEX_RESERVE);
                update_tally_map(it->getAddr(), it->getProperty(), it->getAmountRemaining(), BALANCE);

                // record the cancellation
                bool bValid = true;
                p_txlistdb->recordMetaDExCancelTX(txid, it->getHash(), bValid, block, it->getProperty(), it->getAmountRemaining());

                indexes.erase(it++);
            }
        }
    }
    file_log(">>>>>>\n");

    if (msc_debug_metadex2) MetaDEx_debug_print();

    return rc;
}

void mastercore::MetaDEx_debug_print(bool bShowPriceLevel, bool bDisplay)
{
    file_log("<<<\n");
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        uint32_t prop = my_it->first;

        file_log(" ## property: %u\n", prop);
        md_PricesMap& prices = my_it->second;

        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            XDOUBLE price = it->first;
            md_Set& indexes = it->second;

            if (bShowPriceLevel) file_log("  # Price Level: %s\n", xToString(price));

            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                const CMPMetaDEx& obj = *it;

                if (bDisplay) PrintToConsole("%s= %s\n", xToString(price), obj.ToString());
                else file_log("%s= %s\n", xToString(price), obj.ToString());

                // extra checks: price or either of the amounts is 0
                //        assert((XDOUBLE)0 != obj.effectivePrice());
                //        assert(obj.getAmountForSale());
                //        assert(obj.getAmountDesired());
            }
        }
    }
    file_log(">>>\n");
}
