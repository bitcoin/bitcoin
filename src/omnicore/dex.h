#ifndef OMNICORE_DEX_H
#define OMNICORE_DEX_H

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/tx.h"

#include "amount.h"
#include "tinyformat.h"
#include "uint256.h"

#include <openssl/sha.h>

#include <stdint.h>
#include <fstream>
#include <map>
#include <string>

/** Lookup key to find DEx offers. */
inline std::string STR_SELLOFFER_ADDR_PROP_COMBO(const std::string& address, uint32_t propertyId)
{
    return strprintf("%s-%d", address, propertyId);
}
/** Lookup key to find DEx accepts. */
inline std::string STR_ACCEPT_ADDR_PROP_ADDR_COMBO(const std::string& seller, const std::string& buyer, uint32_t propertyId)
{
    return strprintf("%s-%d+%s", seller, propertyId, buyer);
}
/** Lookup key to find DEx payments. */
inline std::string STR_PAYMENT_SUBKEY_TXID_PAYMENT_COMBO(const std::string& txidStr, unsigned int paymentNumber)
{
    return strprintf("%s-%d", txidStr, paymentNumber);
}
/** Lookup key to find MetaDEx sub-records. TODO: not here! */
inline std::string STR_REF_SUBKEY_TXID_REF_COMBO(const std::string& txidStr, unsigned int refNumber)
{
    return strprintf("%s%d", txidStr, refNumber);
}

/** A single outstanding offer, from one seller of one property.
 *
 * There many be more than one accepted offers.
 */
class CMPOffer
{
private:
    int offerBlock;
    //! amount of MSC for sale specified when the offer was placed
    int64_t offer_amount_original;
    uint32_t property;
    //! amount desired, in BTC
    int64_t BTC_desired_original;
    int64_t min_fee;
    uint8_t blocktimelimit;
    uint256 txid;
    uint8_t subaction;

public:
    uint256 getHash() const { return txid; }
    uint32_t getProperty() const { return property; }
    int64_t getMinFee() const { return min_fee ; }
    uint8_t getBlockTimeLimit() const { return blocktimelimit; }
    uint8_t getSubaction() const { return subaction; }

    int64_t getOfferAmountOriginal() const { return offer_amount_original; }
    int64_t getBTCDesiredOriginal() const { return BTC_desired_original; }

    CMPOffer()
      : offerBlock(0), offer_amount_original(0), property(0), BTC_desired_original(0), min_fee(0),
        blocktimelimit(0), subaction(0)
    {
    }

    CMPOffer(int block, int64_t amountOffered, uint32_t propertyId, int64_t amountDesired,
             int64_t minAcceptFee, uint8_t paymentWindow, const uint256& tx)
      : offerBlock(block), offer_amount_original(amountOffered), property(propertyId),
        BTC_desired_original(amountDesired), min_fee(minAcceptFee), blocktimelimit(paymentWindow),
        txid(tx), subaction(0)
    {
        if (msc_debug_dex) PrintToLog("%s(%d): %s\n", __func__, amountOffered, txid.GetHex());
    }

    CMPOffer(const CMPTransaction& tx)
      : offerBlock(tx.block), offer_amount_original(tx.nValue), property(tx.property),
        BTC_desired_original(tx.amount_desired), min_fee(tx.min_fee),
        blocktimelimit(tx.blocktimelimit), subaction(tx.subaction)
    {
    }

    void saveOffer(std::ofstream& file, SHA256_CTX* shaCtx, const std::string& address) const
    {
        std::string lineOut = strprintf("%s,%d,%d,%d,%d,%d,%d,%d,%s",
                address,
                offerBlock,
                offer_amount_original,
                property,
                BTC_desired_original,
                (OMNI_PROPERTY_BTC),
                min_fee,
                blocktimelimit,
                txid.ToString()
        );

        // add the line to the hash
        SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

        // write the line
        file << lineOut << std::endl;
    }
};

/** Accepted offer on the DEx.
 *
 * Once the accepted offer is seen on the network, the amount of MSC being purchased is
 * taken out of seller's "sell offer"-reserve and put into this buyer's "accept"-reserve.
 */
class CMPAccept
{
private:
    int64_t accept_amount_original;    // amount of MSC/TMSC desired to purchased
    int64_t accept_amount_remaining;   // amount of MSC/TMSC remaining to purchased
    uint8_t blocktimelimit;            // copied from the offer during creation
    uint32_t property;                 // copied from the offer during creation

    int64_t offer_amount_original;     // copied from the Offer during Accept's creation
    int64_t BTC_desired_original;      // copied from the Offer during Accept's creation

    // the original offers TXIDs, needed to match Accept to the Offer during Accept's destruction, etc.
    uint256 offer_txid;
    int block;

public:
    // TODO: it may not intuitive that this is *not* the hash of the accept order
    uint256 getHash() const { return offer_txid; }

    int64_t getOfferAmountOriginal() const { return offer_amount_original; }
    int64_t getBTCDesiredOriginal() const { return BTC_desired_original; }

    int64_t getAcceptAmount() const { return accept_amount_original; }

