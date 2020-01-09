#ifndef BITCOIN_SRC_VBK_TEST_UTIL_TX_HPP
#define BITCOIN_SRC_VBK_TEST_UTIL_TX_HPP

#include <primitives/transaction.h>

#include <vbk/util.hpp>

namespace VeriBlockTest {

// creates valid pop transaction given atv & vtbs
inline CMutableTransaction makePopTx(const std::vector<uint8_t>& atv, const std::vector<std::vector<uint8_t>>& vtbs)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    VeriBlock::setVBKNoInput(tx.vin[0].prevout);
    tx.vin[0].scriptSig << atv << OP_CHECKATV;
    for (const auto& vtb : vtbs) {
        tx.vin[0].scriptSig << vtb << OP_CHECKVTB;
    }
    tx.vin[0].scriptSig << OP_CHECKPOP;

    // set output: OP_RETURN
    tx.vout.resize(1);
    tx.vout[0].scriptPubKey << OP_RETURN;
    tx.vout[0].nValue = 0;

    return tx;
}

} // namespace VeriBlockTest

#endif //BITCOIN_SRC_VBK_TEST_UTIL_TX_HPP
