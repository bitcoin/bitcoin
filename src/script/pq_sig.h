#ifndef BITCOIN_SCRIPT_PQ_SIG_H
#define BITCOIN_SCRIPT_PQ_SIG_H

#include <vector>
#include <cstdint>

// SHRINCS-style PQ signature (580 bytes)
#define PQ_SIG_SIZE 580
#define PQ_PUBKEY_SIZE 32

bool VerifyPQSig(const std::vector<unsigned char>& sig,
                 const std::vector<unsigned char>& msg,
                 const std::vector<unsigned char>& pubkey);

#endif
