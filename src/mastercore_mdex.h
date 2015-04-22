#ifndef MASTERCORE_MDEX_H
#define MASTERCORE_MDEX_H

#include "mastercore_log.h"

#include "chain.h"
#include "main.h"
#include "uint256.h"

#include <boost/multiprecision/cpp_dec_float.hpp>

#include <openssl/sha.h>

#include <stdint.h>

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>

using std::ofstream;
using std::string;

using boost::multiprecision::cpp_dec_float_100;

typedef cpp_dec_float_100 XDOUBLE;

#define DISPLAY_PRECISION_LEN  50
#define INTERNAL_PRECISION_LEN 50

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

namespace mastercore
{
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

int MetaDEx_ADD(const std::string& sender_addr, unsigned int, uint64_t, int block, unsigned int property_desired, uint64_t amount_desired, const uint256& txid, unsigned int idx);
int MetaDEx_CANCEL_AT_PRICE(const uint256&, unsigned int, const std::string&, unsigned int, uint64_t, unsigned int, uint64_t);
int MetaDEx_CANCEL_ALL_FOR_PAIR(const uint256&, unsigned int, const std::string&, unsigned int, unsigned int);
int MetaDEx_CANCEL_EVERYTHING(const uint256& txid, unsigned int block, const std::string& sender_addr, unsigned char ecosystem);
md_PricesMap *get_Prices(unsigned int prop);
md_Set *get_Indexes(md_PricesMap *p, XDOUBLE price);

void MetaDEx_debug_print(bool bShowPriceLevel = false, bool bDisplay = false);
}

#endif // MASTERCORE_MDEX_H

