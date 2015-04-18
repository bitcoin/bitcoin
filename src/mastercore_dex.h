#ifndef _MASTERCOIN_DEX
#define _MASTERCOIN_DEX 1

#include "mastercore.h"
#include "mastercore_log.h"

#include "main.h"
#include "tinyformat.h"
#include "uint256.h"

#include <boost/format.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#include <openssl/sha.h>

#include <stdint.h>

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>

using boost::multiprecision::cpp_dec_float_100;

using std::endl;
using std::ofstream;
using std::string;

typedef cpp_dec_float_100 XDOUBLE;


// this is the internal format for the offer primary key (TODO: replace by a class method)
#define STR_SELLOFFER_ADDR_PROP_COMBO(x) ( x + "-" + strprintf("%d", prop))
#define STR_ACCEPT_ADDR_PROP_ADDR_COMBO( _seller , _buyer ) ( _seller + "-" + strprintf("%d", prop) + "+" + _buyer)
#define STR_PAYMENT_SUBKEY_TXID_PAYMENT_COMBO(txidStr) ( txidStr + "-" + strprintf("%d", paymentNumber))
#define STR_REF_SUBKEY_TXID_REF_COMBO(txidStr) ( txidStr + strprintf("%d", refNumber))

#define DISPLAY_PRECISION_LEN  50
#define INTERNAL_PRECISION_LEN 50

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

// a metadex trade
class CMPMetaDEx
{
private:
  int block;
  uint256 txid;
  unsigned int idx; // index within the block
  unsigned int property;
  uint64_t amount_forsale; // the amount for sale specified when the offer was placed
  unsigned int desired_property;
  int64_t amount_desired;
  uint64_t still_left_forsale;
  unsigned char subaction;
  std::string addr;

public:
  uint256 getHash() const { return txid; }
  void setHash(uint256 hash) { txid = hash; }

  unsigned int getProperty() const { return property; }

  unsigned int getDesProperty() const { return desired_property; }
  const string & getAddr() const { return addr; }

  uint64_t getAmountForSale() const { return amount_forsale; }
  int64_t getAmountDesired() const { return amount_desired; }

  void setAmountForSale(int64_t ao, const string &label = "")
  {
    amount_forsale = ao;
    file_log("%s(%ld %s):%s\n", __FUNCTION__, ao, label, ToString());
  }

  void setAmountDesired(int64_t ad, const string &label = "")
  {
    amount_desired = ad;
    file_log("%s(%ld %s):%s\n", __FUNCTION__, ad, label, ToString());
  }

  unsigned char getAction() const { return subaction; }

  int getBlock() const { return block; }
  unsigned int getIdx() const { return idx; } 

  uint64_t getBlockTime() const
  { 
    CBlockIndex* pblockindex = chainActive[block];
    return pblockindex->GetBlockTime();  
  }

  // needed only by the RPC functions
  // needed only by the RPC functions
  CMPMetaDEx()
      : block(0), txid(0), idx(0), property(0), amount_forsale(0), desired_property(0), amount_desired(0),
        still_left_forsale(0), subaction(0)
  {
  }

  CMPMetaDEx(const std::string& addr, int b, unsigned int c, uint64_t nValue, unsigned int cd, uint64_t ad,
             const uint256& tx, unsigned int i, unsigned char suba, uint64_t lfors = 0)
      : block(b), txid(tx), idx(i), property(c), amount_forsale(nValue), desired_property(cd), amount_desired(ad),
        still_left_forsale(lfors), subaction(suba), addr(addr)
  {
  }

  void Set(const string &, int, unsigned int, uint64_t, unsigned int, uint64_t, const uint256 &, unsigned int, unsigned char);

  std::string ToString() const;

  XDOUBLE effectivePrice() const
  {
  XDOUBLE effective_price = 0;

    // I am the seller
    if (amount_forsale) effective_price = (XDOUBLE) amount_desired / (XDOUBLE) amount_forsale; // division by 0 check

    return (effective_price);
  }

  void saveOffer(ofstream &file, SHA256_CTX *shaCtx) const;
};

unsigned int eraseExpiredAccepts(int blockNow);

namespace mastercore
{
typedef std::map<string, CMPOffer> OfferMap;
typedef std::map<string, CMPAccept> AcceptMap;

extern OfferMap my_offers;
extern AcceptMap my_accepts;

typedef std::pair < uint64_t, uint64_t > MetaDExTypePrice; // the price split up into integer & fractional part for precision

class MetaDEx_compare
{
public:

  bool operator()(const CMPMetaDEx &lhs, const CMPMetaDEx &rhs) const;
};

// ---------------
typedef std::set < CMPMetaDEx , MetaDEx_compare > md_Set; // set of objects sorted by block+idx

// replaced double with float512 or float1024 // hitting the limit on trading 1 Satoshi for 100 BTC !!!
typedef std::map < XDOUBLE , md_Set > md_PricesMap;         // map of prices; there is a set of sorted objects for each price
typedef std::map < unsigned int, md_PricesMap > md_PropertiesMap; // map of properties; there is a map of prices for each property

extern md_PropertiesMap metadex;
// TODO: explore a property-pair, instead of a single property as map's key........
// ---------------

bool DEx_offerExists(const string &seller_addr, unsigned int);
CMPOffer *DEx_getOffer(const string &seller_addr, unsigned int);
CMPAccept *DEx_getAccept(const string &seller_addr, unsigned int, const string &buyer_addr);
int DEx_offerCreate(string seller_addr, unsigned int, uint64_t nValue, int block, uint64_t amount_desired, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended = NULL);
int DEx_offerDestroy(const string &seller_addr, unsigned int);
int DEx_offerUpdate(const string &seller_addr, unsigned int, uint64_t nValue, int block, uint64_t desired, uint64_t fee, unsigned char btl, const uint256 &txid, uint64_t *nAmended = NULL);
int DEx_acceptCreate(const string &buyer, const string &seller, int, uint64_t nValue, int block, uint64_t fee_paid, uint64_t *nAmended = NULL);
int DEx_acceptDestroy(const string &buyer, const string &seller, int, bool bForceErase = false);
int DEx_payment(uint256 txid, unsigned int vout, string seller, string buyer, uint64_t BTC_paid, int blockNow, uint64_t *nAmended = NULL);

int MetaDEx_ADD(const std::string& sender_addr, unsigned int, uint64_t, int block, unsigned int property_desired, uint64_t amount_desired, const uint256& txid, unsigned int idx);
int MetaDEx_CANCEL_AT_PRICE(const uint256&, unsigned int, const std::string&, unsigned int, uint64_t, unsigned int, uint64_t);
int MetaDEx_CANCEL_ALL_FOR_PAIR(const uint256&, unsigned int, const std::string&, unsigned int, unsigned int);
int MetaDEx_CANCEL_EVERYTHING(const uint256& txid, unsigned int block, const std::string& sender_addr, unsigned char ecosystem);
md_PricesMap *get_Prices(unsigned int prop);
md_Set *get_Indexes(md_PricesMap *p, XDOUBLE price);

void MetaDEx_debug_print(bool bShowPriceLevel = false, bool bDisplay = false);
}

#endif // #ifndef _MASTERCOIN_DEX

