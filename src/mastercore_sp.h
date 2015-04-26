#ifndef _MASTERCOIN_SP
#define _MASTERCOIN_SP 1

#include "mastercore.h"
#include "mastercore_log.h"
#include "mastercore_persistence.h"

class CBlockIndex;

#include "uint256.h"
#include "utiltime.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <openssl/sha.h>

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_value.h"

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#include <stdint.h>
#include <stdio.h>

#include <fstream>
#include <map>
#include <string>
#include <vector>

using boost::algorithm::token_compress_on;

using json_spirit::Object;
using json_spirit::Pair;
using json_spirit::Value;
using json_spirit::read_string;
using json_spirit::obj_type;

using std::endl;
using std::ofstream;
using std::string;

/** LevelDB based storage for currencies, smart properties and tokens.
 */
class CMPSPInfo : public CDBBase
{
public:
  struct Entry {
    // common SP data
    string issuer;
    unsigned short prop_type;
    unsigned int prev_prop_id;
    string category;
    string subcategory;
    string name;
    string url;
    string data;
    uint64_t num_tokens;

    // Crowdsale generated SP
    unsigned int property_desired;
    uint64_t deadline;
    unsigned char early_bird;
    unsigned char percentage;

    //We need this. Closedearly states if the SP was a crowdsale and closed due to MAXTOKENS or CLOSE command
    bool close_early;
    bool max_tokens;
    uint64_t  missedTokens;
    uint64_t timeclosed;
    uint256 txid_close;

    // other information
    uint256 txid;
    uint256 creation_block;
    uint256 update_block;
    bool fixed;
    bool manual;

    // for crowdsale properties, schema is 'txid:amtSent:deadlineUnix:userIssuedTokens:IssuerIssuedTokens;'
    // for manual properties, schema is 'txid:grantAmount:revokeAmount;'
    std::map<std::string, std::vector<uint64_t> > historicalData;
    
    Entry()
    : issuer()
    , prop_type(0)
    , prev_prop_id(0)
    , category()
    , subcategory()
    , name()
    , url()
    , data()
    , num_tokens(0)
    , property_desired(0)
    , deadline(0)
    , early_bird(0)
    , percentage(0)
    , close_early(0)
    , max_tokens(0)
    , missedTokens(0)
    , timeclosed(0)
    , txid_close()
    , txid()
    , creation_block()
    , update_block()
    , fixed(false)
    , manual(false)
    , historicalData()
    {
    }

    Object toJSON() const
    {
      Object spInfo;
      spInfo.push_back(Pair("issuer", issuer));
      spInfo.push_back(Pair("prop_type", prop_type));
      spInfo.push_back(Pair("prev_prop_id", (uint64_t)prev_prop_id));
      spInfo.push_back(Pair("category", category));
      spInfo.push_back(Pair("subcategory", subcategory));
      spInfo.push_back(Pair("name", name));
      spInfo.push_back(Pair("url", url));
      spInfo.push_back(Pair("data", data));
      spInfo.push_back(Pair("fixed", fixed));
      spInfo.push_back(Pair("manual", manual));

      spInfo.push_back(Pair("num_tokens", (boost::format("%d") % num_tokens).str()));
      if (false == fixed && false == manual) {
        spInfo.push_back(Pair("property_desired", (uint64_t)property_desired));
        spInfo.push_back(Pair("deadline", (boost::format("%d") % deadline).str()));
        spInfo.push_back(Pair("early_bird", (int)early_bird));
        spInfo.push_back(Pair("percentage", (int)percentage));

        spInfo.push_back(Pair("close_early", (int)close_early));
        spInfo.push_back(Pair("max_tokens", (int)max_tokens));
        spInfo.push_back(Pair("missedTokens", (int) missedTokens));
        spInfo.push_back(Pair("timeclosed", (boost::format("%d") % timeclosed).str()));
        spInfo.push_back(Pair("txid_close", (boost::format("%s") % txid_close.ToString()).str()));
      }

      //Initialize values
      std::map<std::string, std::vector<uint64_t> >::const_iterator it;
      
      std::string values_long = "";
      std::string values = "";

      //Iterate through fundraiser data, serializing it with txid:val:val:val:val;
      bool crowdsale = !fixed && !manual;
      for(it = historicalData.begin(); it != historicalData.end(); it++) {
         values += it->first.c_str();
         if (crowdsale) {
           values += ":" + boost::lexical_cast<std::string>(it->second.at(0));
           values += ":" + boost::lexical_cast<std::string>(it->second.at(1));
           values += ":" + boost::lexical_cast<std::string>(it->second.at(2));
           values += ":" + boost::lexical_cast<std::string>(it->second.at(3));
         } else if (manual) {
           values += ":" + boost::lexical_cast<std::string>(it->second.at(0));
           values += ":" + boost::lexical_cast<std::string>(it->second.at(1));
         }
         values += ";";
         values_long += values;
      }

      spInfo.push_back(Pair("historicalData", values_long));
      spInfo.push_back(Pair("txid", (boost::format("%s") % txid.ToString()).str()));
      spInfo.push_back(Pair("creation_block", (boost::format("%s") % creation_block.ToString()).str()));
      spInfo.push_back(Pair("update_block", (boost::format("%s") % update_block.ToString()).str()));
      return spInfo;
    }

