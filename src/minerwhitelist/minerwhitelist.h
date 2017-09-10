// Copyright (c) 2017 IoP Ventures LLC
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IOP_MINERWHITELIST_H
#define IOP_MINERWHITELIST_H

class CMinerWhiteListDB {
protected:
  CDBWrapper db;
public:
  CMinerWhiteListDB(size_t nCacheSize, bool fMemory, bool fWipe);
  
  enum WhiteListAction {ADD_MINER, REMOVE_MINER, ENABLE_CAP, DISABLE_CAP, NONE};
  
  // Cap management
  bool EnableCap(unsigned int factor);
  bool EnableCap();
  
  bool DisableCap();
  bool IsCapEnabled();
  
  // Statistics
  unsigned int GetCapFactor();
  unsigned int GetNumberOfMiners();
  unsigned int GetAvgBlocksPerMiner();
  unsigned int GetCap();
  
  // Managing Whitelist
  bool AddNewMiner(std::string address);
  bool RemoveMiner(std::string address);
  bool ExistMiner(std::string address);
  
  
  // Managing Cap
  bool MineBlock(std::string address);
  unsigned int GetMinedCoins(std::string address);
  
  unsigned int GetWindowStart(currHeight);
  
  
  bool Sync();
  
};

#endif /* MINERWHITELIST_H_ */