    uint8_t getBlockTimeLimit() const { return blocktimelimit; }
    uint32_t getProperty() const { return property; }

    int getAcceptBlock() const { return block; }

    CMPAccept(int64_t amountAccepted, int blockIn, uint8_t paymentWindow, uint32_t propertyId,
              int64_t offerAmountOriginal, int64_t amountDesired, const uint256& txid)
      : accept_amount_remaining(amountAccepted), blocktimelimit(paymentWindow),
        property(propertyId), offer_amount_original(offerAmountOriginal),
        BTC_desired_original(amountDesired), offer_txid(txid), block(blockIn)
    {
        accept_amount_original = accept_amount_remaining;
        PrintToLog("%s(%d): %s\n", __func__, amountAccepted, txid.GetHex());
    }

    CMPAccept(int64_t acceptAmountOriginal, int64_t acceptAmountRemaining, int blockIn,
              uint8_t paymentWindow, uint32_t propertyId, int64_t offerAmountOriginal,
              int64_t amountDesired, const uint256& txid)
      : accept_amount_original(acceptAmountOriginal),
        accept_amount_remaining(acceptAmountRemaining), blocktimelimit(paymentWindow),
        property(propertyId), offer_amount_original(offerAmountOriginal),
        BTC_desired_original(amountDesired), offer_txid(txid), block(blockIn)
    {
        PrintToLog("%s(%d[%d]): %s\n", __func__, acceptAmountRemaining, acceptAmountOriginal, txid.GetHex());
    }

    void print()
    {
        // TODO: no floating numbers
        PrintToLog("buying: %12.8lf (originally= %12.8lf) in block# %d\n",
                (double) accept_amount_remaining / (double) COIN, (double) accept_amount_original / (double) COIN, block);
    }

    int64_t getAcceptAmountRemaining() const
    {
        PrintToLog("%s(); buyer still wants = %d\n", __func__, accept_amount_remaining);

        return accept_amount_remaining;
    }

    /**
     * Reduce the remaining accepted amount.
     *
     * @param really_purchased  The number of units purchased
     * @return True if the customer is fully satisfied (nothing desired to be purchased)
     */
    bool reduceAcceptAmountRemaining_andIsZero(const int64_t really_purchased)
    {
        bool bRet = false;

        if (really_purchased >= accept_amount_remaining) bRet = true;

        accept_amount_remaining -= really_purchased;

        return bRet;
    }

    void saveAccept(std::ofstream& file, SHA256_CTX* shaCtx, const std::string& address, const std::string& buyer) const
    {
        std::string lineOut = strprintf("%s,%d,%s,%d,%d,%d,%d,%d,%d,%s",
                address,
                property,
                buyer,
                block,
                accept_amount_remaining,
                accept_amount_original,
                blocktimelimit,
                offer_amount_original,
                BTC_desired_original,
                offer_txid.ToString());

        // add the line to the hash
        SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

        // write the line
        file << lineOut << std::endl;
    }
};

namespace mastercore
{
typedef std::map<std::string, CMPOffer> OfferMap;
typedef std::map<std::string, CMPAccept> AcceptMap;

//! In-memory collection of DEx offers
extern OfferMap my_offers;
//! In-memory collection of DEx accepts
extern AcceptMap my_accepts;

/** Determines the amount of bitcoins desired, in case it needs to be recalculated. TODO: don't expose! */
int64_t calculateDesiredBTC(const int64_t amountOffered, const int64_t amountDesired, const int64_t amountAvailable);

bool DEx_offerExists(const std::string& addressSeller, uint32_t propertyId);
CMPOffer* DEx_getOffer(const std::string& addressSeller, uint32_t propertyId);
bool DEx_acceptExists(const std::string& addressSeller, uint32_t propertyId, const std::string& addressBuyer);
CMPAccept* DEx_getAccept(const std::string& addressSeller, uint32_t propertyId, const std::string& addressBuyer);
int DEx_offerCreate(const std::string& addressSeller, uint32_t propertyId, int64_t amountOffered, int block, int64_t amountDesired, int64_t minAcceptFee, uint8_t paymentWindow, const uint256& txid, uint64_t* nAmended = NULL);
int DEx_offerDestroy(const std::string& addressSeller, uint32_t propertyId);
int DEx_offerUpdate(const std::string& addressSeller, uint32_t propertyId, int64_t amountOffered, int block, int64_t amountDesired, int64_t minAcceptFee, uint8_t paymentWindow, const uint256& txid, uint64_t* nAmended = NULL);
int DEx_acceptCreate(const std::string& addressBuyer, const std::string& addressSeller, uint32_t propertyId, int64_t amountAccepted, int block, int64_t feePaid, uint64_t* nAmended = NULL);
int DEx_acceptDestroy(const std::string& addressBuyer, const std::string& addressSeller, uint32_t propertyid, bool fForceErase = false);
int DEx_payment(const uint256& txid, unsigned int vout, const std::string& addressSeller, const std::string& addressBuyer, int64_t amountPaid, int block, uint64_t* nAmended = NULL);

unsigned int eraseExpiredAccepts(int block);
}


#endif // OMNICORE_DEX_H
