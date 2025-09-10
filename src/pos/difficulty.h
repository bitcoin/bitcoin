#ifndef BITCOIN_POS_DIFFICULTY_H
#define BITCOIN_POS_DIFFICULTY_H

#include <arith_uint256.h>
#include <consensus/params.h>

class CBlockIndex;
class CBlockHeader;

/** Compute the next PoS target required based on previous block spacing. */
unsigned int GetPoSNextTargetRequired(const CBlockIndex* pindexLast,
                                      const CBlockHeader* pblock,
                                      const Consensus::Params& params);

#endif // BITCOIN_POS_DIFFICULTY_H
