// DEx & MetaDEx

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

using boost::algorithm::token_compress_on;

using std::string;

using namespace mastercore;


// check to see if such a sell offer exists
bool mastercore::DEx_offerExists(const string &seller_addr, unsigned int prop)
{
//  if (msc_debug_dex) file_log("%s()\n", __FUNCTION__);
const string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr);
OfferMap::iterator my_it = my_offers.find(combo);

  return !(my_it == my_offers.end());
}

// getOffer may replace DEx_offerExists() in the near future
// TODO: locks are needed around map's insert & erase
CMPOffer *mastercore::DEx_getOffer(const string &seller_addr, unsigned int prop)
{
  if (msc_debug_dex) PrintToLog("%s(%s, %u)\n", __FUNCTION__, seller_addr, prop);
const string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr);
OfferMap::iterator my_it = my_offers.find(combo);

  if (my_it != my_offers.end()) return &(my_it->second);

  return (CMPOffer *) NULL;
}

// TODO: locks are needed around map's insert & erase
CMPAccept *mastercore::DEx_getAccept(const string &seller_addr, unsigned int prop, const string &buyer_addr)
{
  if (msc_debug_dex) PrintToLog("%s(%s, %u, %s)\n", __FUNCTION__, seller_addr, prop, buyer_addr);
const string combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(seller_addr, buyer_addr);
AcceptMap::iterator my_it = my_accepts.find(combo);

  if (my_it != my_accepts.end()) return &(my_it->second);

  return (CMPAccept *) NULL;
}

// returns 0 if everything is OK
int mastercore::DEx_offerCreate(string seller_addr, unsigned int prop, uint64_t nValue, int block, uint64_t amount_des, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended)
{
int rc = DEX_ERROR_SELLOFFER;

  // sanity check our params are OK
  if ((!btl) || (!amount_des)) return (DEX_ERROR_SELLOFFER -101); // time limit or amount desired empty

  if (DEx_getOffer(seller_addr, prop)) return (DEX_ERROR_SELLOFFER -10);  // offer already exists

  const string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr);

  if (msc_debug_dex)
   PrintToLog("%s(%s|%s), nValue=%lu)\n", __FUNCTION__, seller_addr, combo, nValue);

  const uint64_t balanceReallyAvailable = getMPbalance(seller_addr, prop, BALANCE);

  // if offering more than available -- put everything up on sale
  if (nValue > balanceReallyAvailable)
  {
  double BTC;

    // AND we must also re-adjust the BTC desired in this case...
    BTC = amount_des * balanceReallyAvailable;
    BTC /= (double)nValue;
    amount_des = rounduint64(BTC);

    nValue = balanceReallyAvailable;

    if (nAmended) *nAmended = nValue;
  }

  if (update_tally_map(seller_addr, prop, - nValue, BALANCE)) // subtract from what's available
  {
    update_tally_map(seller_addr, prop, nValue, SELLOFFER_RESERVE); // put in reserve

    my_offers.insert(std::make_pair(combo, CMPOffer(block, nValue, prop, amount_des, fee, btl, txid)));

    rc = 0;
  }

  return rc;
}

// returns 0 if everything is OK
int mastercore::DEx_offerDestroy(const string &seller_addr, unsigned int prop)
{
const uint64_t amount = getMPbalance(seller_addr, prop, SELLOFFER_RESERVE);

  if (!DEx_offerExists(seller_addr, prop)) return (DEX_ERROR_SELLOFFER -11); // offer does not exist

  const string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr);

  OfferMap::iterator my_it;

  my_it = my_offers.find(combo);

  if (amount)
  {
    update_tally_map(seller_addr, prop, amount, BALANCE);   // give back to the seller from SellOffer-Reserve
    update_tally_map(seller_addr, prop, - amount, SELLOFFER_RESERVE);
  }

  // delete the offer
  my_offers.erase(my_it);

  if (msc_debug_dex)
   PrintToLog("%s(%s|%s)\n", __FUNCTION__, seller_addr, combo);

  return 0;
}

// returns 0 if everything is OK
int mastercore::DEx_offerUpdate(const string &seller_addr, unsigned int prop, uint64_t nValue, int block, uint64_t desired, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended)
{
int rc = DEX_ERROR_SELLOFFER;

  PrintToLog("%s(%s, %d)\n", __FUNCTION__, seller_addr, prop);

  if (!DEx_offerExists(seller_addr, prop)) return (DEX_ERROR_SELLOFFER -12); // offer does not exist

  rc = DEx_offerDestroy(seller_addr, prop);

  if (!rc)
  {
    rc = DEx_offerCreate(seller_addr, prop, nValue, block, desired, fee, btl, txid, nAmended);
  }

  return rc;
}

