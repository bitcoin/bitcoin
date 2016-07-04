#ifndef OMNICORE_STO_H
#define OMNICORE_STO_H

#include <stdint.h>
#include <set>
#include <string>
#include <utility>

namespace mastercore
{
//! Comparator for owner/receiver entries
struct SendToOwners_compare
{
    bool operator()(const std::pair<int64_t, std::string>& p1, const std::pair<int64_t, std::string>& p2) const;
};

//! Fee required to be paid per owner/receiver, nominated in willets
const int64_t TRANSFER_FEE_PER_OWNER = 1;
const int64_t TRANSFER_FEE_PER_OWNER_V1 = 1000;

//! Set of owner/receivers, sorted by amount they own or might receive
typedef std::set<std::pair<int64_t, std::string>, SendToOwners_compare> OwnerAddrType;

/** Determines the receivers and amounts to distribute. */
OwnerAddrType STO_GetReceivers(const std::string& sender, uint32_t property, int64_t amount);
}


#endif // OMNICORE_STO_H
