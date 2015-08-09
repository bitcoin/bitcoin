/**
 * @file dex.cpp
 *
 * This file contains DEx logic.
 */

#include "omnicore/dex.h"

#include "omnicore/convert.h"
#include "omnicore/errors.h"
#include "omnicore/log.h"
#include "omnicore/omnicore.h"

#include "main.h"
#include "tinyformat.h"
#include "uint256.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <openssl/sha.h>

#include <stdint.h>

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mastercore
{
/**
 * Checks, if such a sell offer exists.
 */
bool DEx_offerExists(const std::string& seller_addr, uint32_t prop)
{
    if (msc_debug_dex) PrintToLog("%s()\n", __func__);

    const std::string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr, prop);
    OfferMap::iterator my_it = my_offers.find(combo);

    return !(my_it == my_offers.end());
}

/**
 * @return The DEx sell offer, or NULL, if no match was found
 */
CMPOffer* DEx_getOffer(const std::string& seller_addr, uint32_t prop)
{
    if (msc_debug_dex) PrintToLog("%s(%s, %d)\n", __func__, seller_addr, prop);

    const std::string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr, prop);
    OfferMap::iterator it = my_offers.find(combo);

    if (it != my_offers.end()) return &(it->second);

    return NULL;
}

CMPAccept* DEx_getAccept(const std::string& seller_addr, uint32_t prop, const std::string& buyer_addr)
{
    if (msc_debug_dex) PrintToLog("%s(%s, %d, %s)\n", __func__, seller_addr, prop, buyer_addr);

    const std::string combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(seller_addr, buyer_addr, prop);
    AcceptMap::iterator it = my_accepts.find(combo);

    if (it != my_accepts.end()) return &(it->second);

    return NULL;
}

/**
 * TODO: change nAmended: uint64_t -> int64_t
 * @return 0 if everything is OK
 */
int DEx_offerCreate(const std::string& seller_addr, uint32_t prop, int64_t nValue, int block, int64_t amount_des, int64_t fee, uint8_t btl, const uint256& txid, uint64_t* nAmended)
{
    int rc = DEX_ERROR_SELLOFFER;

    // sanity check our params are OK
    if ((!btl) || (!amount_des)) return (DEX_ERROR_SELLOFFER -101); // time limit or amount desired empty

    if (DEx_getOffer(seller_addr, prop)) return (DEX_ERROR_SELLOFFER -10); // offer already exists

    const std::string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr, prop);

    if (msc_debug_dex) PrintToLog("%s(%s|%s), nValue=%d)\n", __func__, seller_addr, combo, nValue);

    const int64_t balanceReallyAvailable = getMPbalance(seller_addr, prop, BALANCE);

    // -------------------------------------------------------------------------
    // TODO: no floating numbers

    // if offering more than available -- put everything up on sale
    if (nValue > balanceReallyAvailable) {
        PrintToLog("%s: adjusting order: %s offers %s, but has only %s SP %d (%s)\n", __func__,
                        seller_addr, FormatDivisibleMP(nValue),
                        FormatDivisibleMP(balanceReallyAvailable),
                        prop, strMPProperty(prop));

        double BTC;

        // AND we must also re-adjust the BTC desired in this case...
        BTC = amount_des * balanceReallyAvailable;
        BTC /= (double) nValue;
        amount_des = rounduint64(BTC);

        nValue = balanceReallyAvailable;

        if (nAmended) *nAmended = nValue;
    }

    // -------------------------------------------------------------------------

    if (nValue > 0) {
        assert(update_tally_map(seller_addr, prop, -nValue, BALANCE));
        assert(update_tally_map(seller_addr, prop, nValue, SELLOFFER_RESERVE));

        my_offers.insert(std::make_pair(combo, CMPOffer(block, nValue, prop, amount_des, fee, btl, txid)));

        rc = 0;
    }

    return rc;
}

/**
 * @return 0 if everything is OK
 */
