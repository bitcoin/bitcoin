// DEx & MetaDEx

#include "mastercore_dex.h"

#include "mastercore.h"
#include "mastercore_convert.h"
#include "mastercore_errors.h"
#include "mastercore_tx.h"

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


extern int msc_debug_metadex1, msc_debug_metadex2, msc_debug_metadex3;

md_PropertiesMap mastercore::metadex;

md_PricesMap* mastercore::get_Prices(unsigned int prop)
{
md_PropertiesMap::iterator it = metadex.find(prop);

  if (it != metadex.end()) return &(it->second);

  return (md_PricesMap *) NULL;
}

md_Set* mastercore::get_Indexes(md_PricesMap *p, XDOUBLE price)
{
md_PricesMap::iterator it = p->find(price);

  if (it != p->end()) return &(it->second);

  return (md_Set *) NULL;
}

enum MatchReturnType
{
  NOTHING             = 0,
  TRADED              = 1,
  TRADED_MOREINSELLER,
  TRADED_MOREINBUYER,
  ADDED,
  CANCELLED,
};

const string getTradeReturnType(MatchReturnType ret)
{
  switch (ret)
  {
    case NOTHING: return string("NOTHING");
    case TRADED: return string("TRADED");
    case TRADED_MOREINSELLER: return string("TRADED_MOREINSELLER");
    case TRADED_MOREINBUYER: return string("TRADED_MOREINBUYER");
    case ADDED: return string("ADDED");
    case CANCELLED: return string("CANCELLED");
    default: return string("* unknown *");
  }
}

bool operator==(XDOUBLE first, XDOUBLE second)
{
  return (first.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed) == second.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed));
}

bool operator!=(XDOUBLE first, XDOUBLE second)
{
  return !(first == second);
}

bool operator<=(XDOUBLE first, XDOUBLE second)
{
  return ((first.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed) < second.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed)) || (first == second));
}

bool operator>=(XDOUBLE first, XDOUBLE second)
{
  return ((first.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed) > second.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed)) || (first == second));
}

static void PriceCheck(const string &label, XDOUBLE left, XDOUBLE right)
{
const bool bOK = (left == right);

  file_log("PRICE CHECK %s: buyer = %s , inserted = %s : %s\n", label,
   left.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed),
   right.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed), bOK ? "good":"PROBLEM!");
}

