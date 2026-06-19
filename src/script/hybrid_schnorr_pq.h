#ifndef BITCOIN_SCRIPT_HYBRID_SCHNORR_PQ_H
#define BITCOIN_SCRIPT_HYBRID_SCHNORR_PQ_H

#include <stdint.h>
#include <vector>

/* Hybrid Schnorr + Falcon-1024 signature verification */
bool VerifyHybridSchnorrPQ(const std::vector<unsigned char>& sig,
                           const std::vector<unsigned char>& msg,
                           const std::vector<unsigned char>& pubkey);

#endif