    void fromJSON(Object const &json)
    {
      unsigned int idx = 0;
      issuer = json[idx++].value_.get_str();
      prop_type = (unsigned short)json[idx++].value_.get_int();
      prev_prop_id = (unsigned int)json[idx++].value_.get_uint64();
      category = json[idx++].value_.get_str();
      subcategory = json[idx++].value_.get_str();
      name = json[idx++].value_.get_str();
      url = json[idx++].value_.get_str();
      data = json[idx++].value_.get_str();
      fixed = json[idx++].value_.get_bool();
      manual = json[idx++].value_.get_bool();

      num_tokens = boost::lexical_cast<uint64_t>(json[idx++].value_.get_str());
      if (false == fixed && false == manual) {
        property_desired = (unsigned int)json[idx++].value_.get_uint64();
        deadline = boost::lexical_cast<uint64_t>(json[idx++].value_.get_str());
        early_bird = (unsigned char)json[idx++].value_.get_int();
        percentage = (unsigned char)json[idx++].value_.get_int();

        close_early = (unsigned char)json[idx++].value_.get_int();
        max_tokens = (unsigned char)json[idx++].value_.get_int();
        missedTokens = (unsigned char)json[idx++].value_.get_int();
        timeclosed = boost::lexical_cast<uint64_t>(json[idx++].value_.get_str());
        txid_close = uint256(json[idx++].value_.get_str());
      }

      //reconstruct database
      std::string longstr = json[idx++].value_.get_str();

      std::vector<std::string> strngs_vec;
      
      //split serialized form up
      boost::split(strngs_vec, longstr, boost::is_any_of(";"));

      //Go through and deserialize the database
      bool crowdsale = !fixed && !manual;
      for(std::vector<std::string>::size_type i = 0; i != strngs_vec.size(); i++) {
        if (strngs_vec[i].empty()) {
          continue;
        }
        
        std::vector<std::string> str_split_vec;
        boost::split(str_split_vec, strngs_vec[i], boost::is_any_of(":"));

        std::vector<uint64_t> txDataVec;

        if ( crowdsale && str_split_vec.size() == 5) {
          txDataVec.push_back(boost::lexical_cast<uint64_t>( str_split_vec.at(1) ));
          txDataVec.push_back(boost::lexical_cast<uint64_t>( str_split_vec.at(2) ));
          txDataVec.push_back(boost::lexical_cast<uint64_t>( str_split_vec.at(3) ));
          txDataVec.push_back(boost::lexical_cast<uint64_t>( str_split_vec.at(4) ));
        } else if (manual && str_split_vec.size() == 3) {
          txDataVec.push_back(boost::lexical_cast<uint64_t>( str_split_vec.at(1) ));
          txDataVec.push_back(boost::lexical_cast<uint64_t>( str_split_vec.at(2) ));
        }

        historicalData.insert(std::make_pair( str_split_vec.at(0), txDataVec ) ) ;
      }
      txid = uint256(json[idx++].value_.get_str());
      creation_block = uint256(json[idx++].value_.get_str());
      update_block = uint256(json[idx++].value_.get_str());
    }

    bool isDivisible() const
    {
      switch (prop_type)
      {
        case MSC_PROPERTY_TYPE_DIVISIBLE:
        case MSC_PROPERTY_TYPE_DIVISIBLE_REPLACING:
        case MSC_PROPERTY_TYPE_DIVISIBLE_APPENDING:
          return true;
      }
      return false;
    }