// find the best match on the market
// NOTE: sometimes I refer to the older order as seller & the newer order as buyer, in this trade
// INPUT: property, desprop, desprice = of the new order being inserted; the new object being processed
// RETURN: 
static MatchReturnType x_Trade(CMPMetaDEx *newo)
{
    const CMPMetaDEx *p_older = NULL;
    md_PricesMap *prices = NULL;
    const unsigned int prop = newo->getProperty();
    const unsigned int desprop = newo->getDesProperty();
    MatchReturnType NewReturn = NOTHING;
    bool bBuyerSatisfied = false;
    const XDOUBLE buyersprice = newo->effectivePrice();
    const XDOUBLE desprice = (1/buyersprice); // inverse, to be matched against that of the existing older order

    if (msc_debug_metadex1) {
    file_log("%s(%s: prop=%u, desprop=%u, desprice= %s);newo: %s\n",
         __FUNCTION__, newo->getAddr(), prop, desprop, desprice.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed), newo->ToString());
    }

    prices = get_Prices(desprop);

    // nothing for the desired property exists in the market, sorry!
    if (!prices) {
        file_log("%s()=%u:%s NOT FOUND ON THE MARKET\n", __FUNCTION__, NewReturn, getTradeReturnType(NewReturn));
        return NewReturn;
    }

    // within the desired property map (given one property) iterate over the items looking at prices
    for (md_PricesMap::iterator my_it = prices->begin(); my_it != prices->end(); ++my_it) { // check all prices
        XDOUBLE sellers_price = (my_it->first);
        if (msc_debug_metadex2) file_log("comparing prices: desprice %s needs to be GREATER THAN OR EQUAL TO %s\n",
            desprice.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed), sellers_price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed));

        // Is the desired price check satisfied? The buyer's inverse price must be larger than that of the seller.
        if (desprice < sellers_price) continue;
        md_Set *indexes = &(my_it->second);

        // at good (single) price level and property iterate over offers looking at all parameters to find the match
        md_Set::iterator iitt;
        for (iitt = indexes->begin(); iitt != indexes->end();) { // specific price, check all properties
            p_older = &(*iitt);
            if (msc_debug_metadex1) file_log("Looking at existing: %s (its prop= %u, its des prop= %u) = %s\n",
                sellers_price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed), p_older->getProperty(), p_older->getDesProperty(), p_older->ToString());

            // is the desired property correct?
            if (p_older->getDesProperty() != prop) {
                ++iitt;
                continue;
            }
            if (msc_debug_metadex1) file_log("MATCH FOUND, Trade: %s = %s\n", sellers_price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed), p_older->ToString());

            // All Matched ! Trade now.
            // p_older is the old order pointer
            // newo is the new order pointer
            // the price in the older order is used
            const int64_t seller_amountForSale = p_older->getAmountForSale();
            const int64_t seller_amountWanted = p_older->getAmountDesired();
            const int64_t buyer_amountOffered = newo->getAmountForSale();
            if (msc_debug_metadex1) file_log("$$ trading using price: %s; seller: forsale= %ld, wanted= %ld, buyer amount offered= %ld\n",
                sellers_price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed), seller_amountForSale, seller_amountWanted, buyer_amountOffered);
            if (msc_debug_metadex1) file_log("$$ old: %s\n", p_older->ToString());
            if (msc_debug_metadex1) file_log("$$ new: %s\n", newo->ToString());
            int64_t seller_amountGot = seller_amountWanted;
            if (buyer_amountOffered < seller_amountWanted) {
                seller_amountGot = buyer_amountOffered;
            }
            const int64_t buyer_amountStillForSale = buyer_amountOffered - seller_amountGot;

            ///////////////////////////
            XDOUBLE x_buyer_got = (XDOUBLE) seller_amountGot / sellers_price;
            x_buyer_got += (XDOUBLE) 0.5; // ROUND UP
            std::string str_buyer_got = x_buyer_got.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed);
            std::string str_buyer_got_int_part = str_buyer_got.substr(0, str_buyer_got.find_first_of("."));
            const int64_t buyer_amountGot = boost::lexical_cast<int64_t>( str_buyer_got_int_part );
            const int64_t seller_amountLeft = p_older->getAmountForSale() - buyer_amountGot;
            if (msc_debug_metadex1) file_log("$$ buyer_got= %ld, seller_got= %ld, seller_left_for_sale= %ld, buyer_still_for_sale= %ld\n",
                buyer_amountGot, seller_amountGot, seller_amountLeft, buyer_amountStillForSale);
            XDOUBLE seller_amount_stilldesired = (XDOUBLE) seller_amountLeft * sellers_price;
            seller_amount_stilldesired += (XDOUBLE) 0.5; // ROUND UP
            std::string str_amount_stilldesired = seller_amount_stilldesired.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed);
            std::string str_stilldesired_int_part = str_amount_stilldesired.substr(0, str_amount_stilldesired.find_first_of("."));

            ///////////////////////////
            CMPMetaDEx seller_replacement = *p_older;
            seller_replacement.setAmountForSale(seller_amountLeft, "seller_replacement");
            seller_replacement.setAmountDesired(boost::lexical_cast<int64_t>( str_stilldesired_int_part ), "seller_replacement");

            // transfer the payment property from buyer to seller
            // TODO: do something when failing here............
            // FIXME
            // ...
            if (update_tally_map(newo->getAddr(), newo->getProperty(), - seller_amountGot, BALANCE)) {
                if (update_tally_map(p_older->getAddr(), p_older->getDesProperty(), seller_amountGot, BALANCE)) { } // ???
            }

            // transfer the market (the one being sold) property from seller to buyer
            // TODO: do something when failing here............
            // FIXME
            // ...
            if (update_tally_map(p_older->getAddr(), p_older->getProperty(), - buyer_amountGot, METADEX_RESERVE)) {
                update_tally_map(newo->getAddr(), newo->getDesProperty(), buyer_amountGot, BALANCE);
            }
            NewReturn = TRADED;
            XDOUBLE will_pay = (XDOUBLE) buyer_amountStillForSale * newo->effectivePrice();
            will_pay += (XDOUBLE) 0.5;  // ROUND UP
            std::string str_will_pay = will_pay.str(INTERNAL_PRECISION_LEN, std::ios_base::fixed);
            std::string str_will_pay_int_part = str_will_pay.substr(0, str_will_pay.find_first_of("."));
            newo->setAmountForSale(buyer_amountStillForSale, "buyer");
            newo->setAmountDesired(boost::lexical_cast<int64_t>( str_will_pay_int_part ), "buyer");
            if (0 < buyer_amountStillForSale) {
                NewReturn = TRADED_MOREINBUYER;
                PriceCheck(getTradeReturnType(NewReturn), buyersprice, newo->effectivePrice());
            } else {
                bBuyerSatisfied = true;
            }
            if (0 < seller_amountLeft) {  // done with all loops, update the seller, buyer is fully satisfied
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
                if (0 < seller_replacement.getAmountForSale()) {
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

void mastercore::MetaDEx_debug_print(bool bShowPriceLevel, bool bDisplay)
{
  file_log("<<<\n");
  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
  {
    unsigned int prop = my_it->first;

    file_log(" ## property: %u\n", prop);
    md_PricesMap & prices = my_it->second;

    for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
    {
      XDOUBLE price = (it->first);
      md_Set & indexes = (it->second);

      if (bShowPriceLevel) file_log("  # Price Level: %s\n", price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed));

      for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
      {
      CMPMetaDEx obj = *it;

        if (bDisplay) printf("%s= %s\n", price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed).c_str() , obj.ToString().c_str());
        else file_log("%s= %s\n", price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed) , obj.ToString());

        // extra checks: price or either of the amounts is 0
//        assert((XDOUBLE)0 != obj.effectivePrice());
//        assert(obj.getAmountForSale());
//        assert(obj.getAmountDesired());
      }
    }
  }
  file_log(">>>\n");
}

