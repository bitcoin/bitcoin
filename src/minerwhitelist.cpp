// Copyright (c) 2017 IoP Ventures LLC
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "minerwhitelist.h"
#include "../base58.h" 
#include "chain.h"
#include "util.h" 
#include <unordered_map>

// CChain chainActive;

static const std::string DUMMY = "0000000000000000000000000000000000"; 

static const char WL_FACTOR  = 'f';
static const char WL_WINDOW_LENGTH  = 'w';
static const char WL_CAP_ENABLED = 'e';
static const char WL_NUMBER_MINERS = 'N';
static const char WL_ADDRESS = 'a';

static const char DB_HEIGHT = 'h';
static const char DB_BLOCK   = 'b';
static const char DB_INIT  = 'I';
// static const char DB_REINDEX_FLAG = 'R';


namespace {
    struct BlockEntry {
        char key;
        unsigned int index;
        BlockEntry(): key(DB_BLOCK), index(0) {}
        BlockEntry(unsigned int bindex) : key(DB_BLOCK), index(bindex) {}
        
        template<typename Stream>
        void Serialize(Stream &s) const {
            s << key;
            s << index;
        }
        
        template<typename Stream>
        void Unserialize(Stream& s) {
            s >> key;
            s >> index;
        }
    };

    struct BlockDetails {
        bool minedUnderCap;
        std::string miner;
        unsigned int prevFactor;
        BlockDetails(): minedUnderCap(false), miner(DUMMY), prevFactor(0) {}
        BlockDetails(std::string min, bool enabled, unsigned int prevFactor) : minedUnderCap(enabled), miner(min), prevFactor(prevFactor) {}

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
        MinerEntry() : type(WL_ADDRESS), addr(DUMMY) {}
        MinerEntry(CIoPAddress address) : type(WL_ADDRESS), addr(address.ToString()) {}
        MinerEntry(std::string address) : type(WL_ADDRESS), addr(address) {}
        
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
        bool isAdmin;
        unsigned int totalBlocks;
        unsigned int windowBlocks;
        std::vector<unsigned int> blockVector;
        MinerDetails() : whitelisted(false), isAdmin(false), totalBlocks(0), windowBlocks(0) {}
        MinerDetails(bool wlisted, bool isAdmin) : whitelisted(wlisted), isAdmin(isAdmin), totalBlocks(0), windowBlocks(0) {}

        template<typename Stream>
        void Serialize(Stream &s) const {
            s << whitelisted;
            s << totalBlocks;
            s << windowBlocks;
            s << blockVector;
        }
        
        template<typename Stream>
        void Unserialize(Stream& s) {
            s >> whitelisted;
            s >> totalBlocks;
            s >> windowBlocks;
            s >> blockVector;
        }
    };
}


CMinerWhitelistDB::CMinerWhitelistDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "minerwhitelist", nCacheSize, fMemory, fWipe) 
{
}


bool CMinerWhitelistDB::Init(bool fWipe){
    bool init;
    if(fWipe || Read(DB_INIT,init)==false || init==false ) {
        LogPrintf("MinerDatabase: Create new Database.\n");
        CDBBatch batch(*this);
        batch.Write(WL_FACTOR, 0);
        batch.Write(WL_WINDOW_LENGTH, 2016);
        batch.Write(WL_CAP_ENABLED, 0);
        batch.Write(DB_HEIGHT, 0);
        unsigned int numberMiners = 0;
        for (std::set<std::string>::iterator it = Params().GetConsensus().minerWhiteListAdminAddress.begin(); it != Params().GetConsensus().minerWhiteListAdminAddress.end(); it++){
            batch.Write(MinerEntry(*it), MinerDetails(true,true));
            numberMiners += 1;
        }
        
        // At the beginning there was another admin key hardcoded into the client. 
        // also add that address, but allow for later removal.
        std::string third = "pGNcLNCavQLGXwXkVDwoHPCuQUBoXzJtPh";
        batch.Write(MinerEntry(third), MinerDetails(true,false)); 
        numberMiners += 1;
        batch.Write(WL_NUMBER_MINERS, numberMiners);
        LogPrintf("MinerDatabase: Added admin keys.\n");

        // add dummy address which will be first entry for searching the database
        batch.Write(MinerEntry(DUMMY), MinerDetails(false,false)); 


        for (int i=1; i <= Params().GetConsensus().minerWhiteListActivationHeight; i++) {
            batch.Write(BlockEntry(i), BlockDetails(DUMMY, false, 0));
        }
        batch.Write(DB_HEIGHT, Params().GetConsensus().minerWhiteListActivationHeight);
        batch.Write(DB_INIT,true);
        WriteBatch(batch);
    }
    return true;
}

