/**
 * @file consensushash.cpp
 *
 * This file contains the function to generate consensus hashes.
 */

#include "omnicore/consensushash.h"
#include "omnicore/log.h"
#include "omnicore/omnicore.h"

#include <stdint.h>
#include <string>

#include <openssl/sha.h>

namespace mastercore
{
/**
 * Obtains a hash of all balances to use for consensus verification and checkpointing.
 *
 * For increased flexibility, so other implementations like OmniWallet and OmniChest can
 * also apply this methodology without necessarily using the same exact data types (which
 * would be needed to hash the data bytes directly), create a string in the following
 * format for each address/property identifier combo to use for hashing:
 *
 * Format specifiers:
 *   "%s|%d|%d|%d|%d|%d"
 *
 * With placeholders:
 *   "address|propertyid|balance|selloffer_reserve|accept_reserve|metadex_reserve"
 *
 * Example:
 *   SHA256 round 1: "1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R|31|8000|0|0|0"
 *   SHA256 round 2: "1LCShN3ntEbeRrj8XBFWdScGqw5NgDXL5R|2147483657|0|0|0|234235245"
 *   SHA256 round 3: "1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb|1|0|100|0|0"
 *   SHA256 round 4: "1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb|2|0|0|0|50000"
 *   SHA256 round 5: "1LP7G6uiHPaG8jm64aconFF8CLBAnRGYcb|10|0|0|1|0"
 *   SHA256 round 6: "3CwZ7FiQ4MqBenRdCkjjc41M5bnoKQGC2b|1|12345|5|777|9000"
 *
 * Result:
 *   "3580e94167f75620dfa8c267862aa47af46164ed8edaec3a800d732050ec0607"
 *
 * The byte order is important, and in the example we assume:
 *   SHA256("abc") = "ad1500f261ff10b49c7a1796a36103b02322ae5dde404141eacf018fbf1678ba"
 *
 * Note: empty balance records and the pending tally are ignored. Addresses are sorted based
 * on lexicographical order, and balance records are sorted by the property identifiers.
 */
uint256 GetConsensusHash()
{
    // allocate and init a SHA256_CTX
    SHA256_CTX shaCtx;
    SHA256_Init(&shaCtx);

    LOCK(cs_tally);

    if (msc_debug_consensus_hash) PrintToLog("Beginning generation of current consensus hash...\n");

    // loop through the tally map, updating the sha context with the data from each balance and tally type
    for (std::map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
        const std::string& address = my_it->first;
        CMPTally& tally = my_it->second;
        tally.init();
        uint32_t propertyId = 0;
        while (0 != (propertyId = (tally.next()))) {
            int64_t balance = tally.getMoney(propertyId, BALANCE);
            int64_t sellOfferReserve = tally.getMoney(propertyId, SELLOFFER_RESERVE);
            int64_t acceptReserve = tally.getMoney(propertyId, ACCEPT_RESERVE);
            int64_t metaDExReserve = tally.getMoney(propertyId, METADEX_RESERVE);

            // skip this entry if all balances are empty
            if (!balance && !sellOfferReserve && !acceptReserve && !metaDExReserve) continue;

            std::string dataStr = strprintf("%s|%d|%d|%d|%d|%d",
                    address, propertyId, balance, sellOfferReserve, acceptReserve, metaDExReserve);

            if (msc_debug_consensus_hash) PrintToLog("Adding data to consensus hash: %s\n", dataStr);

            // update the sha context with the data string
            SHA256_Update(&shaCtx, dataStr.c_str(), dataStr.length());
        }
    }

    // extract the final result and return the hash
    uint256 consensusHash;
    SHA256_Final((unsigned char*)&consensusHash, &shaCtx);
    if (msc_debug_consensus_hash) PrintToLog("Finished generation of consensus hash.  Result: %s\n", consensusHash.GetHex());

    return consensusHash;
}

} // namespace mastercore