void CMPMetaDEx::Set(const string &sa, int b, unsigned int c, uint64_t nValue, unsigned int cd, uint64_t ad, const uint256 &tx, unsigned int i, unsigned char suba)
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
   effectivePrice().str(DISPLAY_PRECISION_LEN, std::ios_base::fixed),
   addr.c_str(), block, idx, txid.ToString().substr(0,10).c_str(),
   property, FormatMP(property, amount_forsale), desired_property, FormatMP(desired_property, amount_desired));
}

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
  if (msc_debug_dex) file_log("%s(%s, %u)\n", __FUNCTION__, seller_addr, prop);
const string combo = STR_SELLOFFER_ADDR_PROP_COMBO(seller_addr);
OfferMap::iterator my_it = my_offers.find(combo);

  if (my_it != my_offers.end()) return &(my_it->second);

  return (CMPOffer *) NULL;
}

// TODO: locks are needed around map's insert & erase
CMPAccept *mastercore::DEx_getAccept(const string &seller_addr, unsigned int prop, const string &buyer_addr)
{
  if (msc_debug_dex) file_log("%s(%s, %u, %s)\n", __FUNCTION__, seller_addr, prop, buyer_addr);
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
   file_log("%s(%s|%s), nValue=%lu)\n", __FUNCTION__, seller_addr, combo, nValue);

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
   file_log("%s(%s|%s)\n", __FUNCTION__, seller_addr, combo);

  return 0;
}