    void print() const
    {
      PrintToConsole("%s:%s(Fixed=%s,Divisible=%s):%lu:%s/%s, %s %s\n",
        issuer,
        name,
        fixed ? "Yes" : "No",
        isDivisible() ? "Yes" : "No",
        num_tokens,
        category, subcategory, url, data);
    }

  };

private:
  // implied version of msc and tmsc so they don't hit the leveldb
  Entry implied_msc;
  Entry implied_tmsc;

  unsigned int next_spid;
  unsigned int next_test_spid;

public:
  CMPSPInfo(const boost::filesystem::path& path, bool fWipe)
  {
    leveldb::Status status = Open(path, fWipe);
    PrintToConsole("Loading smart property database: %s\n", status.ToString());

    // special cases for constant SPs MSC and TMSC
    implied_msc.issuer = ExodusAddress();
    implied_msc.prop_type = MSC_PROPERTY_TYPE_DIVISIBLE;
    implied_msc.num_tokens = 700000;
    implied_msc.category = "N/A";
    implied_msc.subcategory = "N/A";
    implied_msc.name = "MasterCoin";
    implied_msc.url = "www.mastercoin.org";
    implied_msc.data = "***data***";
    implied_tmsc.issuer = ExodusAddress();
    implied_tmsc.prop_type = MSC_PROPERTY_TYPE_DIVISIBLE;
    implied_tmsc.num_tokens = 700000;
    implied_tmsc.category = "N/A";
    implied_tmsc.subcategory = "N/A";
    implied_tmsc.name = "Test MasterCoin";
    implied_tmsc.url = "www.mastercoin.org";
    implied_tmsc.data = "***data***";

    init();
  }

  virtual ~CMPSPInfo()
  {
    if (msc_debug_persistence) file_log("CMPSPInfo closed\n");
  }

  void init(unsigned int nextSPID = 0x3UL, unsigned int nextTestSPID = TEST_ECO_PROPERTY_1)
  {
    next_spid = nextSPID;
    next_test_spid = nextTestSPID;
  }

  unsigned int peekNextSPID(unsigned char ecosystem)
  {
    unsigned int nextId = 0;

    switch(ecosystem) {
    case OMNI_PROPERTY_MSC: // mastercoin ecosystem, MSC: 1, TMSC: 2, First available SP = 3
      nextId = next_spid;
      break;
    case OMNI_PROPERTY_TMSC: // Test MSC ecosystem, same as above with high bit set
      nextId = next_test_spid;
      break;
    default: // non standard ecosystem, ID's start at 0
      nextId = 0;
    }

    return nextId;
  }

  unsigned int updateSP(unsigned int propertyID, Entry const &info);
  unsigned int putSP(unsigned char ecosystem, Entry const &info);
  bool getSP(unsigned int spid, Entry &info);
  bool hasSP(unsigned int spid);

  unsigned int findSPByTX(uint256 const &txid);

  int popBlock(uint256 const &block_hash);

  static string const watermarkKey;
  void setWatermark(uint256 const &watermark)
  {
    leveldb::WriteBatch commitBatch;
    commitBatch.Delete(watermarkKey);
    commitBatch.Put(watermarkKey, watermark.ToString());
    pdb->Write(syncoptions, &commitBatch);
  }

  int getWatermark(uint256 &watermark)
  {
    string watermarkVal;
    if (pdb->Get(readoptions, watermarkKey, &watermarkVal).ok()) {
      watermark.SetHex(watermarkVal);
      return 0;
    } else {
      return -1;
    }
  }

  void printAll()
  {
    // print off the hard coded MSC and TMSC entries
    for (unsigned int idx = OMNI_PROPERTY_MSC; idx <= OMNI_PROPERTY_TMSC; idx++ ) {
      Entry info;
      PrintToConsole("%10d => ", idx);
      if (getSP(idx, info)) {
        info.print();
      } else {
        PrintToConsole("<Internal Error on implicit SP>\n");
      }
    }

    leveldb::Iterator *iter = NewIterator();
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
      if (iter->key().starts_with("sp-")) {
        std::vector<std::string> vstr;
        std::string key = iter->key().ToString();
        boost::split(vstr, key, boost::is_any_of("-"), token_compress_on);

        PrintToConsole("%10s => ", vstr[1]);

        // parse the encoded json, failing if it doesnt parse or is an object
        Value spInfoVal;
        if (read_string(iter->value().ToString(), spInfoVal) && spInfoVal.type() == obj_type ) {
          Entry info;
          info.fromJSON(spInfoVal.get_obj());
          info.print();
        } else {
          PrintToConsole("<Malformed JSON in DB>\n");
        }
      }
    }