bool CMinerWhitelistDB::EnableCap(unsigned int factor) {
    LogPrintf("MinerDatabase: Enabling Cap with factor %d.\n", factor);
    CDBBatch batch(*this);
    batch.Write(WL_FACTOR, factor);
    batch.Write(WL_CAP_ENABLED, true);
    return WriteBatch(batch);
}

bool CMinerWhitelistDB::ReEnableCap() {
    unsigned int height;
    LogPrintf("MinerDatabase: Re-Enabling Cap with previous factor.\n");
    if (!Read(DB_HEIGHT, height))
        return false;
    BlockDetails det = BlockDetails();
    if (!Read(BlockEntry(height), det))
        return false;
    LogPrintf("MinerDatabase: Previous factor was %d\n", det.prevFactor);
    CDBBatch batch(*this);
    batch.Write(WL_CAP_ENABLED, true);
    batch.Write(WL_FACTOR, det.prevFactor);
    return WriteBatch(batch);
}

bool CMinerWhitelistDB::DisableCap() {
    LogPrintf("MinerDatabase: Disabling Cap.\n");
    CDBBatch batch(*this);
    batch.Write(WL_CAP_ENABLED,false);
    batch.Write(WL_FACTOR, 0);// TODO: think about this a bit more!
    return WriteBatch(batch);
}

bool CMinerWhitelistDB::IsCapEnabled() {
    char fEnabled;
    if (!Read(WL_CAP_ENABLED, fEnabled))
        return false;
    return fEnabled == true;
}

bool CMinerWhitelistDB::IsWhitelistEnabled() {
    return chainActive.Height() >= Params().GetConsensus().minerWhiteListActivationHeight;
}


unsigned int CMinerWhitelistDB::GetCapFactor() {
    unsigned int factor;
    Read(WL_FACTOR, factor);

    return factor;
}

unsigned int CMinerWhitelistDB::GetNumberOfWhitelistedMiners() {
    unsigned int number;
    Read(WL_NUMBER_MINERS, number);

    // On block 34342, the third admin key was finaly blacklisted. 
    // Up to that point substract one from the number of miners to accomodate the 
    // three admins.
    if (chainActive.Height() < 34342) {
        number -= 1;
    }

    return number;
}


unsigned int CMinerWhitelistDB::GetAvgBlocksPerMiner() {
    unsigned int number = GetNumberOfWhitelistedMiners();
    return 2016/number;
}


unsigned int CMinerWhitelistDB::GetCap() {
    
    unsigned int factor = GetCapFactor();
    unsigned int number = GetNumberOfWhitelistedMiners();

    if (chainActive.Height() < Params().GetConsensus().minerCapSystemChangeHeight)
        return factor*(2016/number);

    return (factor*2016)/number;
    
    return 2016;
}


