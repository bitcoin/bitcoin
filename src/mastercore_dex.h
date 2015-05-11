#ifndef OMNICORE_DEX_H
#define OMNICORE_DEX_H 1

#include "mastercore.h"
#include "mastercore_log.h"

#include "amount.h"
#include "tinyformat.h"
#include "uint256.h"

#include <boost/format.hpp>

#include <openssl/sha.h>

#include <stdint.h>

#include <fstream>
#include <map>
#include <string>

using std::endl;
using std::ofstream;
using std::string;


// this is the internal format for the offer primary key (TODO: replace by a class method)
#define STR_SELLOFFER_ADDR_PROP_COMBO(x) ( x + "-" + strprintf("%d", prop))
#define STR_ACCEPT_ADDR_PROP_ADDR_COMBO( _seller , _buyer ) ( _seller + "-" + strprintf("%d", prop) + "+" + _buyer)
#define STR_PAYMENT_SUBKEY_TXID_PAYMENT_COMBO(txidStr) ( txidStr + "-" + strprintf("%d", paymentNumber))
#define STR_REF_SUBKEY_TXID_REF_COMBO(txidStr) ( txidStr + strprintf("%d", refNumber))

// a single outstanding offer -- from one seller of one property, internally may have many accepts
class CMPOffer
{
private:
  int offerBlock;
  uint64_t offer_amount_original; // the amount of MSC for sale specified when the offer was placed
  unsigned int property;
  uint64_t BTC_desired_original; // amount desired, in BTC
  uint64_t min_fee;
  unsigned char blocktimelimit;
  uint256 txid;
  unsigned char subaction;

public:
  uint256 getHash() const { return txid; }
  unsigned int getProperty() const { return property; }
  uint64_t getMinFee() const { return min_fee ; }
  unsigned char getBlockTimeLimit() const { return blocktimelimit; }
  unsigned char getSubaction() const { return subaction; }

  uint64_t getOfferAmountOriginal() const { return offer_amount_original; }
  uint64_t getBTCDesiredOriginal() const { return BTC_desired_original; }

  CMPOffer()
      : offerBlock(0), offer_amount_original(0), property(0), BTC_desired_original(0), min_fee(0), blocktimelimit(0),
        txid(0), subaction(0)
  {
  }

  CMPOffer(int b, uint64_t a, unsigned int cu, uint64_t d, uint64_t fee, unsigned char btl, const uint256& tx)
      : offerBlock(b), offer_amount_original(a), property(cu), BTC_desired_original(d), min_fee(fee), blocktimelimit(btl),
        txid(tx), subaction(0)
  {
    if (msc_debug_dex) file_log("%s(%lu): %s , line %d, file: %s\n", __FUNCTION__, a, txid.GetHex(), __LINE__, __FILE__);
  }

  void Set(uint64_t d, uint64_t fee, unsigned char btl, unsigned char suba)
  {
    BTC_desired_original = d;
    min_fee = fee;
    blocktimelimit = btl;
    subaction = suba;
  }

  void saveOffer(ofstream &file, SHA256_CTX *shaCtx, string const &addr ) const {
    // compose the outputline
    // seller-address, ...
    string lineOut = (boost::format("%s,%d,%d,%d,%d,%d,%d,%d,%s")
      % addr
      % offerBlock
      % offer_amount_original
      % property
      % BTC_desired_original
      % ( OMNI_PROPERTY_BTC )
      % min_fee
      % (int)blocktimelimit
      % txid.ToString()).str();

    // add the line to the hash
    SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << endl;
  }
};  // end of CMPOffer class

// do a map of buyers, primary key is buyer+property
// MUST account for many possible accepts and EACH property offer
class CMPAccept
{
private:
  uint64_t accept_amount_original;    // amount of MSC/TMSC desired to purchased
  uint64_t accept_amount_remaining;   // amount of MSC/TMSC remaining to purchased
// once accept is seen on the network the amount of MSC being purchased is taken out of seller's SellOffer-Reserve and put into this Buyer's Accept-Reserve
  unsigned char blocktimelimit;       // copied from the offer during creation
  unsigned int property;              // copied from the offer during creation

  uint64_t offer_amount_original; // copied from the Offer during Accept's creation
  uint64_t BTC_desired_original;  // copied from the Offer during Accept's creation