int DEx_offerDestroy(const std::string& seller_addr, uint32_t prop)
{
    const int64_t amount = getMPbalance(seller_addr, prop, SELLOFFER_RESERVE);

    if (!DEx_offerExists(seller_addr, prop)) return (DEX_ERROR_SELLOFFER -11); // offer does not exist

    const std::string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr, prop);

    OfferMap::iterator my_it = my_offers.find(combo);

    if (amount > 0) {
        assert(update_tally_map(seller_addr, prop, -amount, SELLOFFER_RESERVE));
        assert(update_tally_map(seller_addr, prop, amount, BALANCE)); // give back to the seller from SellOffer-Reserve
    }

    // delete the offer
    my_offers.erase(my_it);

    if (msc_debug_dex) PrintToLog("%s(%s|%s)\n", __func__, seller_addr, combo);

    return 0;
}

/**
 * @return 0 if everything is OK
 */
int DEx_offerUpdate(const std::string& seller_addr, uint32_t prop, int64_t nValue, int block, int64_t desired, int64_t fee, uint8_t btl, const uint256& txid, uint64_t* nAmended)
{
    if (msc_debug_dex) PrintToLog("%s(%s, %d)\n", __func__, seller_addr, prop);

    int rc = DEX_ERROR_SELLOFFER;

    if (!DEx_offerExists(seller_addr, prop)) {
        return (DEX_ERROR_SELLOFFER -12); // offer does not exist
    }

    rc = DEx_offerDestroy(seller_addr, prop);

    if (!rc) {
        rc = DEx_offerCreate(seller_addr, prop, nValue, block, desired, fee, btl, txid, nAmended);
    }

    return rc;
}

/**
 * TODO: change nAmended: uint64_t -> int64_t
 * @return 0 if everything is OK
 */
int DEx_acceptCreate(const std::string& buyer, const std::string& seller, uint32_t prop, int64_t nValue, int block, int64_t fee_paid, uint64_t* nAmended)
{
    int rc = DEX_ERROR_ACCEPT -10;
    OfferMap::iterator my_it;
    const std::string selloffer_combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller, prop);
    const std::string accept_combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(seller, buyer, prop);
    int64_t nActualAmount = getMPbalance(seller, prop, SELLOFFER_RESERVE);

    my_it = my_offers.find(selloffer_combo);

    if (my_it == my_offers.end()) return DEX_ERROR_ACCEPT -15;

    CMPOffer& offer = my_it->second;

    if (msc_debug_dex) PrintToLog("%s(offer: %s)\n", __func__, offer.getHash().GetHex());

    // here we ensure the correct BTC fee was paid in this acceptance message, per spec
    if (fee_paid < offer.getMinFee()) {
        PrintToLog("ERROR: fee too small -- the ACCEPT is rejected! (%d is smaller than %d)\n", fee_paid, offer.getMinFee());
        return DEX_ERROR_ACCEPT -105;
    }

    PrintToLog("%s(%s) OFFER FOUND\n", __func__, selloffer_combo);

    // the older accept is the valid one: do not accept any new ones!
    if (DEx_getAccept(seller, prop, buyer)) {
        PrintToLog("%s() ERROR: an accept from this same seller for this same offer is already open !!!!!\n", __func__);
        return DEX_ERROR_ACCEPT -205;
    }

    if (nActualAmount > nValue) {
        nActualAmount = nValue;

        if (nAmended) *nAmended = nActualAmount;
    }

    // TODO: think if we want to save nValue -- as the amount coming off the wire into the object or not
    if (nActualAmount > 0) {
        assert(update_tally_map(seller, prop, -nActualAmount, SELLOFFER_RESERVE));
        assert(update_tally_map(seller, prop, nActualAmount, ACCEPT_RESERVE));

        CMPAccept acceptOffer(nActualAmount, block, offer.getBlockTimeLimit(), offer.getProperty(), offer.getOfferAmountOriginal(), offer.getBTCDesiredOriginal(), offer.getHash());
        my_accepts.insert(std::make_pair(accept_combo, acceptOffer));

        rc = 0;
    }

    return rc;
}

