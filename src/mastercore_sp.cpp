// Smart Properties & Crowd Sales

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "util.h"
#include "wallet.h"

#include <stdint.h>
#include <string.h>

#include <fstream>
#include <algorithm>

#include <vector>

#include <utility>
#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;
using namespace leveldb;

#include "mastercore.h"

using namespace mastercore;

#include "mastercore_sp.h"

unsigned int CMPSPInfo::updateSP(unsigned int propertyID, Entry const &info)
{
    // cannot update implied SP
    if (OMNI_PROPERTY_MSC == propertyID || OMNI_PROPERTY_TMSC == propertyID) {
      return propertyID;
    }

    std::string nextIdStr;
    unsigned int res = propertyID;

    Object spInfo = info.toJSON();

    // generate the SP id
    string spKey = (boost::format(FORMAT_BOOST_SPKEY) % propertyID).str();
    string spValue = write_string(Value(spInfo), false);
    string spPrevKey = (boost::format("blk-%s:sp-%d") % info.update_block.ToString() % propertyID).str();
    string spPrevValue;

    leveldb::WriteBatch commitBatch;
    
    leveldb::ReadOptions readOpts;
    readOpts.fill_cache = false;
    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;

    // if a value exists move it to the old key
    if (false == pDb->Get(readOpts, spKey, &spPrevValue).IsNotFound()) {
      commitBatch.Put(spPrevKey, spPrevValue);
    }
    commitBatch.Put(spKey, spValue);
    pDb->Write(writeOptions, &commitBatch);

    file_log("Updated LevelDB with SP data successfully\n");
    return res;
}

unsigned int CMPSPInfo::putSP(unsigned char ecosystem, Entry const &info)
{
    std::string nextIdStr;
    unsigned int res = 0;
    switch(ecosystem) {
    case OMNI_PROPERTY_MSC: // mastercoin ecosystem, MSC: 1, TMSC: 2, First available SP = 3
      res = next_spid++;
      break;
    case OMNI_PROPERTY_TMSC: // Test MSC ecosystem, same as above with high bit set
      res = next_test_spid++;
      break;
    default: // non standard ecosystem, ID's start at 0
      res = 0;
    }

    Object spInfo = info.toJSON();

    // generate the SP id
    string spKey = (boost::format(FORMAT_BOOST_SPKEY) % res).str();
    string spValue = write_string(Value(spInfo), false);
    string txIndexKey = (boost::format(FORMAT_BOOST_TXINDEXKEY) % info.txid.ToString() ).str();
    string txValue = (boost::format("%d") % res).str();

    // sanity checking
    string existingEntry;
    leveldb::ReadOptions readOpts;
    readOpts.fill_cache = true;
    if (false == pDb->Get(readOpts, spKey, &existingEntry).IsNotFound() && false == boost::equals(spValue, existingEntry)) {
      file_log("%s WRITING SP %d TO LEVELDB WHEN A DIFFERENT SP ALREADY EXISTS FOR THAT ID!!!\n", __FUNCTION__, res);
    } else if (false == pDb->Get(readOpts, txIndexKey, &existingEntry).IsNotFound() && false == boost::equals(txValue, existingEntry)) {
      file_log("%s WRITING INDEX TXID %s : SP %d IS OVERWRITING A DIFFERENT VALUE!!!\n", __FUNCTION__, info.txid.ToString().c_str(), res);
    }

    // atomically write both the the SP and the index to the database
    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;

    leveldb::WriteBatch commitBatch;
    commitBatch.Put(spKey, spValue);
    commitBatch.Put(txIndexKey, txValue);

    pDb->Write(writeOptions, &commitBatch);
    return res;
}


