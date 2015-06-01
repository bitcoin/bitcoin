#ifndef OMNICORE_UTILS_H
#define OMNICORE_UTILS_H

#include <string>

#define MAX_SHA256_OBFUSCATION_TIMES  255

/** Generates hashes used for obfuscation via ToUpper(HexStr(SHA256(x))). */
void PrepareObfuscatedHashes(const std::string& strSeed, std::string(&vstrHashes)[1+MAX_SHA256_OBFUSCATION_TIMES]);


#endif // OMNICORE_UTILS_H