/**
 * Finalizes an accepted offer.
 *
 * This function is called by handler_block() for each accepted offer that has
 * expired. This function is also called when the purchase has been completed,
 * and the buyer bought everything he was allocated).
 *
 * @return 0 if everything is OK
 */
int DEx_acceptDestroy(const std::string& buyer, const std::string& seller, uint32_t prop, bool bForceErase)
{
    int rc = DEX_ERROR_ACCEPT -20;
    CMPOffer* p_offer = DEx_getOffer(seller, prop);
    CMPAccept* p_accept = DEx_getAccept(seller, prop, buyer);
    bool bReturnToMoney; // return to BALANCE of the seller, otherwise return to SELLOFFER_RESERVE
    const std::string accept_combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(seller, buyer, prop);

    if (!p_accept) return rc; // sanity check

    const int64_t nActualAmount = p_accept->getAcceptAmountRemaining();

    // if the offer is gone ACCEPT_RESERVE should go back to BALANCE
    if (!p_offer) {
        bReturnToMoney = true;
    } else {
        PrintToLog("%s() HASHES: offer=%s, accept=%s\n", __func__, p_offer->getHash().GetHex(), p_accept->getHash().GetHex());

        // offer exists, determine whether it's the original offer or some random new one
        if (p_offer->getHash() == p_accept->getHash()) {
            // same offer, return to SELLOFFER_RESERVE
            bReturnToMoney = false;
        } else {
            // old offer is gone !
            bReturnToMoney = true;
        }
    }

    if (nActualAmount > 0) {
        if (bReturnToMoney) {
            assert(update_tally_map(seller, prop, -nActualAmount, ACCEPT_RESERVE));
            assert(update_tally_map(seller, prop, nActualAmount, BALANCE));
        } else {
            assert(update_tally_map(seller, prop, -nActualAmount, ACCEPT_RESERVE));
            assert(update_tally_map(seller, prop, nActualAmount, SELLOFFER_RESERVE));
        }
    }

    // can only erase when is NOT called from an iterator loop
    if (bForceErase) {
        AcceptMap::iterator my_it = my_accepts.find(accept_combo);

        if (my_accepts.end() != my_it) my_accepts.erase(my_it);
    }

    rc = 0;
    return rc;
}

namespace legacy
{
/**
 * Determine traded amount for a DEx payment.
 * @return The number of tokens that can be purchased
 */
static int64_t calculateDExTrade(int64_t amountOffered, int64_t amountDesired, int64_t amountPaid)
{
    double amountPurchased = double(amountPaid) * double(amountOffered) / double(amountDesired);

    return static_cast<int64_t>(rounduint64(amountPurchased));
}
}

/**
 * Handles incoming BTC payment for the offer.
 * TODO: change nAmended: uint64_t -> int64_t
 */
