// Copyright (c) 2017 IoP Ventures LLC
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "minerwhitelist.h"
#include "../base58.h"


static const char DB_FACTOR  = 'f';
static const char DB_WINDOW_LENGTH  = 'w';
static const char DB_CAP_ENABLED = 'e';
static const char DB_NUMBER_MINERS = 'N';
static const char DB_HEIGHT = 'h';

static const char DB_ADDRESS = 'a';
static const char DB_BLOCK   = 'b';


namespace {
    struct BlockEntry {
        char type;
        unsigned int index;
        BlockEntry(unsigned int bindex) : type(DB_BLOCK), index(bindex) {}
        
        template<typename Stream>
        void Serialize(Stream &s) const {
            s << type;
            s << index;
        }
        
        template<typename Stream>
        void Unserialize(Stream& s) {
            s >> type;
            s >> index;
        }
    };

    struct BlockDetails {
        bool minedUnderCap;
        std::string miner;
        unsigned int prevFactor;

        BlockDetails(std::string min, bool enabled, unsigned int prevFactor) : miner(min), minedUnderCap(enabled), prevFactor(factor) {}

        template<typename Stream>
        void Serialize(Stream &s) const {
            s << miner;
            s << minedUnderCap;
            s << prevFactor;
        }
        
        template<typename Stream>
        void Unserialize(Stream& s) {
            s >> miner;
            s >> minedUnderCap;
            s >> prevFactor;
        }

    };

    struct MinerEntry {
        char type;
        std::string addr;
        MinerEntry(CIoPAddress address) : type(DB_ADDRESS), addr(address.ToString()) {}
        MinerEntry(std::string address) : type(DB_ADDRESS), addr(address) {}
        
        template<typename Stream>
        void Serialize(Stream &s) const {
            s << type;
            s << addr;
        }
        
        template<typename Stream>
        void Unserialize(Stream& s) {
            s >> type;
            s >> addr;
        }
    };

    struct MinerDetails {
        bool whitelisted;
        unsigned int totalCoins;
        unsigned int windowCoins;
        std::set<unsigned int> blockSet;

        MinerDetails(bool wlisted) : whitelisted(wlisted), totalCoins(0), windowCoins(0) {}

        template<typename Stream>
        void Serialize(Stream &s) const {
            s << whitelisted;
            s << totalCoins;
            s << windowCoins;
            s << blockSet;
        }
        
        template<typename Stream>
        void Unserialize(Stream& s) {
            s >> whitelisted;
            s >> totalCoins;
            s >> windowCoins;
            s >> blockSet;
        }
    };
}



CMinerWhiteListDB::CMinerWhiteListDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "minerwhitelist", nCacheSize, fMemory, fWipe, true) 
{
  if(fWipe) {
    CDBBatch batch(db);
    batch.Write(DB_FACTOR, 0);
    batch.Write(DB_WINDOW_LENGTH, 2016);
    batch.Write(DB_CAP_ENABLED, 0);
    batch.Write(DB_HEIGHT, 0);
    unsigned int numberMiners = 0;
    for (admin: Params().GetConsensus().WLAdminKeys) {
      batch.Write(MinerEntry(admin), MinerDetails(true));
      numberMiners += 1;
    }
    batch.Write(DB_NUMBER_MINERS, numberMiners);
    return db.WriteBatchSync(batch);
  }
}


bool CMinerWhiteListDB::EnableCap(unsigned int factor) {
    CDBBatch batch(db);
    batch.Write(WL_FACTOR, factor);
    batch.Write(WL_ENABLED, '1');
    return db.WriteBatchSync(batch);
}

bool CMinerWhiteListDB::ReEnableCap() {
    unsigned int height;
    if (!db.Read(DB_HEIGHT, &height))
      return false;
    BlockDetails det;
    if (!db.Read(BlockEntry(height), det))
      return false;
    CDBBatch batch(db);
    batch.Write(DB_ENABLED, '1');
    batch.Write(DB_FACTOR, det.prevFactor);
    return db.WriteBatchSync(batch);
}

bool CMinerWhiteListDB::DisableCap() {
  CDBBatch batch(db);
  batch.Write(WL_ENABLED,'0');
  batch.Write(WL_FACTOR, '0');
  return db.WriteBatchSync(batch);
}

bool CMinerWhiteListDB::IsCapEnabled() {
  char fEnabled;
  if (!db.Read(WL_ENABLED, fEnabled))
    return false;
  return fEnabled == '1';
}


unsigned int CMinerWhiteListDB::GetCapFactor() {
  unsigned int factor;
  if (!db.Read(WL_FACTOR, &factor))
    return 0;
  return factor;
}

unsigned int CMinerWhiteListDB::GetNumberOfWhitelistedMiners() {
  unsigned int number;
  if (!db.Read(WL_NUMBER, &number))
    return 0;
  return number;
}