bool CMinerWhitelistDB::WhitelistMiner(std::string address) {
    
    MinerEntry mEntry(address);
    MinerDetails mDetails = MinerDetails();
    CDBBatch batch(*this);
    unsigned int number;
    Read(WL_NUMBER_MINERS, number);
    if (Read(mEntry, mDetails)) {
        LogPrintf("MinerDatabase: Miner already in Database. %s\n", address);
        if (!mDetails.whitelisted) {
            LogPrintf("MinerDatabase: Adding Miner back to whitelist. %s\n", address);
            mDetails.whitelisted = true;
            batch.Write(mEntry, mDetails);
            batch.Write(WL_NUMBER_MINERS, number+1);
        }
        LogPrintf("MinerDatabase: Miner already whitelisted. %s\n", address);
    } 
    else {
        LogPrintf("MinerDatabase: Whitelisting previously unknown Miner %s\n", address);
        batch.Write(mEntry,MinerDetails(true,false));
        batch.Write(WL_NUMBER_MINERS, number+1);
    }
    
    return WriteBatch(batch);
}

bool CMinerWhitelistDB::BlacklistMiner(std::string address) {
    
    MinerEntry mEntry(address);
    MinerDetails mDetails = MinerDetails();
    CDBBatch batch(*this);

    unsigned int number;
    Read(WL_NUMBER_MINERS, number);

    if (Read(mEntry, mDetails)) {
        // if (mDetails.isAdmin) { // don't remove admins, but no error.
        // LogPrintf("MinerDatabase: NOT Blacklisting admin %s\n", address);
        //     return true;
        // }
        if (mDetails.whitelisted) {
            LogPrintf("MinerDatabase: Blacklisting Miner %s\n", address);
            mDetails.whitelisted = false;
            batch.Write(mEntry, mDetails);
            batch.Write(WL_NUMBER_MINERS, number-1);
        }
    }
    else {
        LogPrintf("MinerDatabase: Blacklisting previously unknown Miner %s\n", address);
        batch.Write(mEntry, MinerDetails(false,false));
    }

    return WriteBatch(batch);
}

bool CMinerWhitelistDB::ExistMiner(std::string address) {
    return Exists(MinerEntry(address));
}

bool CMinerWhitelistDB::isWhitelisted(std::string address) {
    MinerEntry mEntry(address);
    MinerDetails mDetails = MinerDetails();
    if (!Exists(mEntry)) {
        LogPrintf("MinerDatabase: Miner does not exist in database.\n");
        return false;
    }
    LogPrintf("MinerDatabase: Miner is in database.\n");
    Read(mEntry, mDetails);
    return mDetails.whitelisted;
}

unsigned int CMinerWhitelistDB::GetTotalBlocks(std::string address) {
    MinerDetails det = MinerDetails();
    if (Read(MinerEntry(address), det))
        return det.totalBlocks;
    return 0;
}

unsigned int CMinerWhitelistDB::GetBlocksInWindow(std::string address) {
    MinerDetails det = MinerDetails();
    if (Read(MinerEntry(address), det))
        return det.windowBlocks;
    return 0;
}

unsigned int CMinerWhitelistDB::GetWindowStart(unsigned int height) {
  if (height < 2017)
		return 1;
  if (height < Params().GetConsensus().minerCapSystemChangeHeight) {
  	return height-height%2016;
  }
	return height - 2016 + 1;
}

// bool CMinerWhitelistDB::Sync() {
//   return Sync();
// }

bool CMinerWhitelistDB::DumpWindowStats(std::vector< std::pair< std::string, uint32_t > > *MinerVector) {
    LogPrintf("MinerDatabase: Dumping all miner stats.\n");
    
    std::unique_ptr<CDBIterator> it(NewIterator());
    for (it->Seek(MinerEntry(DUMMY)); it->Valid(); it->Next()) { // Seek should end up at an address (they start with 'a')
        MinerEntry entry = MinerEntry();
        MinerDetails det = MinerDetails();
        if (it->GetKey(entry)) { // Does this work? Should give false if Key is of other type than what we expect?!
            if (entry.addr == DUMMY)
                continue;
            if (entry.type != WL_ADDRESS)
                break;
            it->GetValue(det);
            if (det.whitelisted == false || det.windowBlocks == 0) 
                continue;
            std::pair<std::string, uint32_t> elem;
            elem.first = entry.addr;
            elem.second = det.windowBlocks;
            (*MinerVector).push_back(elem);
        }
        else { // no miner anymore
            break;
        }
    } 
    return true;
}