int DEx_payment(const uint256& txid, unsigned int vout, const std::string& seller, const std::string& buyer, int64_t BTC_paid, int blockNow, uint64_t* nAmended)
{
    int rc = DEX_ERROR_PAYMENT;

    uint32_t prop = OMNI_PROPERTY_MSC; // test for MSC accept first
    CMPAccept* p_accept = DEx_getAccept(seller, prop, buyer);

    if (!p_accept) {
        prop = OMNI_PROPERTY_TMSC; // test for TMSC accept second
        p_accept = DEx_getAccept(seller, prop, buyer);
    }

    if (msc_debug_dex) PrintToLog("%s(%s, %s)\n", __func__, seller, buyer);

    if (!p_accept) return (DEX_ERROR_PAYMENT -1); // there must be an active Accept for this payment

    // -------------------------------------------------------------------------
    // TODO: no floating numbers

    const int64_t amountDesiredBTC = p_accept->getBTCDesiredOriginal();
    const int64_t amountOfferedMSC = p_accept->getOfferAmountOriginal();

    // divide by 0 protection
    if (0 == amountDesiredBTC) return (DEX_ERROR_PAYMENT -2);

    int64_t amountPurchased = legacy::calculateDExTrade(amountOfferedMSC, amountDesiredBTC, BTC_paid);

    // -------------------------------------------------------------------------

    const int64_t amountRemaining = p_accept->getAcceptAmountRemaining(); // actual amount desired, in the Accept

    if (msc_debug_dex) PrintToLog(
            "%s: BTC desired: %s, offered amount: %s, amount to purchase: %s, amount remaining: %s\n", __func__,
            FormatDivisibleMP(amountDesiredBTC), FormatDivisibleMP(amountOfferedMSC),
            FormatDivisibleMP(amountPurchased), FormatDivisibleMP(amountRemaining));

    // if units_purchased is greater than what's in the Accept, the buyer gets only what's in the Accept
    if (amountRemaining < amountPurchased) {
        amountPurchased = amountRemaining;

        if (nAmended) *nAmended = amountPurchased;
    }

    if (amountPurchased > 0) {
        PrintToLog("%s: seller %s offered %s %s for %s BTC\n", __func__,
                seller, FormatDivisibleMP(amountOfferedMSC), strMPProperty(prop), FormatDivisibleMP(amountDesiredBTC));
        PrintToLog("%s: buyer %s pays %s BTC to purchase %s %s\n", __func__,
                buyer, FormatDivisibleMP(BTC_paid), FormatDivisibleMP(amountPurchased), strMPProperty(prop));

        assert(update_tally_map(seller, prop, -amountPurchased, ACCEPT_RESERVE));
        assert(update_tally_map(buyer, prop, amountPurchased, BALANCE));

        bool bValid = true;
        p_txlistdb->recordPaymentTX(txid, bValid, blockNow, vout, prop, amountPurchased, buyer, seller);

        rc = 0;
        PrintToLog("#######################################################\n");
    }

    // reduce the amount of units still desired by the buyer and if 0 must destroy the Accept
    if (p_accept->reduceAcceptAmountRemaining_andIsZero(amountPurchased)) {
        const int64_t selloffer_reserve = getMPbalance(seller, prop, SELLOFFER_RESERVE);
        const int64_t accept_reserve = getMPbalance(seller, prop, ACCEPT_RESERVE);

        DEx_acceptDestroy(buyer, seller, prop, true);

        // delete the Offer object if there is nothing in its Reserves -- everything got puchased and paid for
        if ((0 == selloffer_reserve) && (0 == accept_reserve)) {
            DEx_offerDestroy(seller, prop);
        }
    }

    return rc;
}

unsigned int eraseExpiredAccepts(int blockNow)
{
    unsigned int how_many_erased = 0;
    AcceptMap::iterator my_it = my_accepts.begin();

    while (my_accepts.end() != my_it) {
        CMPAccept& mpaccept = my_it->second;

        if ((blockNow - mpaccept.getAcceptBlock()) >= static_cast<int>(mpaccept.getBlockTimeLimit())) {
            PrintToLog("%s() FOUND EXPIRED ACCEPT, erasing: blockNow=%d, offer block=%d, blocktimelimit= %d\n",
                    __func__, blockNow, mpaccept.getAcceptBlock(), mpaccept.getBlockTimeLimit());

            // extract the seller, buyer and property from the key
            std::vector<std::string> vstr;
            boost::split(vstr, my_it->first, boost::is_any_of("-+"), boost::token_compress_on);
            std::string seller = vstr[0];
            uint32_t property = atoi(vstr[1]);
            std::string buyer = vstr[2];

            DEx_acceptDestroy(buyer, seller, property);

            my_accepts.erase(my_it++);

            ++how_many_erased;
        } else my_it++;
    }

    return how_many_erased;
}


} // namespace mastercore
