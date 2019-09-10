#ifndef BITCOIN_OMNICORE_PARSING_H
#define BITCOIN_OMNICORE_PARSING_H

#include <string>
#include <vector>

class CTransaction;
class CMPTransaction;
class uint160;

// Encoding classes
#define NO_MARKER                       0
#define OMNI_CLASS_A                    1
#define OMNI_CLASS_B                    2
#define OMNI_CLASS_C                    3

// Encoding properties
#define PACKET_SIZE_CLASS_A            19
#define PACKET_SIZE                    31
#define MAX_PACKETS                   255
#define MAX_SHA256_OBFUSCATION_TIMES  255

/**
 * Swaps byte order on little-endian systems and does nothing 
 * otherwise. SwapByteOrder cycles on LE systems.
 */
void SwapByteOrder16(uint16_t&);
void SwapByteOrder32(uint32_t&);
void SwapByteOrder64(uint64_t&);

/** Determines the Bitcoin address associated with a given hash and version. */
std::string HashToAddress(unsigned char version, const uint160& hash);

/** Generates hashes used for obfuscation via ToUpper(HexStr(SHA256(x))). */
void PrepareObfuscatedHashes(const std::string& strSeed, int hashCount, std::string(&vstrHashes)[1+MAX_SHA256_OBFUSCATION_TIMES]);

/** Parses a transaction and populates the CMPTransaction object. */
int ParseTransaction(const CTransaction& tx, int nBlock, unsigned int idx, CMPTransaction& mptx, unsigned int nTime=0);


#endif // BITCOIN_OMNICORE_PARSING_H