// returns 0 if everything is OK
int mastercore::DEx_acceptCreate(const string &buyer, const string &seller, int prop, uint64_t nValue, int block, uint64_t fee_paid, uint64_t *nAmended)
{
int rc = DEX_ERROR_ACCEPT - 10;
OfferMap::iterator my_it;
const string selloffer_combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller);
const string accept_combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(seller, buyer);
uint64_t nActualAmount = getMPbalance(seller, prop, SELLOFFER_RESERVE);

  my_it = my_offers.find(selloffer_combo);

  if (my_it == my_offers.end()) return DEX_ERROR_ACCEPT -15;

  CMPOffer &offer = my_it->second;

  if (msc_debug_dex) PrintToLog("%s(offer: %s)\n", __FUNCTION__, offer.getHash().GetHex());

  // here we ensure the correct BTC fee was paid in this acceptance message, per spec
  if (fee_paid < offer.getMinFee())
  {
    PrintToLog("ERROR: fee too small -- the ACCEPT is rejected! (%lu is smaller than %lu)\n", fee_paid, offer.getMinFee());
    return DEX_ERROR_ACCEPT -105;
  }

  PrintToLog("%s(%s) OFFER FOUND\n", __FUNCTION__, selloffer_combo);

  // the older accept is the valid one: do not accept any new ones!
  if (DEx_getAccept(seller, prop, buyer))
  {
    PrintToLog("%s() ERROR: an accept from this same seller for this same offer is already open !!!!!\n", __FUNCTION__);
    return DEX_ERROR_ACCEPT -205;
  }

  if (nActualAmount > nValue)
  {
    nActualAmount = nValue;

    if (nAmended) *nAmended = nActualAmount;
  }

  // TODO: think if we want to save nValue -- as the amount coming off the wire into the object or not
  if (update_tally_map(seller, prop, - nActualAmount, SELLOFFER_RESERVE))
  {
    if (update_tally_map(seller, prop, nActualAmount, ACCEPT_RESERVE))
    {
      // insert into the map !
      my_accepts.insert(std::make_pair(accept_combo, CMPAccept(nActualAmount, block,
       offer.getBlockTimeLimit(), offer.getProperty(), offer.getOfferAmountOriginal(), offer.getBTCDesiredOriginal(), offer.getHash() )));

      rc = 0;
    }
  }

  return rc;
}

// this function is called by handler_block() for each Accept that has expired
// this function is also called when the purchase has been completed (the buyer bought everything he was allocated)
//
// returns 0 if everything is OK
int mastercore::DEx_acceptDestroy(const string &buyer, const string &seller, int prop, bool bForceErase)
{
int rc = DEX_ERROR_ACCEPT - 20;
CMPOffer *p_offer = DEx_getOffer(seller, prop);
CMPAccept *p_accept = DEx_getAccept(seller, prop, buyer);
bool bReturnToMoney; // return to BALANCE of the seller, otherwise return to SELLOFFER_RESERVE
const string accept_combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(seller, buyer);

  if (!p_accept) return rc; // sanity check

  const uint64_t nActualAmount = p_accept->getAcceptAmountRemaining();

  // if the offer is gone ACCEPT_RESERVE should go back to BALANCE
  if (!p_offer)
  {
    bReturnToMoney = true;
  }
  else
  {
    PrintToLog("%s() HASHES: offer=%s, accept=%s\n", __FUNCTION__, p_offer->getHash().GetHex(), p_accept->getHash().GetHex());

    // offer exists, determine whether it's the original offer or some random new one
    if (p_offer->getHash() == p_accept->getHash())
    {
      // same offer, return to SELLOFFER_RESERVE
      bReturnToMoney = false;
    }
    else
    {
      // old offer is gone !
      bReturnToMoney = true;
    }
  }

  if (bReturnToMoney)
  {
    if (update_tally_map(seller, prop, - nActualAmount, ACCEPT_RESERVE))
    {
      update_tally_map(seller, prop, nActualAmount, BALANCE);
      rc = 0;
    }
  }
  else
  {
    // return to SELLOFFER_RESERVE
    if (update_tally_map(seller, prop, - nActualAmount, ACCEPT_RESERVE))
    {
      update_tally_map(seller, prop, nActualAmount, SELLOFFER_RESERVE);
      rc = 0;
    }
  }

  // can only erase when is NOT called from an iterator loop
  if (bForceErase)
  {
  const AcceptMap::iterator my_it = my_accepts.find(accept_combo);

    if (my_accepts.end() !=my_it) my_accepts.erase(my_it);
  }

  return rc;
}