bool CMinerWhitelistDB::DumpStatsForMiner(std::string address, bool *wlisted, unsigned int *windowBlocks, unsigned int *totalBlocks, unsigned int *lastBlock) {
    LogPrintf("MinerDatabase: Dumping stats for miner %s.\n", address);
    
    MinerEntry entry = MinerEntry(address);
    if(!Exists(entry))
        return false;
    
    MinerDetails details = MinerDetails();
    Read(entry, details);
    *wlisted = details.whitelisted;
    *windowBlocks = details.windowBlocks;
    *totalBlocks = details.totalBlocks;
    *lastBlock = details.blockVector.back();

    return true;
}


bool CMinerWhitelistDB::MineBlock(unsigned int newHeight, std::string address) {
    LogPrintf("MinerDatabase: Adding Block %d to the database. Mined by %s\n", newHeight, address);
    // First check that we are actually at the tip
    unsigned int currHeight;
    Read(DB_HEIGHT, currHeight);
    LogPrintf("MinerDatabase: Database is currently at block %d\n", currHeight);

    assert(newHeight==currHeight+1);

    CDBBatch batch(*this);
    

    /* SPECIAL STUFF for block no 617. The first implementation of the whitelist took
    whitelist transactions into account. Block 617 contained the transaction that whitelisted
    miner of block 617. Add this miner on block 616 instead. */
    if (newHeight==616) 
        WhitelistMiner("pQAqAHfaJaCDQKedbgSXvDkJHhSsTmKGV9");
    if (newHeight==9912)
        WhitelistMiner("pDqhZSLQGfz2JGq1MNdYpp5M2QLjdujeAF");



    // First make entry for the new Block
    LogPrintf("MinerDatabase: Making entry for new block.\n");
    batch.Write(BlockEntry(newHeight), BlockDetails(address, IsCapEnabled(), GetCapFactor()));
    batch.Write(DB_HEIGHT, newHeight);
   
    // Now make sure that the minerstats match up. 
    // Window Handling
    if ( newHeight < Params().GetConsensus().minerCapSystemChangeHeight ) {
        LogPrintf("MinerDatabase: We are using the old cap system.\n");
        // old system
        // Reset the window every 2016 blocks
        if ( newHeight%2016 == 0) {
            LogPrintf("MinerDatabase: New Window beginning\n");
            LogPrintf("MinerDatabase: Creating Iterator\n");
            std::unique_ptr<CDBIterator> it(NewIterator());
            LogPrintf("MinerDatabase: Starting Loop.\n");
            for (it->Seek(MinerEntry(DUMMY)); it->Valid(); it->Next()) { // SeekToFirst should end up at an address (they start with 'a')
                MinerEntry entry = MinerEntry();
                MinerDetails det = MinerDetails();
                LogPrintf("MinerDatabase: Reading Entry.\n");
                if (it->GetKey(entry)) { // Does this work? Should give false if Key is of other type than what we expect?!
                    it->GetValue(det);
                    if (entry.addr == address) { 
                        det.totalBlocks += 1;
                    }
                    det.windowBlocks = 0;
                    batch.Write(entry, det);
                }
                else { // no miner anymore
                    break;
                }
            } 
        } 
        else {
            LogPrintf("MinerDatabase: In window\n");
            MinerDetails minDets = MinerDetails();
            if (!Exists(MinerEntry(address)))
                return false;
            Read(MinerEntry(address), minDets);
            // Increase counter for the one mining the block
            minDets.totalBlocks += 1;
            minDets.windowBlocks += 1;
            minDets.blockVector.push_back(newHeight);
            batch.Write(MinerEntry(address), minDets);
        }
    }
    else {
        // new System
        LogPrintf("MinerDatabase: We are using the new cap system.\n");
        MinerDetails minDets = MinerDetails();
        if (!Exists(MinerEntry(address)))
            return false;
        Read(MinerEntry(address), minDets);
        // Increase counter for the one mining the block
        minDets.totalBlocks += 1;
        minDets.windowBlocks += 1;
        minDets.blockVector.push_back(newHeight);

      
        // Check if there is a block dropping out of the window
        unsigned int blockDroppingOut = GetWindowStart(newHeight)-1;
        
        if (blockDroppingOut) { // There is!
            BlockDetails dropDets = BlockDetails();
            if (!Exists(BlockEntry(blockDroppingOut)))
                return false;
            Read(BlockEntry(blockDroppingOut), dropDets);
            if (dropDets.miner == address) { 
                // the miner who mined the current block also mined the one 
                // dropping out of the window so decrease his count again.
                minDets.windowBlocks -= 1;
                batch.Write(MinerEntry(address), minDets);  
            } else { // different Miner
                MinerDetails otherDets = MinerDetails();
                if (!Exists(MinerEntry(dropDets.miner)))
                    return false;
                Read(MinerEntry(dropDets.miner), otherDets);
                otherDets.windowBlocks -= 1;
                batch.Write(MinerEntry(address), minDets);  
                batch.Write(MinerEntry(dropDets.miner), otherDets);  
            }
        } else {
            batch.Write(MinerEntry(address), minDets);
        }
    }
    WriteBatch(batch);
    return true;
}
      
