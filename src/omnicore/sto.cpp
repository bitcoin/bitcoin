#include "omnicore/sto.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/tally.h"
#include "omnicore/uint256_extensions.h"

#include "arith_uint256.h"
#include "sync.h"

#include <assert.h>
#include <stdint.h>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace mastercore
{

/**
 * Compares two owner/receiver entries, based on amount.
 */
bool SendToOwners_compare::operator()(const std::pair<int64_t, std::string>& p1, const std::pair<int64_t, std::string>& p2) const
{
    if (p1.first == p2.first) return p1.second > p2.second; // reverse check
    else return p1.first < p2.first;
}

/**
 * Determines the receivers and amounts to distribute.
 *
 * The sender is excluded from the result set.
 */
OwnerAddrType STO_GetReceivers(const std::string& sender, uint32_t property, int64_t amount)
{
    int64_t totalTokens = 0;
    int64_t senderTokens = 0;
    OwnerAddrType ownerAddrSet;

    {
        LOCK(cs_tally);
        std::unordered_map<std::string, CMPTally>::iterator it;

        for (it = mp_tally_map.begin(); it != mp_tally_map.end(); ++it) {
            const std::string& address = it->first;
            const CMPTally& tally = it->second;

            int64_t tokens = 0;
            tokens += tally.getMoney(property, BALANCE);
            tokens += tally.getMoney(property, SELLOFFER_RESERVE);
            tokens += tally.getMoney(property, ACCEPT_RESERVE);
            tokens += tally.getMoney(property, METADEX_RESERVE);

            // Do not include the sender
            if (address == sender) {
                senderTokens = tokens;
                continue;
            }

            totalTokens += tokens;

            // Only holders with balance are relevant
            if (0 < tokens) {
                ownerAddrSet.insert(std::make_pair(tokens, address));
            }
        }
    }

    // Split up what was taken and distribute between all holders
    int64_t sent_so_far = 0;
    OwnerAddrType receiversSet;

    for (OwnerAddrType::reverse_iterator it = ownerAddrSet.rbegin(); it != ownerAddrSet.rend(); ++it) {
        const std::string& address = it->second;

        arith_uint256 owns = ConvertTo256(it->first);
        arith_uint256 temp = owns * ConvertTo256(amount);
        arith_uint256 piece = DivideAndRoundUp(temp, ConvertTo256(totalTokens));

        int64_t will_really_receive = 0;
        int64_t should_receive = ConvertTo64(piece);

        // Ensure that no more than available is distributed
        if ((amount - sent_so_far) < should_receive) {
            will_really_receive = amount - sent_so_far;
        } else {
            will_really_receive = should_receive;
        }

        sent_so_far += will_really_receive;

        if (msc_debug_sto) {
            PrintToLog("%14d = %s, temp= %38s, should_get= %19d, will_really_get= %14d, sent_so_far= %14d\n",
                it->first, address, temp.ToString(), should_receive, will_really_receive, sent_so_far);
        }

        // Stop, once the whole amount is allocated
        if (will_really_receive > 0) {
            receiversSet.insert(std::make_pair(will_really_receive, address));
        } else {
            break;
        }
    }

    uint64_t numberOfOwners = receiversSet.size();
    PrintToLog("\t    Total Tokens: %s\n", FormatMP(property, totalTokens + senderTokens));
    PrintToLog("\tExcluding Sender: %s\n", FormatMP(property, totalTokens));
    PrintToLog("\t          Owners: %d\n", numberOfOwners);

    return receiversSet;
}

} // namespace mastercore