  uint256 offer_txid; // the original offers TXIDs, needed to match Accept to the Offer during Accept's destruction, etc.

public:
  uint256 getHash() const { return offer_txid; }

  uint64_t getOfferAmountOriginal() { return offer_amount_original; }
  uint64_t getBTCDesiredOriginal() { return BTC_desired_original; }

  int block;          // 'accept' message sent in this block

  unsigned char getBlockTimeLimit() { return blocktimelimit; }
  unsigned int getProperty() const { return property; }

  int getAcceptBlock()  { return block; }

  CMPAccept(uint64_t a, int b, unsigned char blt, unsigned int c, uint64_t o, uint64_t btc, const uint256 &txid):accept_amount_remaining(a),blocktimelimit(blt),property(c),
   offer_amount_original(o), BTC_desired_original(btc),offer_txid(txid),block(b)
  {
    accept_amount_original = accept_amount_remaining;
    file_log("%s(%lu), line %d, file: %s\n", __FUNCTION__, a, __LINE__, __FILE__);
  }

  CMPAccept(uint64_t origA, uint64_t remA, int b, unsigned char blt, unsigned int c, uint64_t o, uint64_t btc, const uint256 &txid):accept_amount_original(origA),accept_amount_remaining(remA),blocktimelimit(blt),property(c),
   offer_amount_original(o), BTC_desired_original(btc),offer_txid(txid),block(b)
  {
    file_log("%s(%lu[%lu]), line %d, file: %s\n", __FUNCTION__, remA, origA, __LINE__, __FILE__);
  }

  void print()
  {
    file_log("buying: %12.8lf (originally= %12.8lf) in block# %d\n",
     (double)accept_amount_remaining/(double)COIN, (double)accept_amount_original/(double)COIN, block);
  }

  uint64_t getAcceptAmountRemaining() const
  { 
    file_log("%s(); buyer still wants = %lu, line %d, file: %s\n", __FUNCTION__, accept_amount_remaining, __LINE__, __FILE__);

    return accept_amount_remaining;
  }

  // reduce accept_amount_remaining and return "true" if the customer is fully satisfied (nothing desired to be purchased)
  bool reduceAcceptAmountRemaining_andIsZero(const uint64_t really_purchased)
  {
  bool bRet = false;

    if (really_purchased >= accept_amount_remaining) bRet = true;

    accept_amount_remaining -= really_purchased;

    return bRet;
  }

  void saveAccept(ofstream &file, SHA256_CTX *shaCtx, string const &addr, string const &buyer ) const {
    // compose the outputline
    // seller-address, property, buyer-address, amount, fee, block
    string lineOut = (boost::format("%s,%d,%s,%d,%d,%d,%d,%d,%d,%s")
      % addr
      % property
      % buyer
      % block
      % accept_amount_remaining
      % accept_amount_original
      % (int)blocktimelimit
      % offer_amount_original
      % BTC_desired_original
      % offer_txid.ToString()).str();

    // add the line to the hash
    SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << endl;
  }

};  // end of CMPAccept class

unsigned int eraseExpiredAccepts(int blockNow);

namespace mastercore
{
typedef std::map<string, CMPOffer> OfferMap;
typedef std::map<string, CMPAccept> AcceptMap;

extern OfferMap my_offers;
extern AcceptMap my_accepts;

bool DEx_offerExists(const string &seller_addr, unsigned int);
CMPOffer *DEx_getOffer(const string &seller_addr, unsigned int);
CMPAccept *DEx_getAccept(const string &seller_addr, unsigned int, const string &buyer_addr);
int DEx_offerCreate(string seller_addr, unsigned int, uint64_t nValue, int block, uint64_t amount_desired, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended = NULL);
int DEx_offerDestroy(const string &seller_addr, unsigned int);
int DEx_offerUpdate(const string &seller_addr, unsigned int, uint64_t nValue, int block, uint64_t desired, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended = NULL);
int DEx_acceptCreate(const string &buyer, const string &seller, int, uint64_t nValue, int block, uint64_t fee_paid, uint64_t *nAmended = NULL);
int DEx_acceptDestroy(const string &buyer, const string &seller, int, bool bForceErase = false);
int DEx_payment(uint256 txid, unsigned int vout, string seller, string buyer, uint64_t BTC_paid, int blockNow, uint64_t *nAmended = NULL);
}


#endif // OMNICORE_DEX_H
