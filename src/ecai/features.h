#ifndef BITCOIN_ECAI_FEATURES_H
#define BITCOIN_ECAI_FEATURES_H

#include <array>
#include <vector>
#include <primitives/transaction.h>
#include <txmempool.h>

namespace ecai {

// Deterministic TLV (sorted by tag:u8). Big-endian numeric fields.
std::vector<unsigned char> EncodeTLVFeatures(const CTransaction& tx,
                                             const CTxMemPool& mempool);

// Convenience wrapper to compute compressed commitment P_tx (33B)
bool FeatureCommitment(const CTransaction& tx, const CTxMemPool& mempool,
                       std::array<unsigned char,33>& out);

} // namespace ecai
#endif