bool CMPSPInfo::getSP(unsigned int spid, Entry &info)
{
    // special cases for constant SPs MSC and TMSC
    if (OMNI_PROPERTY_MSC == spid) {
      info = implied_msc;
      return true;
    } else if (OMNI_PROPERTY_TMSC == spid) {
      info = implied_tmsc;
      return true;
    }

    leveldb::ReadOptions readOpts;
    readOpts.fill_cache = true;

    string spKey = (boost::format(FORMAT_BOOST_SPKEY) % spid).str();
    string spInfoStr;
    if (false == pDb->Get(readOpts, spKey, &spInfoStr).ok()) {
      return false;
    }

    // parse the encoded json, failing if it doesnt parse or is an object
    Value spInfoVal;
    if (false == read_string(spInfoStr, spInfoVal) || spInfoVal.type() != obj_type ) {
      return false;
    }

    // transfer to the Entry structure
    Object &spInfo = spInfoVal.get_obj();
    info.fromJSON(spInfo);
    return true;
}

bool CMPSPInfo::hasSP(unsigned int spid)
{
    // special cases for constant SPs MSC and TMSC
    if (OMNI_PROPERTY_MSC == spid || OMNI_PROPERTY_TMSC == spid) {
      return true;
    }

    leveldb::ReadOptions readOpts;
    readOpts.fill_cache = true;

    string spKey = (boost::format(FORMAT_BOOST_SPKEY) % spid).str();
    leveldb::Iterator *iter = pDb->NewIterator(readOpts);
    iter->Seek(spKey);
    bool res = (iter->Valid() && iter->key().compare(spKey) == 0);
    // clean up the iterator
    delete iter;

    return res;
}

unsigned int CMPSPInfo::findSPByTX(uint256 const &txid)
{
    unsigned int res = 0;
    leveldb::ReadOptions readOpts;
    readOpts.fill_cache = true;

    string txIndexKey = (boost::format(FORMAT_BOOST_TXINDEXKEY) % txid.ToString() ).str();
    string spidStr;
    if (pDb->Get(readOpts, txIndexKey, &spidStr).ok()) {
      res = boost::lexical_cast<unsigned int>(spidStr);
    }

    return res;
}

int CMPSPInfo::popBlock(uint256 const &block_hash)
{
    int res = 0;
    int remainingSPs = 0;
    leveldb::WriteBatch commitBatch;

    leveldb::ReadOptions readOpts;
    readOpts.fill_cache = false;
    leveldb::Iterator *iter = pDb->NewIterator(readOpts);
    for (iter->Seek("sp-"); iter->Valid() && iter->key().starts_with("sp-"); iter->Next()) {
      // parse the encoded json, failing if it doesnt parse or is an object
      Value spInfoVal;
      if (read_string(iter->value().ToString(), spInfoVal) && spInfoVal.type() == obj_type ) {
        Entry info;
        info.fromJSON(spInfoVal.get_obj());
        if (info.update_block == block_hash) {
          // need to roll this SP back
          if (info.update_block == info.creation_block) {
            // this is the block that created this SP, so delete the SP and the tx index entry
            string txIndexKey = (boost::format(FORMAT_BOOST_TXINDEXKEY) % info.txid.ToString() ).str();
            commitBatch.Delete(iter->key());
            commitBatch.Delete(txIndexKey);
          } else {
            std::vector<std::string> vstr;
            std::string key = iter->key().ToString();
            boost::split(vstr, key, boost::is_any_of("-"), token_compress_on);
            unsigned int propertyID = boost::lexical_cast<unsigned int>(vstr[1]);

            string spPrevKey = (boost::format("blk-%s:sp-%d") % info.update_block.ToString() % propertyID).str();
            string spPrevValue;

            if (false == pDb->Get(readOpts, spPrevKey, &spPrevValue).IsNotFound()) {
              // copy the prev state to the current state and delete the old state
              commitBatch.Put(iter->key(), spPrevValue);
              commitBatch.Delete(spPrevKey);
              remainingSPs++;
            } else {
              // failed to find a previous SP entry, trigger reparse
              res = -1;
            }
          }
        } else {
          remainingSPs++;
        }
      } else {
        // failed to parse the db json, trigger reparse
        res = -1;
      }
    }

    // clean up the iterator
    delete iter;

    leveldb::WriteOptions writeOptions;
    writeOptions.sync = true;

    pDb->Write(writeOptions, &commitBatch);

    if (res < 0) {
      return res;
    } else {
      return remainingSPs;
    }
}

