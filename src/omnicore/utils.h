#ifndef OMNICORE_UTILS_H
#define OMNICORE_UTILS_H

#include <string>

#include "uint256.h"

#define MAX_SHA256_OBFUSCATION_TIMES  255

/** Generates hashes used for obfuscation via ToUpper(HexStr(SHA256(x))). */
void PrepareObfuscatedHashes(const std::string& strSeed, int hashCount, std::string(&vstrHashes)[1+MAX_SHA256_OBFUSCATION_TIMES]);

/** Determines the Bitcoin address associated with a given hash and version. */
std::string HashToAddress(unsigned char version, const uint160& hash);

/** Returns a vector of bytes containing the version and hash160 for an address.*/
std::vector<unsigned char> AddressToBytes(const std::string& address);

#endif // OMNICORE_UTILS_H
