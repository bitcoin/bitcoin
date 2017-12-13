#ifndef OMNICORE_WALLETTXBUILDER_H
#define OMNICORE_WALLETTXBUILDER_H

#include <stdint.h>
#include <string>
#include <vector>

class uint256;


int WalletTxBuilder(const std::string& senderAddress, const std::string& receiverAddress, const std::string& redemptionAddress,
                 int64_t referenceAmount, const std::vector<unsigned char>& data, uint256& txid, std::string& rawHex, bool commit);


#endif // OMNICORE_WALLETTXBUILDER_H