CMPCrowd *mastercore::getCrowd(const string & address)
{
CrowdMap::iterator my_it = my_crowds.find(address);

  if (my_it != my_crowds.end()) return &(my_it->second);

  return (CMPCrowd *) NULL;
}

bool mastercore::isPropertyDivisible(unsigned int propertyId)
{
// TODO: is a lock here needed
CMPSPInfo::Entry sp;

  if (_my_sps->getSP(propertyId, sp)) return sp.isDivisible();

  return true;
}

string mastercore::getPropertyName(unsigned int propertyId)
{
  CMPSPInfo::Entry sp;
  if (_my_sps->getSP(propertyId, sp)) return sp.name;
  return "Property Name Not Found";
}

bool mastercore::isCrowdsaleActive(unsigned int propertyId)
{
  for(CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it)
  {
      CMPCrowd crowd = it->second;
      unsigned int foundPropertyId = crowd.getPropertyId();
      if (foundPropertyId == propertyId) return true;
  }
  return false;
}

// save info from the crowdsale that's being erased
void mastercore::dumpCrowdsaleInfo(const string &address, CMPCrowd &crowd, bool bExpired)
{
  boost::filesystem::path pathInfo = GetDataDir() / INFO_FILENAME;
  FILE *fp = fopen(pathInfo.string().c_str(), "a");

  if (!fp)
  {
    file_log("\nPROBLEM writing %s, errno= %d\n", INFO_FILENAME, errno);
    return;
  }

  fprintf(fp, "\n%s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());
  fprintf(fp, "\nCrowdsale ended: %s\n", bExpired ? "Expired" : "Was closed");

  crowd.print(address, fp);

  fflush(fp);
  fclose(fp);
}

// calculates and returns fundraiser bonus, issuer premine, and total tokens
// propType : divisible/indiv
// bonusPerc: bonus percentage
// currentSecs: number of seconds of current tx
// numProps: number of properties
// issuerPerc: percentage of tokens to issuer
int mastercore::calculateFractional(unsigned short int propType, unsigned char bonusPerc, uint64_t fundraiserSecs, 
  uint64_t numProps, unsigned char issuerPerc, const std::map<std::string, std::vector<uint64_t> > txFundraiserData, 
  const uint64_t amountPremined  )
{

  //initialize variables
  double totalCreated = 0;
  double issuerPercentage = (double) (issuerPerc * 0.01);

  std::map<std::string, std::vector<uint64_t> >::const_iterator it;

  //iterate through fundraiser data
  for(it = txFundraiserData.begin(); it != txFundraiserData.end(); it++) {

    // grab the seconds and amt transferred from this tx
    uint64_t currentSecs = it->second.at(1);
    double amtTransfer = it->second.at(0);

    //make calc for bonus given in sec
    uint64_t bonusSeconds = fundraiserSecs - currentSecs;
  
    //turn it into weeks
    double weeks = bonusSeconds / (double) 604800;
    
    //make it a %
    double ebPercentage = weeks * bonusPerc;
    double bonusPercentage = ( ebPercentage / 100 ) + 1;
  
    //init var
    double createdTokens;

    //if indiv or div, do different truncation
    if( MSC_PROPERTY_TYPE_DIVISIBLE == propType ) {
      //calculate tokens
      createdTokens = (amtTransfer/1e8) * (double) numProps * bonusPercentage ;
      
      //printf("prop 2: is %Lf, and %Lf \n", createdTokens, issuerTokens);
      
      //add totals up
      totalCreated += createdTokens;
    } else {
      //printf("amount xfer %Lf and props %f and bonus percs %Lf \n", amtTransfer, (double) numProps, bonusPercentage);
      
      //same here
      createdTokens = (uint64_t) ( (amtTransfer/1e8) * (double) numProps * bonusPercentage);
      
      totalCreated += createdTokens;
    }
  };

  // calculate premine
  double totalPremined = totalCreated * issuerPercentage;
  double missedTokens;

  // calculate based on div/indiv, truncation/not
  if( 2 == propType ) {
    missedTokens = totalPremined - amountPremined;
  } else {
    missedTokens = (uint64_t) (totalPremined - amountPremined);
  }

  //return value
  return missedTokens;
}


//go hunting for whether a simple send is a crowdsale purchase
//TODO !!!! horribly inefficient !!!! find a more efficient way to do this
bool mastercore::isCrowdsalePurchase(uint256 txid, string address, int64_t *propertyId, int64_t *userTokens, int64_t *issuerTokens)
{
//1. loop crowdsales (active/non-active) looking for issuer address
//2. loop those crowdsales for that address and check their participant txs in database

  //check for an active crowdsale to this address
  CMPCrowd *crowd;
  crowd = getCrowd(address);
  if (crowd)
  {
      unsigned int foundPropertyId = crowd->getPropertyId();
      std::map<std::string, std::vector<uint64_t> > database = crowd->getDatabase();
      std::map<std::string, std::vector<uint64_t> >::const_iterator it;
      for(it = database.begin(); it != database.end(); it++)
      {
          string tmpTxid = it->first; //uint256 txid = it->first;
          string compTxid = txid.GetHex().c_str(); //convert to string to compare since this is how stored in levelDB
          if (tmpTxid == compTxid)
          {
              int64_t tmpUserTokens = it->second.at(2);
              int64_t tmpIssuerTokens = it->second.at(3);
              *propertyId = foundPropertyId;
              *userTokens = tmpUserTokens;
              *issuerTokens = tmpIssuerTokens;
              return true;
          }
      }
   }

   //if we still haven't found txid, check non active crowdsales to this address
   int64_t tmpPropertyId;
   unsigned int nextSPID = _my_sps->peekNextSPID(1);
   unsigned int nextTestSPID = _my_sps->peekNextSPID(2);

   for (tmpPropertyId = 1; tmpPropertyId<nextSPID; tmpPropertyId++)
   {
       CMPSPInfo::Entry sp;
       if (false != _my_sps->getSP(tmpPropertyId, sp))
       {
           if (sp.issuer == address)
           {
               std::map<std::string, std::vector<uint64_t> > database = sp.historicalData;
               std::map<std::string, std::vector<uint64_t> >::const_iterator it;
               for(it = database.begin(); it != database.end(); it++)
               {
                   string tmpTxid = it->first; //uint256 txid = it->first;
                   string compTxid = txid.GetHex().c_str(); //convert to string to compare since this is how stored in levelDB
                   if (tmpTxid == compTxid)
                   {
                       int64_t tmpUserTokens = it->second.at(2);
                       int64_t tmpIssuerTokens = it->second.at(3);
                       *propertyId = tmpPropertyId;
                       *userTokens = tmpUserTokens;
                       *issuerTokens = tmpIssuerTokens;
                       return true;
                   }
               }
           }
       }
   }
   for (tmpPropertyId = TEST_ECO_PROPERTY_1; tmpPropertyId<nextTestSPID; tmpPropertyId++)
   {
       CMPSPInfo::Entry sp;
       if (false != _my_sps->getSP(tmpPropertyId, sp))
       {
           if (sp.issuer == address)
           {
               std::map<std::string, std::vector<uint64_t> > database = sp.historicalData;
               std::map<std::string, std::vector<uint64_t> >::const_iterator it;
               for(it = database.begin(); it != database.end(); it++)
               {
                   string tmpTxid = it->first; //uint256 txid = it->first;
                   string compTxid = txid.GetHex().c_str(); //convert to string to compare since this is how stored in levelDB
                   if (tmpTxid == compTxid)
                   {
                       int64_t tmpUserTokens = it->second.at(2);
                       int64_t tmpIssuerTokens = it->second.at(3);
                       *propertyId = tmpPropertyId;
                       *userTokens = tmpUserTokens;
                       *issuerTokens = tmpIssuerTokens;
                       return true;
                   }
               }
           }
       }
   }

  //didn't find anything, not a crowdsale purchase
  return false;
}

void mastercore::eraseMaxedCrowdsale(const string &address, uint64_t blockTime, int block)
{
    CrowdMap::iterator it = my_crowds.find(address);
    
    if (it != my_crowds.end()) {

      CMPCrowd &crowd = it->second;
      file_log("%s() FOUND MAXED OUT CROWDSALE from address= '%s', erasing...\n", __FUNCTION__, address.c_str());

      dumpCrowdsaleInfo(address, crowd);
      
      CMPSPInfo::Entry sp;
      
      //get sp from data struct
      _my_sps->getSP(crowd.getPropertyId(), sp);
      
      //get txdata
      sp.historicalData = crowd.getDatabase();
      sp.close_early = 1;
      sp.max_tokens = 1;
      sp.timeclosed = blockTime;
      
      //update SP with this data
      sp.update_block = chainActive[block]->GetBlockHash();
      _my_sps->updateSP(crowd.getPropertyId() , sp);
      
      //No calculate fractional calls here, no more tokens (at MAX)
      
      my_crowds.erase(it);
    }
}

unsigned int mastercore::eraseExpiredCrowdsale(CBlockIndex const * pBlockIndex)
{
const int64_t blockTime = pBlockIndex->GetBlockTime();
unsigned int how_many_erased = 0;
CrowdMap::iterator my_it = my_crowds.begin();

  while (my_crowds.end() != my_it)
  {
    // my_it->first = key
    // my_it->second = value

    CMPCrowd &crowd = my_it->second;

    if (blockTime > (int64_t)crowd.getDeadline())
    {
      file_log("%s() FOUND EXPIRED CROWDSALE from address= '%s', erasing...\n", __FUNCTION__, (my_it->first).c_str());

      // TODO: dump the info about this crowdsale being delete into a TXT file (JSON perhaps)
      dumpCrowdsaleInfo(my_it->first, my_it->second, true);
      
      // Begin calculate Fractional 
      CMPSPInfo::Entry sp;
      
      //get sp from data struct
      _my_sps->getSP(crowd.getPropertyId(), sp);

      //file_log("\nValues going into calculateFractional(): hexid %s earlyBird %d deadline %lu numProps %lu issuerPerc %d, issuerCreated %ld \n", sp.txid.GetHex().c_str(), sp.early_bird, sp.deadline, sp.num_tokens, sp.percentage, crowd.getIssuerCreated());

      //find missing tokens
      double missedTokens = calculateFractional(sp.prop_type,
                          sp.early_bird,
                          sp.deadline,
                          sp.num_tokens,
                          sp.percentage,
                          crowd.getDatabase(),
                          crowd.getIssuerCreated());


      //file_log("\nValues coming out of calculateFractional(): Total tokens, Tokens created, Tokens for issuer, amountMissed: issuer %s %lu %lu %lu %f\n",sp.issuer.c_str(), crowd.getUserCreated() + crowd.getIssuerCreated(), crowd.getUserCreated(), crowd.getIssuerCreated(), missedTokens);

      //get txdata
      sp.historicalData = crowd.getDatabase();
      sp.missedTokens = (int64_t) missedTokens;

      //update SP with this data
      sp.update_block = pBlockIndex->GetBlockHash();
     _my_sps->updateSP(crowd.getPropertyId() , sp);

      //update values
      update_tally_map(sp.issuer, crowd.getPropertyId(), missedTokens, BALANCE);
      //End
                     
      my_crowds.erase(my_it++);

      ++how_many_erased;
    }
    else my_it++;

  }

  return how_many_erased;
}

char *mastercore::c_strPropertyType(int i)
{
  switch (i)
  {
    case MSC_PROPERTY_TYPE_DIVISIBLE: return (char *) "divisible";
    case MSC_PROPERTY_TYPE_INDIVISIBLE: return (char *) "indivisible";
  }

  return (char *) "*** property type error ***";
}