    // clean up the iterator
    delete iter;
  }
};

// live crowdsales are these objects in a map
class CMPCrowd
{
private:
  unsigned int propertyId;

  uint64_t nValue;

  unsigned int property_desired;
  uint64_t deadline;
  unsigned char early_bird;
  unsigned char percentage;

  uint64_t u_created;
  uint64_t i_created;

  uint256 txid;  // NOTE: not persisted as it doesnt seem used

  std::map<std::string, std::vector<uint64_t> > txFundraiserData;  // schema is 'txid:amtSent:deadlineUnix:userIssuedTokens:IssuerIssuedTokens;'
public:
  CMPCrowd():propertyId(0),nValue(0),property_desired(0),deadline(0),early_bird(0),percentage(0),u_created(0),i_created(0)
  {
  }

  CMPCrowd(unsigned int pid, uint64_t nv, unsigned int cd, uint64_t dl, unsigned char eb, unsigned char per, uint64_t uct, uint64_t ict):
   propertyId(pid),nValue(nv),property_desired(cd),deadline(dl),early_bird(eb),percentage(per),u_created(uct),i_created(ict)
  {
  }

  unsigned int getPropertyId() const { return propertyId; }

  uint64_t getDeadline() const { return deadline; }
  uint64_t getCurrDes() const { return property_desired; }

  void incTokensUserCreated(uint64_t amount) { u_created += amount; }
  void incTokensIssuerCreated(uint64_t amount) { i_created += amount; }

  uint64_t getUserCreated() const { return u_created; }
  uint64_t getIssuerCreated() const { return i_created; }

  void insertDatabase(std::string txhash, std::vector<uint64_t> txdata ) { txFundraiserData.insert(std::make_pair<std::string, std::vector<uint64_t>& >(txhash,txdata)); }
  std::map<std::string, std::vector<uint64_t> > getDatabase() const { return txFundraiserData; }

  void print(const string & address, FILE *fp = stdout) const
  {
    fprintf(fp, "%34s : id=%u=%X; prop=%u, value= %lu, deadline: %s (%lX)\n", address.c_str(), propertyId, propertyId,
     property_desired, nValue, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", deadline).c_str(), deadline);
  }

  void saveCrowdSale(ofstream &file, SHA256_CTX *shaCtx, string const &addr) const
  {
    // compose the outputline
    // addr,propertyId,nValue,property_desired,deadline,early_bird,percentage,created,mined
    string lineOut = (boost::format("%s,%d,%d,%d,%d,%d,%d,%d,%d")
      % addr
      % propertyId
      % nValue
      % property_desired
      % deadline
      % (int)early_bird
      % (int)percentage
      % u_created
      % i_created ).str();

    // append N pairs of address=nValue;blockTime for the database
    std::map<std::string, std::vector<uint64_t> >::const_iterator iter;
    for (iter = txFundraiserData.begin(); iter != txFundraiserData.end(); ++iter) {
      lineOut.append((boost::format(",%s=") % (*iter).first).str());
      std::vector<uint64_t> const &vals = (*iter).second;

      std::vector<uint64_t>::const_iterator valIter;
      for (valIter = vals.begin(); valIter != vals.end(); ++valIter) {
        if (valIter != vals.begin()) {
          lineOut.append(";");
        }

        lineOut.append((boost::format("%d") % (*valIter)).str());
      }
    }

    // add the line to the hash
    SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

    // write the line
    file << lineOut << endl;
  }
};  // end of CMPCrowd class

namespace mastercore
{
extern CMPSPInfo *_my_sps;

typedef std::map<string, CMPCrowd> CrowdMap;

extern CrowdMap my_crowds;

char *c_strPropertyType(int i);

CMPCrowd *getCrowd(const string & address);

int calculateFractional(unsigned short int propType, unsigned char bonusPerc, uint64_t fundraiserSecs, 
  uint64_t numProps, unsigned char issuerPerc, const std::map<std::string, std::vector<uint64_t> > txFundraiserData, 
  const uint64_t amountPremined  );

void eraseMaxedCrowdsale(const string &address, uint64_t blockTime, int block);

unsigned int eraseExpiredCrowdsale(CBlockIndex const * pBlockIndex);

void dumpCrowdsaleInfo(const string &address, CMPCrowd &crowd, bool bExpired = false);
}

#endif // _MASTERCOIN_SP

