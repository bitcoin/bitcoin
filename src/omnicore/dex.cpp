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
bool DEx_offerExists(const std::string& addressSeller, uint32_t propertyId)
{
    std::string key = STR_SELLOFFER_ADDR_PROP_COMBO(addressSeller, propertyId);

    return !(my_offers.find(key) == my_offers.end());
}

/**
 * Retrieves a sell offer.
 *
 * @return The sell offer, or NULL, if no match was found
 */
CMPOffer* DEx_getOffer(const std::string& addressSeller, uint32_t propertyId)
{
    if (msc_debug_dex) PrintToLog("%s(%s, %d)\n", __func__, addressSeller, propertyId);

    std::string key = STR_SELLOFFER_ADDR_PROP_COMBO(addressSeller, propertyId);
    OfferMap::iterator it = my_offers.find(key);

    if (it != my_offers.end()) return &(it->second);

    return NULL;
}

/**
 * Checks, if such an accept order exists.
 */
bool DEx_acceptExists(const std::string& addressSeller, uint32_t propertyId, const std::string& addressBuyer)
{
    std::string key = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(addressSeller, addressBuyer, propertyId);

    return !(my_accepts.find(key) == my_accepts.end());
}

/**
 * Retrieves an accept order.
 *
 * @return The accept order, or NULL, if no match was found
 */
CMPAccept* DEx_getAccept(const std::string& addressSeller, uint32_t propertyId, const std::string& addressBuyer)
{
    if (msc_debug_dex) PrintToLog("%s(%s, %d, %s)\n", __func__, addressSeller, propertyId, addressBuyer);

    std::string key = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(addressSeller, addressBuyer, propertyId);
    AcceptMap::iterator it = my_accepts.find(key);

    if (it != my_accepts.end()) return &(it->second);

    return NULL;
}

namespace legacy
{
/**
 * Determines adjusted amount desired, in case it needs to be recalculated.
 * @return The new number of tokens
 */
static int64_t adjustDExOffer(int64_t amountOffered, int64_t amountDesired, int64_t amountAvailable)
{
    double BTC;

    BTC = amountDesired * amountAvailable;
    BTC /= (double) amountOffered;
    amountDesired = rounduint64(BTC);

    return static_cast<int64_t>(amountDesired);
}
}

/**
 * Creates a new sell offer.
 *
 * TODO: change nAmended: uint64_t -> int64_t
 * @return 0 if everything is OK
 */
int DEx_offerCreate(const std::string& addressSeller, uint32_t propertyId, int64_t amountOffered, int block, int64_t amountDesired, int64_t minAcceptFee, uint8_t paymentWindow, const uint256& txid, uint64_t* nAmended)
{
    int rc = DEX_ERROR_SELLOFFER;

    // sanity checks
    // shold not be removed, because it may be used when updating/destroying an offer
    if (paymentWindow == 0) {
        return (DEX_ERROR_SELLOFFER -101);
    }
    if (amountDesired == 0) {
        return (DEX_ERROR_SELLOFFER -101);
    }
    if (DEx_getOffer(addressSeller, propertyId)) {
        return (DEX_ERROR_SELLOFFER -10); // offer already exists
    }

    const std::string key = STR_SELLOFFER_ADDR_PROP_COMBO(addressSeller, propertyId);
    if (msc_debug_dex) PrintToLog("%s(%s|%s), nValue=%d)\n", __func__, addressSeller, key, amountOffered);

    const int64_t balanceReallyAvailable = getMPbalance(addressSeller, propertyId, BALANCE);

    // if offering more than available -- put everything up on sale
    if (amountOffered > balanceReallyAvailable) {
        PrintToLog("%s: adjusting order: %s offers %s %s, but has only %s %s available\n", __func__,
                        addressSeller, FormatDivisibleMP(amountOffered), strMPProperty(propertyId),
                        FormatDivisibleMP(balanceReallyAvailable), strMPProperty(propertyId));

        // AND we must also re-adjust the BTC desired in this case...
        // TODO: no floating numbers
        amountDesired = legacy::adjustDExOffer(amountOffered, amountDesired, balanceReallyAvailable);
        amountOffered = balanceReallyAvailable;
        if (nAmended) *nAmended = amountOffered;

        PrintToLog("%s: adjusting order: updated amount for sale: %s %s, offered for: %s BTC\n", __func__,
                        FormatDivisibleMP(amountOffered), strMPProperty(propertyId), FormatDivisibleMP(amountDesired));
    }

    if (amountOffered > 0) {
        assert(update_tally_map(addressSeller, propertyId, -amountOffered, BALANCE));
        assert(update_tally_map(addressSeller, propertyId, amountOffered, SELLOFFER_RESERVE));

        CMPOffer sellOffer(block, amountOffered, propertyId, amountDesired, minAcceptFee, paymentWindow, txid);
        my_offers.insert(std::make_pair(key, sellOffer));

        rc = 0;
    }

    return rc;
}