// returns 0 if everything is OK
int mastercore::DEx_offerUpdate(const string &seller_addr, unsigned int prop, uint64_t nValue, int block, uint64_t desired, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended)
{
int rc = DEX_ERROR_SELLOFFER;

  file_log("%s(%s, %d)\n", __FUNCTION__, seller_addr, prop);

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

  if (msc_debug_dex) file_log("%s(offer: %s)\n", __FUNCTION__, offer.getHash().GetHex());

  // here we ensure the correct BTC fee was paid in this acceptance message, per spec
  if (fee_paid < offer.getMinFee())
  {
    file_log("ERROR: fee too small -- the ACCEPT is rejected! (%lu is smaller than %lu)\n", fee_paid, offer.getMinFee());
    return DEX_ERROR_ACCEPT -105;
  }

  file_log("%s(%s) OFFER FOUND\n", __FUNCTION__, selloffer_combo);

  // the older accept is the valid one: do not accept any new ones!
  if (DEx_getAccept(seller, prop, buyer))
  {
    file_log("%s() ERROR: an accept from this same seller for this same offer is already open !!!!!\n", __FUNCTION__);
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
    file_log("%s() HASHES: offer=%s, accept=%s\n", __FUNCTION__, p_offer->getHash().GetHex(), p_accept->getHash().GetHex());

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

  if (msc_debug_dex) file_log("%s(%s, %s)\n", __FUNCTION__, seller, buyer);

  if (!p_accept) return (DEX_ERROR_PAYMENT -1);  // there must be an active Accept for this payment

  const double BTC_desired_original = p_accept->getBTCDesiredOriginal();
  const double offer_amount_original = p_accept->getOfferAmountOriginal();

  if (0==(double)BTC_desired_original) return (DEX_ERROR_PAYMENT -2);  // divide by 0 protection

  double perc_X = (double)BTC_paid/BTC_desired_original;
  double Purchased = offer_amount_original * perc_X;

  uint64_t units_purchased = rounduint64(Purchased);

  const uint64_t nActualAmount = p_accept->getAcceptAmountRemaining();  // actual amount desired, in the Accept

  if (msc_debug_dex)
   file_log("BTC_desired= %30.20lf , offer_amount=%30.20lf , perc_X= %30.20lf , Purchased= %30.20lf , units_purchased= %lu\n",
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

      file_log("#######################################################\n");
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
      file_log("%s() FOUND EXPIRED ACCEPT, erasing: blockNow=%d, offer block=%d, blocktimelimit= %d\n",
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

// pretty much directly linked to the ADD TX21 command off the wire
int mastercore::MetaDEx_ADD(const std::string& sender_addr, unsigned int prop, uint64_t amount, int block, unsigned int property_desired, uint64_t amount_desired, const uint256& txid, unsigned int idx)
{
int rc = METADEX_ERROR -1;

  // MetaDEx implementation phase 1 check
  if ((prop != OMNI_PROPERTY_MSC) && (property_desired != OMNI_PROPERTY_MSC) &&
   (prop != OMNI_PROPERTY_TMSC) && (property_desired != OMNI_PROPERTY_TMSC))
  {
    return METADEX_ERROR -800;
  }

    // store the data into the temp MetaDEx object here
    CMPMetaDEx new_mdex(sender_addr, block, prop, amount, property_desired, amount_desired, txid, idx, CMPTransaction::ADD);
    XDOUBLE neworder_buyersprice = new_mdex.effectivePrice();

    if (msc_debug_metadex1) file_log("%s(); buyer obj: %s\n", __FUNCTION__, new_mdex.ToString());

    // given the property & the price find the proper place for insertion

    // TODO: reconsider for boost::multiprecision
    // FIXME
    if (0 >= neworder_buyersprice)
    {
      // do not work with 0 prices
      return METADEX_ERROR -66;
    }

    if (msc_debug_metadex3) MetaDEx_debug_print();

    // TRADE, check matches, remainder of the order will be put into the order book
    x_Trade(&new_mdex);

    if (msc_debug_metadex3) MetaDEx_debug_print();

#if 0
    // if anything is left in the new order, INSERT
    if ((0 < new_mdex.getAmountForSale()) && (!disable_Combo))
    {
      x_AddOrCancel(&new_mdex); // straight match to ADD
    }
#endif

    if (msc_debug_metadex3) MetaDEx_debug_print();

    // plain insert
    if (0 < new_mdex.getAmountForSale())
    { // not added nor subtracted, insert as new or post-traded amounts
    md_PricesMap temp_prices, *p_prices = get_Prices(prop);
    md_Set temp_indexes, *p_indexes = NULL;
    std::pair<md_Set::iterator,bool> ret;

      if (p_prices)
      {
        p_indexes = get_Indexes(p_prices, neworder_buyersprice);
      }

      if (!p_indexes) p_indexes = &temp_indexes;

      ret = p_indexes->insert(new_mdex);

      if (false == ret.second)
      {
        file_log("%s() ERROR: ALREADY EXISTS, line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
      }
      else
      {
        // TODO: think about failure scenarios
        // FIXME
        if (update_tally_map(sender_addr, prop, - new_mdex.getAmountForSale(), BALANCE)) // subtract from what's available
        {
          // TODO: think about failure scenarios
          // FIXME
          update_tally_map(sender_addr, prop, new_mdex.getAmountForSale(), METADEX_RESERVE); // put in reserve
        }

        // price check
        PriceCheck("Insert", neworder_buyersprice, new_mdex.effectivePrice());

        if (msc_debug_metadex1) file_log("==== INSERTED: %s= %s\n", neworder_buyersprice.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed), new_mdex.ToString());
      }

      if (!p_prices) p_prices = &temp_prices;

      (*p_prices)[neworder_buyersprice] = *p_indexes;

      metadex[prop] = *p_prices;
    } // Must Insert

  rc = 0;

  if (msc_debug_metadex3) MetaDEx_debug_print();

  return rc;
}

int mastercore::MetaDEx_CANCEL_AT_PRICE(const uint256& txid, unsigned int block, const std::string& sender_addr, unsigned int prop, uint64_t amount, unsigned int property_desired, uint64_t amount_desired)
{
int rc = METADEX_ERROR -20;
CMPMetaDEx mdex(sender_addr, 0, prop, amount, property_desired, amount_desired, 0, 0, CMPTransaction::CANCEL_AT_PRICE);
md_PricesMap *prices = get_Prices(prop);
const CMPMetaDEx *p_mdex = NULL;

  if (msc_debug_metadex1) file_log("%s():%s\n", __FUNCTION__, mdex.ToString());

  if (msc_debug_metadex2) MetaDEx_debug_print();

  if (!prices)
  {
    file_log("%s() NOTHING FOUND for %s\n", __FUNCTION__, mdex.ToString());
    return rc -1;
  }

  // within the desired property map (given one property) iterate over the items
  for (md_PricesMap::iterator my_it = prices->begin(); my_it != prices->end(); ++my_it)
  {
  XDOUBLE sellers_price = (my_it->first);

    if (mdex.effectivePrice() != sellers_price) continue;

    md_Set *indexes = &(my_it->second);

    for (md_Set::iterator iitt = indexes->begin(); iitt != indexes->end();)
    { // for iitt
      p_mdex = &(*iitt);

      if (msc_debug_metadex3) file_log("%s(): %s\n", __FUNCTION__, p_mdex->ToString());

      if ((p_mdex->getDesProperty() != property_desired) || (p_mdex->getAddr() != sender_addr))
      {
        ++iitt;
        continue;
      }

      rc = 0;
      file_log("%s(): REMOVING %s\n", __FUNCTION__, p_mdex->ToString());

      // move from reserve to main
      update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), - p_mdex->getAmountForSale(), METADEX_RESERVE);
      update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), p_mdex->getAmountForSale(), BALANCE);

      // record the cancellation
      bool bValid = true;
      p_txlistdb->recordMetaDExCancelTX(txid, p_mdex->getHash(), bValid, block, p_mdex->getProperty(), p_mdex->getAmountForSale());

      indexes->erase(iitt++);
    }
  }

  if (msc_debug_metadex2) MetaDEx_debug_print();

  return rc;
}