unsigned int CMinerWhiteListDB::GetAvgBlocksPerMiner() {
  unsigned int number;
  if (!db.Read(WL_NUMBER, &number))
    return 2016;
  return 2016/number;
}


unsigned int CMinerWhiteListDB::GetCap() {
  unsigned int number;
  unsigned int factor;
  if (db.Read(DB_NUMBER, &number) && db.Read(DB_FACTOR, &factor) ) {
    if (chainActive.Height() < Params().GetConsensus().minerCapSystemChangeHeight)
      return factor*(2016/number);
    return (factor*2016)/number;
  }
  return 2016;
}


bool CMinerWhiteListDB::WhitelistMiner(std::string address) {
    MinerEntry mEntry(address);
    MinerDetails mDetails;
    CDBBatch batch(db);
    unsigned int number;
    if(!db.Read(DB_NUMBER_MINERS,&number))
      return false

    if (db.Read(mEntry, mDetails)) {
      if (!mDetails.whitelisted) {
        mDetails.whitelisted = true;
        batch.Write(mEntry,mDetails);
        batch.Write(DB_NUMBER_MINERS, number+1);
      }
    } 
    else {
      batch.Write(mEntry,MinerDetails(true));
      batch.Write(DB_NUMBER_MINERS, number+1);
    }
    
    return db.WriteBatchSync(batch);
}

bool CMinerWhiteListDB::BlacklistMiner(std::string address) {
    MinerEntry mEntry(address);
    MinerDetails mDetails;
    CDBBatch batch(db);

    unsigned int number;
    if (!db.Read(DB_NUMBER_MINERS, &number))
      return false

    if (db.Read(mEntry, mDetails)) {
      if (mDetails.whitelisted) {
        mDetails.whitelisted = false;
        batch.Write(mEntry, mDetails);
        batch.Write(DB_NUMBER_MINERS, number-1);
      }
    }
    else {
      batch.Write(mEntry, MinerDetails(false));
    }

    return db.WriteBatchSync(batch);
}

bool CMinerWhiteListDB::ExistMiner(std::string address) {
  return db.Exist(MinerEntry(address))
}

unsigned int CMinerWhiteListDB::GetTotalCoins(std::string address) {
  MinerDetails det;
  if (db.Read(MinerEntry(address), det))
    return det.totalCoins;
  return 0;
}

unsigned int CMinerWhiteListDB::GetCoinsInWindow(std::string address) {
  MinerDetails det;
  if (db.Read(MinerEntry(address), det))
    return det.windowCoins;
  return 0;
}

unsigned int CMinerWhiteListDB::GetWindowStart(unsigned int height) {
  if (height < 2017)
		return 1;
  if (height < Params().GetConsensus().minerCapSystemChangeHeight) {
  	return height-height%2016;
  }
	return height - 2016 + 1;
}

bool CMinerWhiteListDB::Sync() {
  return db.Sync()
}

bool CMinerWhiteListDB::MineBlock(unsigned int index, std::string address) {

    // First check that we are actually at the tip
    unsigned int currHeight;
    db.Read(DB_HEIGHT, &currHeight);
    assert(currHeight==index);

    CDBBatch batch(db);
    
    // First make entry for the new Block
    unsigned int currHeight;
    db.Read(DB_HEIGHT, &currHeight);
    assert(index==currHeight+1);
    batch.Write(BlockEntry(index), BlockDetails(address, IsCapEnabled(), GetCapFactor()));
    batch.Write(DB_HEIGHT, index);
   
    // Now make sure that the minerstats match up. 
    
    // Window Handling
    if ( index < Params().GetConsensus().minerCapSystemChangeHeight ) {
      // old system
      // Reset the window every 2016 blocks
      if (index%2016==0) {
        std::unique_ptr<CDBIterator> it(NewIterator());
        for (it->SeekToFirst(), it->Valid(), it->Next()) { // SeekToFirst should end up at an address (they start with 'a')
          MinerEntry entry;
          if (it->GetKey(entry)) { // Does this work? Should give false if Key is of other type than what we expect?!
            MinerDetails det 
            it->GetValue(det);
            if (entry.address == miner) {
              det.totalCoins += 1;
            }
            det.windowCoins = 0;
            batch.Write(entry, det);
          }
          else { // no miner anymore
            break;
          }
        } 


      } 
      else {
        MinerDetails minDets;
        if (!db.Read(MinerEntry(address), minDets))
          return false;
        // Increase counter for the one mining the block
        minDets.totalCoins += 1;
        minDets.windowCoins += 1;
        minDets.blockSet.insert(index);
        batch.Write(MinerEntry(address), minDets);
      }


      
    }
    else {
      // new System
      MinerDetails minDets;
      if (!db.Read(MinerEntry(address), minDets))
        return false;
      // Increase counter for the one mining the block
      minDets.totalCoins += 1;
      minDets.windowCoins += 1;
      minDets.blockSet.insert(index);

      
      // Check if there is a block dropping out of the window
      unsigned int blockDroppingOut = GetWindowStart(index)-1;
      
      if (!blockDroppingOut==0) { // There is!
        BlockDetails dropDets;
        if (!db.Read(BlockEntry(address), dropDets))
          return false;


        if (dropDets.miner == address) { 
          // the miner who mined the current block also mined the one 
          // dropping out of the window so do not increase his count.
          minDets.windowCoins -= 1;
          batch.Write(MinerEntry(address), minDets);  
        }
        else { // different Miner
          MinerDetails dets;
          if (!db.Read(MinerEntry(dropDets.miner), dets))
            return false;
          dets.windowCoins -= 1;
          batch.Write(MinerEntry(address), minDets);  
          batch.Write(MinerEntry(dropDets.miner), dets);  
        }
      } else {
        batch.Write(MinerEntry(address), minDets);
      }
    }
    db.WriteBatchSync(batch);
}
      