// incoming BTC payment for the offer
// TODO: verify proper partial payment handling
int mastercore::DEx_payment(uint256 txid, unsigned int vout, string seller, string buyer, uint64_t BTC_paid, int blockNow, uint64_t *nAmended)
{
//  if (msc_debug_dex) file_log("%s()\n", __FUNCTION__);
int rc = DEX_ERROR_PAYMENT;
CMPAccept *p_accept;
int prop;

prop = OMNI_PROPERTY_MSC; //test for MSC accept first
p_accept = DEx_getAccept(seller, prop, buyer);

  if (!p_accept) 
  {
    prop = OMNI_PROPERTY_TMSC; //test for TMSC accept second
    p_accept = DEx_getAccept(seller, prop, buyer); 
  }

  if (msc_debug_dex) PrintToLog("%s(%s, %s)\n", __FUNCTION__, seller, buyer);

  if (!p_accept) return (DEX_ERROR_PAYMENT -1);  // there must be an active Accept for this payment

  const double BTC_desired_original = p_accept->getBTCDesiredOriginal();
  const double offer_amount_original = p_accept->getOfferAmountOriginal();

  if (0==(double)BTC_desired_original) return (DEX_ERROR_PAYMENT -2);  // divide by 0 protection

  double perc_X = (double)BTC_paid/BTC_desired_original;
  double Purchased = offer_amount_original * perc_X;

  uint64_t units_purchased = rounduint64(Purchased);

  const uint64_t nActualAmount = p_accept->getAcceptAmountRemaining();  // actual amount desired, in the Accept

  if (msc_debug_dex)
   PrintToLog("BTC_desired= %30.20lf , offer_amount=%30.20lf , perc_X= %30.20lf , Purchased= %30.20lf , units_purchased= %lu\n",
   BTC_desired_original, offer_amount_original, perc_X, Purchased, units_purchased);

  // if units_purchased is greater than what's in the Accept, the buyer gets only what's in the Accept
  if (nActualAmount < units_purchased)
  {
    units_purchased = nActualAmount;

    if (nAmended) *nAmended = units_purchased;
  }

  if (update_tally_map(seller, prop, - units_purchased, ACCEPT_RESERVE))
  {
      update_tally_map(buyer, prop, units_purchased, BALANCE);
      rc = 0;
      bool bValid = true;
      p_txlistdb->recordPaymentTX(txid, bValid, blockNow, vout, prop, units_purchased, buyer, seller);

      PrintToLog("#######################################################\n");
  }

  // reduce the amount of units still desired by the buyer and if 0 must destroy the Accept
  if (p_accept->reduceAcceptAmountRemaining_andIsZero(units_purchased))
  {
  const uint64_t selloffer_reserve = getMPbalance(seller, prop, SELLOFFER_RESERVE);
  const uint64_t accept_reserve = getMPbalance(seller, prop, ACCEPT_RESERVE);

    DEx_acceptDestroy(buyer, seller, prop, true);

    // delete the Offer object if there is nothing in its Reserves -- everything got puchased and paid for
    if ((0 == selloffer_reserve) && (0 == accept_reserve))
    {
      DEx_offerDestroy(seller, prop);
    }
  }

  return rc;
}

unsigned int eraseExpiredAccepts(int blockNow)
{
unsigned int how_many_erased = 0;
AcceptMap::iterator my_it = my_accepts.begin();

  while (my_accepts.end() != my_it)
  {
    // my_it->first = key
    // my_it->second = value

    CMPAccept &mpaccept = my_it->second;

    if ((blockNow - mpaccept.block) >= (int) mpaccept.getBlockTimeLimit())
    {
      PrintToLog("%s() FOUND EXPIRED ACCEPT, erasing: blockNow=%d, offer block=%d, blocktimelimit= %d\n",
       __FUNCTION__, blockNow, mpaccept.block, mpaccept.getBlockTimeLimit());

      // extract the seller, buyer & property from the Key
      std::vector<std::string> vstr;
      boost::split(vstr, my_it->first, boost::is_any_of("-+"), token_compress_on);
      string seller = vstr[0];
      int property = atoi(vstr[1]);
      string buyer = vstr[2];

      DEx_acceptDestroy(buyer, seller, property);

      my_accepts.erase(my_it++);

      ++how_many_erased;
    }
    else my_it++;

  }

  return how_many_erased;
}