int mastercore::MetaDEx_CANCEL_ALL_FOR_PAIR(const uint256& txid, unsigned int block, const std::string& sender_addr, unsigned int prop, unsigned int property_desired)
{
int rc = METADEX_ERROR -30;
md_PricesMap *prices = get_Prices(prop);
const CMPMetaDEx *p_mdex = NULL;

  file_log("%s(%d,%d)\n", __FUNCTION__, prop, property_desired);

  if (msc_debug_metadex3) MetaDEx_debug_print();

  if (!prices)
  {
    file_log("%s() NOTHING FOUND\n", __FUNCTION__);
    return rc -1;
  }

  // within the desired property map (given one property) iterate over the items
  for (md_PricesMap::iterator my_it = prices->begin(); my_it != prices->end(); ++my_it)
  {
  md_Set *indexes = &(my_it->second);

    for (md_Set::iterator iitt = indexes->begin(); iitt != indexes->end();)
    { // for iitt
      p_mdex = &(*iitt);

      if (msc_debug_metadex3) file_log("%s(): %s\n", __FUNCTION__, p_mdex->ToString());

      if ((p_mdex->getDesProperty() != property_desired) || (p_mdex->getAddr() != sender_addr))
      {
        ++iitt;
        continue;
      }

      rc = 0;
      file_log("%s(): REMOVING %s\n", __FUNCTION__, p_mdex->ToString());

      // move from reserve to main
      update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), - p_mdex->getAmountForSale(), METADEX_RESERVE);
      update_tally_map(p_mdex->getAddr(), p_mdex->getProperty(), p_mdex->getAmountForSale(), BALANCE);

      // record the cancellation
      bool bValid = true;
      p_txlistdb->recordMetaDExCancelTX(txid, p_mdex->getHash(), bValid, block, p_mdex->getProperty(), p_mdex->getAmountForSale());

      indexes->erase(iitt++);
    }
  }

  if (msc_debug_metadex3) MetaDEx_debug_print();

  return rc;
}

