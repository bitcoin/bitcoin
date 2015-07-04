#ifndef OMNICORE_FETCHWALLETTX_H
#define OMNICORE_FETCHWALLETTX_H

class uint256;

//#include "util.h"
#include "txdb.h"

#include <stdint.h>
#include <map>
#include <string>

/** Gets the byte offset of a transaction from the tx index
  */
uint64_t GetTransactionByteOffset(const uint256& txid);

/**
  * Returns an ordered list of Omni transactions that are relevant to the wallet
  * Ignores order in the wallet (which can be skewed by watch addresses) and utilizes block height and position within block
  */
std::map<std::string,uint256> FetchWalletOmniTransactions(int count, int startBlock = 0, int endBlock = 999999);

#endif // OMNICORE_FETCHWALLETTX_H
