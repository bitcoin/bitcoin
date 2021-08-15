/**
 * @file dex.cpp
 *
 * This file contains DEx logic.
 */

#include <omnicore/dex.h>

#include <omnicore/convert.h>
#include <omnicore/dbtxlist.h>
#include <omnicore/log.h>
#include <omnicore/rules.h>
#include <omnicore/uint256_extensions.h>

#include <arith_uint256.h>
#include <validation.h>
#include <tinyformat.h>
#include <uint256.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

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
 * Checks, if the seller has any open offer.
 */
bool DEx_hasOffer(const std::string& addressSeller)
{
    for (auto const& offer : my_offers) {
        if (offer.first.find(addressSeller) == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Retrieves the identifier of the token for sale.
 *
 * NOTE: special care, if there are multiple open offers!
 * NOTE: the assumption is there can only be one active offer per seller!
 *
 * @param addressSeller The address of the seller with an open offer
 * @param retTokenId    The token identifier for sale
 * @return True, if there is an open offer
 */
bool DEx_getTokenForSale(const std::string& addressSeller, uint32_t& retTokenId)
{
    for (auto const& offer : my_offers) {
        if (offer.first.find(addressSeller) == 0) {

            // Format is: "address-tokenid"
            std::vector<std::string> vstr;
            boost::split(vstr, offer.first, boost::is_any_of("-"), boost::token_compress_on);

            if (vstr.size() != 2) {
                PrintToLog("ERROR: failed to parse token for sale: %s\n", offer.first);
                return false;
            }

            try {
                retTokenId = boost::lexical_cast<uint32_t>(vstr[1]);
                return true;
            }
            catch (boost::bad_lexical_cast const& e) {
                PrintToLog("ERROR: failed to parse token for sale: %s (%s)\n", offer.first, e.what());
            }
        }
    }

    return false;
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

    return nullptr;
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

    return nullptr;
}

namespace legacy
{
/**
 * Legacy calculation of Master Core 0.0.9.
 *
 * @see:
 * https://github.com/mastercoin-MSC/mastercore/blob/mscore-0.0.9/src/mastercore_dex.cpp#L439-L449
 */
static int64_t calculateDesiredBTC(const int64_t amountOffered, const int64_t amountDesired, const int64_t amountAvailable)
{
    uint64_t nValue = static_cast<uint64_t>(amountOffered);
    uint64_t amount_des = static_cast<uint64_t>(amountDesired);
    uint64_t balanceReallyAvailable = static_cast<uint64_t>(amountAvailable);

    double BTC;

    BTC = amount_des * balanceReallyAvailable;
    BTC /= (double) nValue;
    amount_des = rounduint64(BTC);

    return static_cast<int64_t>(amount_des);
}
}

/**
 * Determines the amount of bitcoins desired, in case it needs to be recalculated.
 *
 * TODO: don't expose it!
 * @return The amount of bitcoins desired
 */
int64_t calculateDesiredBTC(const int64_t amountOffered, const int64_t amountDesired, const int64_t amountAvailable)
{
    if (amountOffered == 0) {
        return 0; // divide by null protection
    }

    arith_uint256 amountOffered256 = ConvertTo256(amountOffered);
    arith_uint256 amountDesired256 = ConvertTo256(amountDesired);
    arith_uint256 amountAvailable256 = ConvertTo256(amountAvailable);

    amountDesired256 = DivideAndRoundUp((amountDesired256 * amountAvailable256), amountOffered256);

    return ConvertTo64(amountDesired256);
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

    // Ensure further there can only be one active offer
    if (IsFeatureActivated(FEATURE_FREEDEX, block)) {
        if (DEx_hasOffer(addressSeller)) {
            return (DEX_ERROR_SELLOFFER -10); // offer already exists
        }
    }

    const std::string key = STR_SELLOFFER_ADDR_PROP_COMBO(addressSeller, propertyId);
    if (msc_debug_dex) PrintToLog("%s(%s|%s), nValue=%d)\n", __func__, addressSeller, key, amountOffered);

    const int64_t balanceReallyAvailable = GetTokenBalance(addressSeller, propertyId, BALANCE);

    /**
     * After this feature is enabled, it is no longer valid to create orders, which offer more than
     * the seller has available, and the amounts are no longer adjusted based on the actual balance.
     */
    if (IsFeatureActivated(FEATURE_DEXMATH, block)) {
        if (amountOffered > balanceReallyAvailable) {
            PrintToLog("%s: rejected: sender %s has insufficient balance of property %d [%s < %s]\n", __func__,
                        addressSeller, propertyId, FormatDivisibleMP(balanceReallyAvailable), FormatDivisibleMP(amountOffered));
            return (DEX_ERROR_SELLOFFER -25);
        }
    }

    // -------------------------------------------------------------------------
    // legacy::

    // if offering more than available -- put everything up on sale
    if (amountOffered > balanceReallyAvailable) {
        PrintToLog("%s: adjusting order: %s offers %s %s, but has only %s %s available\n", __func__,
                        addressSeller, FormatDivisibleMP(amountOffered), strMPProperty(propertyId),
                        FormatDivisibleMP(balanceReallyAvailable), strMPProperty(propertyId));

        // AND we must also re-adjust the BTC desired in this case...
        amountDesired = legacy::calculateDesiredBTC(amountOffered, amountDesired, balanceReallyAvailable);
        amountOffered = balanceReallyAvailable;
        if (nAmended) *nAmended = amountOffered;

        PrintToLog("%s: adjusting order: updated amount for sale: %s %s, offered for: %s QTC\n", __func__,
                        FormatDivisibleMP(amountOffered), strMPProperty(propertyId), FormatDivisibleMP(amountDesired));
    }
    // -------------------------------------------------------------------------

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
 * Destroys a sell offer.
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

    const int64_t amountReserved = GetTokenBalance(addressSeller, propertyId, SELLOFFER_RESERVE);

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
        PrintToLog("%s: rejected: an accept order from this same seller for this same offer already exists\n", __func__);
        return DEX_ERROR_ACCEPT -205;
    }

    // ensure the correct BTC fee was paid in this acceptance message
    if (feePaid < offer.getMinFee()) {
        PrintToLog("%s: rejected: transaction fee too small [%d < %d]\n", __func__, feePaid, offer.getMinFee());
        return DEX_ERROR_ACCEPT -105;
    }

    int64_t amountReserved = 0;
    int64_t amountRemainingForSale = GetTokenBalance(addressSeller, propertyId, SELLOFFER_RESERVE);

    // ensure the buyer only can reserve the amount that is still available
    if (amountRemainingForSale >= amountAccepted) {
        amountReserved = amountAccepted;
    } else {
        amountReserved = amountRemainingForSale;
        PrintToLog("%s: buyer wants to reserve %d tokens, but only %d tokens are available\n", __func__, amountAccepted, amountRemainingForSale);
    }

    if (amountReserved > 0) {
        assert(update_tally_map(addressSeller, propertyId, -amountReserved, SELLOFFER_RESERVE));
        assert(update_tally_map(addressSeller, propertyId, amountReserved, ACCEPT_RESERVE));

        CMPAccept acceptOffer(amountReserved, block, offer.getBlockTimeLimit(), offer.getProperty(), offer.getOfferAmountOriginal(), offer.getBTCDesiredOriginal(), offer.getHash());
        my_accepts.insert(std::make_pair(keyAcceptOrder, acceptOffer));

        rc = 0;
    }

    if (nAmended) *nAmended = amountReserved;

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
 * Legacy calculation of Master Core 0.0.9.
 *
 * @see:
 * https://github.com/mastercoin-MSC/mastercore/blob/mscore-0.0.9/src/mastercore_dex.cpp#L660-L668
 */
static int64_t calculateDExPurchase(const int64_t amountOffered, const int64_t amountDesired, const int64_t amountPaid)
{
    uint64_t acceptOfferAmount = static_cast<uint64_t>(amountOffered);
    uint64_t acceptBTCDesired = static_cast<uint64_t>(amountDesired);
    uint64_t BTC_paid = static_cast<uint64_t>(amountPaid);

    const double BTC_desired_original = acceptBTCDesired;
    const double offer_amount_original = acceptOfferAmount;

    double perc_X = (double) BTC_paid / BTC_desired_original;
    double Purchased = offer_amount_original * perc_X;

    uint64_t units_purchased = rounduint64(Purchased);

    return static_cast<int64_t>(units_purchased);
}
}

/**
 * Determines the purchased amount of tokens.
 *
 * The amount is rounded up, which may be in favor of the buyer, to avoid small
 * leftovers of 1 willet. This is not exploitable due to transaction fees.
 *
 * The desired amount must not be zero, and the purchased amount should be checked
 * against the actual amount that is still left for sale.
 *
 * @return The amount of tokens purchased
 */
int64_t calculateDExPurchase(const int64_t amountOffered, const int64_t amountDesired, const int64_t amountPaid)
{
    // conversion
    arith_uint256 amountOffered256 = ConvertTo256(amountOffered);
    arith_uint256 amountDesired256 = ConvertTo256(amountDesired);
    arith_uint256 amountPaid256 = ConvertTo256(amountPaid);

    // actual calculation; round up
    arith_uint256 amountPurchased256 = DivideAndRoundUp((amountPaid256 * amountOffered256), amountDesired256);

    // convert back to int64_t
    return ConvertTo64(amountPurchased256);
}

/**
 * Handles incoming BTC payment for the offer.
 * TODO: change nAmended: uint64_t -> int64_t
 */
int DEx_payment(const uint256& txid, unsigned int vout, const std::string& addressSeller, const std::string& addressBuyer, int64_t amountPaid, int block, uint64_t* nAmended)
{
    if (msc_debug_dex) PrintToLog("%s(%s, %s)\n", __func__, addressSeller, addressBuyer);

    int rc = DEX_ERROR_PAYMENT;

    uint32_t propertyId = OMNI_PROPERTY_MSC;
    CMPAccept* p_accept = nullptr;

    /**
     * When the feature is not activated, first check, if there is an open offer
     * for OMNI, and if not, check if there is an open offer for TOMNI.
     *
     * If the feature is activated, simply retrieve the token identifier of the
     * token for sale.
     */
    if (!IsFeatureActivated(FEATURE_FREEDEX, block)) {
        propertyId = OMNI_PROPERTY_MSC;  // test for OMNI accept first
        p_accept = DEx_getAccept(addressSeller, propertyId, addressBuyer);

        if (!p_accept) {
            propertyId = OMNI_PROPERTY_TMSC; // test for TOMNI accept second
            p_accept = DEx_getAccept(addressSeller, propertyId, addressBuyer);
        }
    } else {
        // Retrieve and get the token for sale for that seller

        if (DEx_getTokenForSale(addressSeller, propertyId)) {
            p_accept = DEx_getAccept(addressSeller, propertyId, addressBuyer);
        }
    }

    if (!p_accept) {
        // there must be an active accept order for this payment
        return (DEX_ERROR_PAYMENT -1);
    }

    // -------------------------------------------------------------------------

    const int64_t amountDesired = p_accept->getBTCDesiredOriginal();
    const int64_t amountOffered = p_accept->getOfferAmountOriginal();

    // divide by 0 protection
    if (0 == amountDesired) {
        if (msc_debug_dex) PrintToLog("%s: ERROR: desired amount of accept order is zero", __func__);

        return (DEX_ERROR_PAYMENT -2);
    }

    int64_t amountPurchased = 0;

    /**
     * As long as this feature is disabled, floating point math is used to
     * determine the purchased amount.
     *
     * After this feature is enabled, plain integer math is used to determine
     * the purchased amount. The purchased amount is rounded up, which may be
     * in favor of the buyer, to avoid small leftovers of 1 willet.
     *
     * This is not exploitable due to transaction fees.
     */
    if (IsFeatureActivated(FEATURE_DEXMATH, block)) {
        amountPurchased = calculateDExPurchase(amountOffered, amountDesired, amountPaid);
    } else {
        // Fallback to original calculation:
        amountPurchased = legacy::calculateDExPurchase(amountOffered, amountDesired, amountPaid);
    }

    // -------------------------------------------------------------------------

    const int64_t amountRemaining = p_accept->getAcceptAmountRemaining(); // actual amount desired, in the Accept

    if (msc_debug_dex) PrintToLog(
            "%s: QTC desired: %s, offered amount: %s, amount to purchase: %s, amount remaining: %s\n", __func__,
            FormatDivisibleMP(amountDesired), FormatDivisibleMP(amountOffered),
            FormatDivisibleMP(amountPurchased), FormatDivisibleMP(amountRemaining));

    // if units_purchased is greater than what's in the Accept, the buyer gets only what's in the Accept
    if (amountRemaining < amountPurchased) {
        amountPurchased = amountRemaining;

        if (nAmended) *nAmended = amountPurchased;
    }

    if (amountPurchased > 0) {
        PrintToLog("%s: seller %s offered %s %s for %s QTC\n", __func__,
                addressSeller, FormatDivisibleMP(amountOffered), strMPProperty(propertyId), FormatDivisibleMP(amountDesired));
        PrintToLog("%s: buyer %s pays %s QTC to purchase %s %s\n", __func__,
                addressBuyer, FormatDivisibleMP(amountPaid), FormatDivisibleMP(amountPurchased), strMPProperty(propertyId));

        assert(update_tally_map(addressSeller, propertyId, -amountPurchased, ACCEPT_RESERVE));
        assert(update_tally_map(addressBuyer, propertyId, amountPurchased, BALANCE));

        bool valid = true;
        pDbTransactionList->recordPaymentTX(txid, valid, block, vout, propertyId, amountPurchased, addressBuyer, addressSeller);

        rc = 0;
        PrintToLog("#######################################################\n");
    }

    // reduce the amount of units still desired by the buyer and if 0 destroy the Accept order
    if (p_accept->reduceAcceptAmountRemaining_andIsZero(amountPurchased)) {
        const int64_t reserveSell = GetTokenBalance(addressSeller, propertyId, SELLOFFER_RESERVE);
        const int64_t reserveAccept = GetTokenBalance(addressSeller, propertyId, ACCEPT_RESERVE);

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