bool CMinerWhitelistDB::RewindBlock(unsigned int index) {
    LogPrintf("MinerDatabase: Removing Block %d from the database.\n", index);

    CDBBatch batch(*this);

    // First check that we are actually at the tip
    unsigned int currHeight;
    Read(DB_HEIGHT, currHeight);
    LogPrintf("MinerDatabase: Database was at block %d previously.\n", currHeight);
    assert(currHeight==index);

    // See who mined this block
    BlockDetails currDets = BlockDetails();
    if (!Exists(BlockEntry(currHeight)))
        return false;
    Read(BlockEntry(currHeight), currDets);
    std::string miner = currDets.miner;

    // delete entry for the block
    batch.Erase(BlockEntry(index));
    batch.Write(DB_HEIGHT, index-1);
   

    // Window Handling
    if ( index < Params().GetConsensus().minerCapSystemChangeHeight ) {
        // old system
        // code will never be used as the old system is buried unter checkpoints

        // This is what happens at the window border
        if (index%2016==0) {
            // TODO iterate over the last 2016 blocks to restore the correct value for the cap.
            std::unique_ptr<CDBIterator> it(NewIterator());
            std::unordered_map<std::string, unsigned int> coinMap; 
            int bcount = 0;
            for (it->Seek(BlockEntry(index-2016+1)); it->Valid(); it->Next()) { // Start at beginning of Window
                BlockEntry entry = BlockEntry();
                bcount+=1; // count blocks
                if (bcount > 2016)
                    break; // only work until current block
                if (it->GetKey(entry)) { // Does this work? Should give false if Key is of other type than what we expect?!
                    BlockDetails det = BlockDetails();
                    it->GetValue(det);
                    MinerDetails bMDets = MinerDetails();
                    
                    if (!Exists(MinerEntry(det.miner)))
                        return false;
                    Read(MinerEntry(det.miner), bMDets);
                    if (coinMap.count(det.miner)) {
                        coinMap[det.miner] += 1; // miner already has coins
                    } else {
                        coinMap[det.miner] = 1; // first appearance of miner this window
                    }
                } else { // Not a block anymore (LevelDB is sorted)
                    break;
                }
            }
            // we now have a map of the coincount per miner. rewrite it into db
            for (auto& x: coinMap) {
                MinerDetails mdetails = MinerDetails(); 
                Read(MinerEntry(x.first), mdetails);
                
                if (x.first == miner) {
                    mdetails.windowBlocks = x.second - 1; // The current block will be removed from database. Do not count it
                    mdetails.totalBlocks -= 1; // also remove current block from the miners total count.
                } else {
                    mdetails.windowBlocks = x.second;
                }

                batch.Write(MinerEntry(x.first), mdetails);
            } 
        } else { // normal stuff during the window
            MinerDetails minDets = MinerDetails();
            if (!Exists(MinerEntry(miner)))
                return false;
            Read(MinerEntry(miner), minDets);
            // Decrease counter for the one mining the block
            minDets.totalBlocks -= 1;
            minDets.windowBlocks -= 1;
            minDets.blockVector.pop_back();
            batch.Write(MinerEntry(miner), minDets);
        }
    }
    else {
        // new System
        MinerDetails minDets = MinerDetails();
        if (!Exists(MinerEntry(miner)))
            return false;
        Read(MinerEntry(miner), minDets);
        // Decrease counter for the one mining the block
        minDets.totalBlocks -= 1;
        minDets.windowBlocks -= 1;
        minDets.blockVector.pop_back();

        // Check if there is a block moving back into the window
        unsigned int blockMovingIn = GetWindowStart(index)-1;
        
        if (blockMovingIn) { // There is!
            BlockDetails dropDets = BlockDetails();
            if (!Exists(BlockEntry(blockMovingIn)))
                return false;
            Read(BlockEntry(blockMovingIn), dropDets);
            if (dropDets.miner == miner) { 
                // the miner who mined the current block also mined the one 
                // moving into the window so re-increase his count.
                minDets.windowBlocks += 1;
                batch.Write(MinerEntry(miner), minDets);  
            } else { // different Miner
                MinerDetails dets = MinerDetails();
                if (!Exists(MinerEntry(dropDets.miner)))
                    return false;
                Read(MinerEntry(dropDets.miner), dets);
                dets.windowBlocks += 1;
                batch.Write(MinerEntry(miner), minDets);  
                batch.Write(MinerEntry(dropDets.miner), dets);  
            }
        } else {
            batch.Write(MinerEntry(miner), minDets);
        }
    }
    WriteBatch(batch);
    return true;
}