// scan the orderbook and remove everything for an address
int mastercore::MetaDEx_CANCEL_EVERYTHING(const uint256& txid, unsigned int block, const std::string& sender_addr, unsigned char ecosystem)
{
  int rc = METADEX_ERROR -40;

  file_log("%s()\n", __FUNCTION__);

  if (msc_debug_metadex2) MetaDEx_debug_print();

  file_log("<<<<<<\n");

  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
  {
    unsigned int prop = my_it->first;

    // skip property, if it is not in the expected ecosystem
    if (isMainEcosystemProperty(ecosystem) && !isMainEcosystemProperty(prop)) continue;
    if (isTestEcosystemProperty(ecosystem) && !isTestEcosystemProperty(prop)) continue;

    file_log(" ## property: %u\n", prop);
    md_PricesMap & prices = my_it->second;

    for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
    {
      XDOUBLE price = (it->first);
      md_Set & indexes = (it->second);

      file_log("  # Price Level: %s\n", price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed));

      for (md_Set::iterator it = indexes.begin(); it != indexes.end();)
      {
        file_log("%s= %s\n", price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed) , it->ToString());

        if (it->getAddr() != sender_addr)
        {
          ++it;
          continue;
        }

        rc = 0;
        file_log("%s(): REMOVING %s\n", __FUNCTION__, it->ToString());

        // move from reserve to balance
        update_tally_map(it->getAddr(), it->getProperty(), - it->getAmountForSale(), METADEX_RESERVE);
        update_tally_map(it->getAddr(), it->getProperty(), it->getAmountForSale(), BALANCE);

        // record the cancellation
        bool bValid = true;
        p_txlistdb->recordMetaDExCancelTX(txid, it->getHash(), bValid, block, it->getProperty(), it->getAmountForSale());

        indexes.erase(it++);
      }
    }
  }
  file_log(">>>>>>\n");

  if (msc_debug_metadex2) MetaDEx_debug_print();

  return rc;
}

bool MetaDEx_compare::operator()(const CMPMetaDEx &lhs, const CMPMetaDEx &rhs) const
{
  if (lhs.getBlock() == rhs.getBlock()) return lhs.getIdx() < rhs.getIdx();
  else return lhs.getBlock() < rhs.getBlock();
}

void CMPMetaDEx::saveOffer(std::ofstream &file, SHA256_CTX *shaCtx) const
{
    string lineOut = (boost::format("%s,%d,%d,%d,%d,%d,%d,%d,%s,%d")
      % addr
      % block
      % amount_forsale
      % property
      % amount_desired
      % desired_property
      % (unsigned int) subaction
      % idx
      % txid.ToString()
      % still_left_forsale
      ).str();

    // add the line to the hash
    SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << std::endl;
}

