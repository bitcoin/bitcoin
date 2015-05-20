#ifndef OMNICORE_PENDING_H
#define OMNICORE_PENDING_H

class uint256;

#include <stdint.h>
#include <string>

/** Adds a transaction to the pending map using supplied parameters. */
void PendingAdd(const uint256& txid, const std::string& sendingAddress, const std::string& refAddress, uint16_t type, uint32_t propertyId, int64_t amount, uint32_t propertyIdDesired = 0, int64_t amountDesired = 0, int64_t action = 0);

/** Deletes a transaction from the pending map and credits the amount back to the pending tally for the address. */
void PendingDelete(const uint256& txid);


#endif // OMNICORE_PENDING_H