/**
 * Destorys a sell offer.
 *
 * The remaining amount reserved for the offer is returned to the available balance.
 *
 * @return 0 if the offer was destroyed
 */
int DEx_offerDestroy(const std::string& addressSeller, uint32_t propertyId)
{
    if (!DEx_offerExists(addressSeller, propertyId)) {
        return (DEX_ERROR_SELLOFFER -11); // offer does not exist
    }

    const int64_t amountReserved = getMPbalance(addressSeller, propertyId, SELLOFFER_RESERVE);

    // return the remaining reserved amount back to the seller
    if (amountReserved > 0) {
        assert(update_tally_map(addressSeller, propertyId, -amountReserved, SELLOFFER_RESERVE));
        assert(update_tally_map(addressSeller, propertyId, amountReserved, BALANCE));
    }

    // delete the offer
    const std::string key = STR_SELLOFFER_ADDR_PROP_COMBO(addressSeller, propertyId);
    OfferMap::iterator it = my_offers.find(key);
    my_offers.erase(it);

    if (msc_debug_dex) PrintToLog("%s(%s|%s)\n", __func__, addressSeller, key);

    return 0;
}

/**
 * Updates a sell offer.
 *
 * The old offer is destroyed, and replaced by the updated offer.
 *
 * @return 0 if everything is OK
 */
int DEx_offerUpdate(const std::string& addressSeller, uint32_t propertyId, int64_t amountOffered, int block, int64_t amountDesired, int64_t minAcceptFee, uint8_t paymentWindow, const uint256& txid, uint64_t* nAmended)
{
    if (msc_debug_dex) PrintToLog("%s(%s, %d)\n", __func__, addressSeller, propertyId);

    if (!DEx_offerExists(addressSeller, propertyId)) {
        return (DEX_ERROR_SELLOFFER -12); // offer does not exist
    }

    int rc = DEx_offerDestroy(addressSeller, propertyId);

    if (rc == 0) {
        // if the old offer was destroyed successfully, try to create a new one
        rc = DEx_offerCreate(addressSeller, propertyId, amountOffered, block, amountDesired, minAcceptFee, paymentWindow, txid, nAmended);
    }

    return rc;
}

/**
 * Creates a new accept order.
 *
 * TODO: change nAmended: uint64_t -> int64_t
 * @return 0 if everything is OK
 */
int DEx_acceptCreate(const std::string& addressBuyer, const std::string& addressSeller, uint32_t propertyId, int64_t amountAccepted, int block, int64_t feePaid, uint64_t* nAmended)
{
    int rc = DEX_ERROR_ACCEPT -10;
    const std::string keySellOffer = STR_SELLOFFER_ADDR_PROP_COMBO(addressSeller, propertyId);
    const std::string keyAcceptOrder = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(addressSeller, addressBuyer, propertyId);

    OfferMap::const_iterator my_it = my_offers.find(keySellOffer);

    if (my_it == my_offers.end()) {
        PrintToLog("%s: rejected: no matching sell offer for accept order found\n", __func__);
        return (DEX_ERROR_ACCEPT -15);
    }

    const CMPOffer& offer = my_it->second;

    PrintToLog("%s: found a matching sell offer [seller: %s, buyer: %s, property: %d)\n", __func__,
                    addressSeller, addressBuyer, propertyId);

    // the older accept is the valid one: do not accept any new ones!
    if (DEx_acceptExists(addressSeller, propertyId, addressBuyer)) {
        PrintToLog("%s: rejected: an accept order from this same seller for this same offer is already exists\n", __func__);
        return DEX_ERROR_ACCEPT -205;
    }

    // ensure the correct BTC fee was paid in this acceptance message
    if (feePaid < offer.getMinFee()) {
        PrintToLog("%s: rejected: transaction fee too small [%d < %d]\n", __func__, feePaid, offer.getMinFee());
        return DEX_ERROR_ACCEPT -105;
    }

    int64_t nActualAmount = getMPbalance(addressSeller, propertyId, SELLOFFER_RESERVE);

    if (nActualAmount > amountAccepted) {
        nActualAmount = amountAccepted;

        if (nAmended) *nAmended = nActualAmount;
    }

    if (nActualAmount > 0) {
        assert(update_tally_map(addressSeller, propertyId, -nActualAmount, SELLOFFER_RESERVE));
        assert(update_tally_map(addressSeller, propertyId, nActualAmount, ACCEPT_RESERVE));

        CMPAccept acceptOffer(nActualAmount, block, offer.getBlockTimeLimit(), offer.getProperty(), offer.getOfferAmountOriginal(), offer.getBTCDesiredOriginal(), offer.getHash());
        my_accepts.insert(std::make_pair(keyAcceptOrder, acceptOffer));

        rc = 0;
    }

    return rc;
}

