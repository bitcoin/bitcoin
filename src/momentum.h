#include "uint256.h"
#include "semiOrderedMap.h"

#ifndef BITCOINTALKCOIN_MOMENTUM_H
#define BITCOINTALKCOIN_MOMENTUM_H

std::vector< std::pair<uint32_t,uint32_t> > momentum_search( uint256 midHash );
bool momentum_verify( uint256 midHash, uint32_t a, uint32_t b );
 
#endif