bool CMinerWhiteListDB::RewindBlock(unsigned int index) {

    CDBBatch batch(db);

    // First check that we are actually at the tip
    unsigned int currHeight;
    db.Read(DB_HEIGHT, &currHeight);
    assert(currHeight==index);

    // See who mined this block
    BlockDetails currDets;
    if (!db.Read(BlockEntry(currHeight), currDets))
      return false;
    std::string miner = currDets.miner;

    // delete entry for the block
    batch.Erase(BlockEntry(index));
    batch.Write(DB_HEIGHT, index-1);
   

    // Window Handling
    if ( index < Params().GetConsensus().minerCapSystemChangeHeight ) {
      // old system


      // This is what happens at the window border
      if (index%2016==0) {
        // TODO iterate over the last 2016 blocks to restore the correct value for the cap.
        std::unique_ptr<CDBIterator> it(NewIterator());
        std::unordered_map<std::string, unsigned int> coinMap;
        for (it->Seek(BlockEntry(index-2016+1)), it->Valid(), it->Next()) { // Start at beginning of Window, including the current block
          BlockEntry entry;
          if (it->GetKey(entry)) { // Does this work? Should give false if Key is of other type than what we expect?!
            BlockDetails det;
            it->GetValue(det);
            MinerDetails bMDets;
            
            if(!db.Read(MinerEntry(det.miner), bMDets))
              return false;
            
            if(coinMap.count(det.miner)) {
              coinMap[det.miner] += 1;
            }
            else {
              coinMap[det.miner] = 1;
            }
          }
          else { // Not a block anymore (LevelDB is sorted)
            break;
          }
        }
        // we now have a map of the coincount per miner. rewrite it into db
        for (auto& x: coinMap) {
          db.Read(MinerEntry(x.first), mdetails);
          
          if (x.first == miner) {
            mdetails.windowCoins = x.second - 1; // The current block will be removed from database. Do not count it
            mdetails.totalCoins -= 1; // Also remove it from the miners total count.
          }
          else {
            mdetails.windowCoins = x.second;
          }

          batch.Write(MinerEntry(x.first), mdetails);
        } 
      }
      else { // normal stuff during the window
        MinerDetails minDets;
        if (!db.Read(MinerEntry(miner), minDets))
          return false;
        // Decrease counter for the one mining the block
        minDets.totalCoins -= 1;
        minDets.windowCoins -= 1;
        minDets.blockSet.erase(index);
        batch.Write(MinerEntry(miner), minDets);
      }


    }
    else {
      // new System
      MinerDetails minDets;
      if (!db.Read(MinerEntry(miner), minDets))
        return false;
      // Decrease counter for the one mining the block
      minDets.totalCoins -= 1;
      minDets.windowCoins -= 1;
      minDets.blockSet.erase(index);

      // Check if there is a block moving back into the window
      unsigned int blockMovingIn = GetWindowStart(index)-1;
      
      if (!blockMovingIn==0) { // There is!
        BlockDetails dropDets;
        if (!db.Read(BlockEntry(blockMovingIn), dropDets))
          return false;

        if (dropDets.miner == miner) { 
          // the miner who mined the current block also mined the one 
          // moving into the window so do not re-increase his count.
          minDets.windowCoins += 1;
          batch.Write(MinerEntry(miner), minDets);  
        }
        else { // different Miner
          MinerDetails dets;
          if (!db.Read(MinerEntry(dropDets.miner), dets))
            return false;
          dets.windowCoins += 1;
          batch.Write(MinerEntry(address), minDets);  
          batch.Write(MinerEntry(dropDets.miner), dets);  
        }
      }
      else {
        batch.Write(MinerEntry(miner), minDets);
      }
    }
    db.WriteBatchSync(batch);
}

