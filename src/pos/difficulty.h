#ifndef BITCOIN_POS_DIFFICULTY_H
#define BITCOIN_POS_DIFFICULTY_H

#include <arith_uint256.h>
#include <consensus/params.h>
#include <cstdint>

class CBlockIndex;

/** Compute the next PoS target required based on previous block spacing. */
unsigned int GetPoSNextTargetRequired(const CBlockIndex* pindexLast,
                                      int64_t nBlockTime,
                                      const Consensus::Params& params);

#endif // BITCOIN_POS_DIFFICULTY_H