bool CMinerWhitelistDB::hasExceededCap(std::string address) {
    if (Params().GetConsensus().minerWhiteListAdminAddress.count(address) || (chainActive.Height() < 38304 && address == "pGNcLNCavQLGXwXkVDwoHPCuQUBoXzJtPh"))
        return false;
    
    
    unsigned int cap = CMinerWhitelistDB::GetCap();
    unsigned int blocks = CMinerWhitelistDB::GetBlocksInWindow(address);
    
    LogPrintf("MinerCap: Has %s exceeded miner cap? %s > %s ?\n", address, blocks, cap);

    // There has been a bug in the previous implementation that did not take the current block 
    // into account when counting the number of blocks mined. Fix these manually.
    if (blocks == cap + 1 && chainActive.Height() < Params().GetConsensus().minerCapSystemChangeHeight && address == getMinerforBlock(chainActive.Height()) ) {
        LogPrintf("MinerCap: Exception because of two consecutive blocks from the same miner.\n");
        return false;
    }

    return blocks > cap;
}

// bool CMinerWhitelistDB::WriteReindexing(bool fReindexing) {
//     if (fReindexing)
//         return Write(DB_REINDEX_FLAG, '1');
//     else
//         return Erase(DB_REINDEX_FLAG);
// }

// bool CMinerWhitelistDB::ReadReindexing(bool &fReindexing) {
//     fReindexing = Exists(DB_REINDEX_FLAG);
//     return true;
// }


std::string CMinerWhitelistDB::getMinerforBlock(unsigned int index) {
    BlockDetails bdets = BlockDetails();
    Read(BlockEntry(index), bdets);
    return bdets.miner;
}
