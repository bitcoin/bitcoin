#ifndef OMNICORE_UTILS_H
#define OMNICORE_UTILS_H

#include "mastercore.h" // MAX_SHA256_OBFUSCATION_TIMES

class CScript;

#include <string>

/** Generates hashes used for obfuscation via ToUpper(HexStr(SHA256(x))). */
void PrepareObfuscatedHashes(const std::string& strSeed, std::string(&vstrHashes)[1+MAX_SHA256_OBFUSCATION_TIMES]);

/** Determines the minimum output amount to be spent by an output. */
int64_t GetDustLimit(const CScript& scriptPubKey);

#endif // OMNICORE_UTILS_H