/**
 * Finalizes an accepted offer.
 *
 * This function is called by handler_block() for each accepted offer that has
 * expired. This function is also called when the purchase has been completed,
 * and the buyer bought everything that was reserved.
 *
 * @return 0 if everything is OK
 */
int DEx_acceptDestroy(const std::string& addressBuyer, const std::string& addressSeller, uint32_t propertyid, bool fForceErase)
{
    int rc = DEX_ERROR_ACCEPT -20;
    CMPOffer* p_offer = DEx_getOffer(addressSeller, propertyid);
    CMPAccept* p_accept = DEx_getAccept(addressSeller, propertyid, addressBuyer);
    bool fReturnToMoney = true;

    if (!p_accept) return rc; // sanity check

    // if the offer is gone, ACCEPT_RESERVE should go back to seller's BALANCE
    // otherwise move the previously accepted amount back to SELLOFFER_RESERVE
    if (!p_offer) {
        fReturnToMoney = true;
    } else {
        PrintToLog("%s: finalize trade [offer=%s, accept=%s]\n", __func__,
                        p_offer->getHash().GetHex(), p_accept->getHash().GetHex());

        // offer exists, determine whether it's the original offer or some random new one
        if (p_offer->getHash() == p_accept->getHash()) {
            // same offer, return to SELLOFFER_RESERVE
            fReturnToMoney = false;
        } else {
            // old offer is gone !
            fReturnToMoney = true;
        }
    }

    const int64_t amountRemaining = p_accept->getAcceptAmountRemaining();

    if (amountRemaining > 0) {
        if (fReturnToMoney) {
            assert(update_tally_map(addressSeller, propertyid, -amountRemaining, ACCEPT_RESERVE));
            assert(update_tally_map(addressSeller, propertyid, amountRemaining, BALANCE));
        } else {
            assert(update_tally_map(addressSeller, propertyid, -amountRemaining, ACCEPT_RESERVE));
            assert(update_tally_map(addressSeller, propertyid, amountRemaining, SELLOFFER_RESERVE));
        }
    }

    // can only erase when is NOT called from an iterator loop
    if (fForceErase) {
        std::string key = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(addressSeller, addressBuyer, propertyid);
        AcceptMap::iterator it = my_accepts.find(key);

        if (my_accepts.end() != it) {
            my_accepts.erase(it);
        }
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
int DEx_payment(const uint256& txid, unsigned int vout, const std::string& addressSeller, const std::string& addressBuyer, int64_t amountPaid, int block, uint64_t* nAmended)
{
    if (msc_debug_dex) PrintToLog("%s(%s, %s)\n", __func__, addressSeller, addressBuyer);

    int rc = DEX_ERROR_PAYMENT;

    uint32_t propertyId = OMNI_PROPERTY_MSC; // test for MSC accept first
    CMPAccept* p_accept = DEx_getAccept(addressSeller, propertyId, addressBuyer);

    if (!p_accept) {
        propertyId = OMNI_PROPERTY_TMSC; // test for TMSC accept second
        p_accept = DEx_getAccept(addressSeller, propertyId, addressBuyer);
    }

    if (!p_accept) {
        // there must be an active accept order for this payment
        return (DEX_ERROR_PAYMENT -1);
    }

    // -------------------------------------------------------------------------
    // TODO: no floating numbers

    const int64_t amountDesired = p_accept->getBTCDesiredOriginal();
    const int64_t amountOffered = p_accept->getOfferAmountOriginal();

    // divide by 0 protection
    if (0 == amountDesired) {
        if (msc_debug_dex) PrintToLog("%s: ERROR: desired amount of accept order is zero", __func__);

        return (DEX_ERROR_PAYMENT -2);
    }

    int64_t amountPurchased = legacy::calculateDExTrade(amountOffered, amountDesired, amountPaid);

    // -------------------------------------------------------------------------

    const int64_t amountRemaining = p_accept->getAcceptAmountRemaining(); // actual amount desired, in the Accept

    if (msc_debug_dex) PrintToLog(
            "%s: BTC desired: %s, offered amount: %s, amount to purchase: %s, amount remaining: %s\n", __func__,
            FormatDivisibleMP(amountDesired), FormatDivisibleMP(amountOffered),
            FormatDivisibleMP(amountPurchased), FormatDivisibleMP(amountRemaining));

    // if units_purchased is greater than what's in the Accept, the buyer gets only what's in the Accept
    if (amountRemaining < amountPurchased) {
        amountPurchased = amountRemaining;

        if (nAmended) *nAmended = amountPurchased;
    }

    if (amountPurchased > 0) {
        PrintToLog("%s: seller %s offered %s %s for %s BTC\n", __func__,
                addressSeller, FormatDivisibleMP(amountOffered), strMPProperty(propertyId), FormatDivisibleMP(amountDesired));
        PrintToLog("%s: buyer %s pays %s BTC to purchase %s %s\n", __func__,
                addressBuyer, FormatDivisibleMP(amountPaid), FormatDivisibleMP(amountPurchased), strMPProperty(propertyId));

        assert(update_tally_map(addressSeller, propertyId, -amountPurchased, ACCEPT_RESERVE));
        assert(update_tally_map(addressBuyer, propertyId, amountPurchased, BALANCE));

        bool valid = true;
        p_txlistdb->recordPaymentTX(txid, valid, block, vout, propertyId, amountPurchased, addressBuyer, addressSeller);

        rc = 0;
        PrintToLog("#######################################################\n");
    }

    // reduce the amount of units still desired by the buyer and if 0 destroy the Accept order
    if (p_accept->reduceAcceptAmountRemaining_andIsZero(amountPurchased)) {
        const int64_t reserveSell = getMPbalance(addressSeller, propertyId, SELLOFFER_RESERVE);
        const int64_t reserveAccept = getMPbalance(addressSeller, propertyId, ACCEPT_RESERVE);

        DEx_acceptDestroy(addressBuyer, addressSeller, propertyId, true);

        // delete the Offer object if there is nothing in its Reserves -- everything got puchased and paid for
        if ((0 == reserveSell) && (0 == reserveAccept)) {
            DEx_offerDestroy(addressSeller, propertyId);
        }
    }

    return rc;
}

unsigned int eraseExpiredAccepts(int blockNow)
{
    unsigned int how_many_erased = 0;
    AcceptMap::iterator it = my_accepts.begin();

    while (my_accepts.end() != it) {
        const CMPAccept& acceptOrder = it->second;

        int blocksSinceAccept = blockNow - acceptOrder.getAcceptBlock();
        int blocksPaymentWindow = static_cast<int>(acceptOrder.getBlockTimeLimit());

        if (blocksSinceAccept >= blocksPaymentWindow) {
            PrintToLog("%s: sell offer: %s\n", __func__, acceptOrder.getHash().GetHex());
            PrintToLog("%s: erasing at block: %d, order confirmed at block: %d, payment window: %d\n",
                    __func__, blockNow, acceptOrder.getAcceptBlock(), acceptOrder.getBlockTimeLimit());

            // extract the seller, buyer and property from the key
            std::vector<std::string> vstr;
            boost::split(vstr, it->first, boost::is_any_of("-+"), boost::token_compress_on);
            std::string addressSeller = vstr[0];
            uint32_t propertyId = atoi(vstr[1]);
            std::string addressBuyer = vstr[2];

            DEx_acceptDestroy(addressBuyer, addressSeller, propertyId);

            my_accepts.erase(it++);

            ++how_many_erased;
        } else it++;
    }

    return how_many_erased;
}


} // namespace mastercore
