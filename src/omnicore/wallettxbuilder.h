#ifndef OMNICORE_WALLETTXBUILDER_H
#define OMNICORE_WALLETTXBUILDER_H

class uint256;

#include <stdint.h>
#include <string>
#include <vector>

/**
 * Creates and sends a transaction.
 */
int WalletTxBuilder(
        const std::string& senderAddress,
        const std::string& receiverAddress,
        const std::string& redemptionAddress,
        int64_t referenceAmount,
        const std::vector<unsigned char>& payload,
        uint256& retTxid,
        std::string& retRawTx,
        bool commit);

/**
 * Creates and sends a raw transaction by selecting all coins from the sender
 * and enough coins from a fee source. Change is sent to the fee source!
 */
int CreateFundedTransaction(
        const std::string& senderAddress,
        const std::string& receiverAddress,
        const std::string& feeAddress,
        const std::vector<unsigned char>& payload,
        uint256& retTxid);


#endif // OMNICORE_WALLETTXBUILDER_H
