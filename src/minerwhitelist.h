// Copyright (c) 2017 IoP Ventures LLC
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IOP_MINERWHITELIST_H
#define IOP_MINERWHITELIST_H

#include "dbwrapper.h"

class CMinerWhitelistDB {
protected:
  CDBWrapper db;
public:
  CMinerWhitelistDB(size_t nCacheSize, bool fMemory, bool fWipe);
  
  enum WhitelistAction {ADD_MINER, REMOVE_MINER, ENABLE_CAP, DISABLE_CAP, NONE};
  
  // Cap management
  bool EnableCap(unsigned int factor);
  bool ReEnableCap();
  
  bool DisableCap();
  bool IsCapEnabled();
  
  // Statistics
  unsigned int GetCapFactor();
  unsigned int GetNumberOfWhitelistedMiners();
  unsigned int GetAvgBlocksPerMiner();
  unsigned int GetCap();
  
  // Managing Whitelist
  bool WhitelistMiner(std::string address);
  bool BlacklistMiner(std::string address);
  bool ExistMiner(std::string address);
  
  
  unsigned int GetTotalBlocks(std::string address);
  unsigned int GetBlocksInWindow(std::string address);
  unsigned int GetWindowStart(unsigned int height);
  bool hasExceededCap(std::string address);

  // Managing Cap

  bool MineBlock(unsigned int index, std::string address);
  bool RewindBlock(unsigned int index);
  
  
  
  bool Sync();
  
};

#endif /* MINERWHITELIST_H_ */
