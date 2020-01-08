#ifndef INTEGRATION_REFERENCE_BTC_POP_HPP
#define INTEGRATION_REFERENCE_BTC_POP_HPP

#include <cstdint>
#include <vector>

namespace VeriBlock {

using ByteArray = std::vector<uint8_t>;

struct Publications {
    ByteArray atv;
    std::vector<ByteArray> vtbs;
};

struct Context {
    std::vector<ByteArray> btc;
    std::vector<ByteArray> vbk;

    bool operator==(const Context& other) const
    {
        return btc == other.btc && vbk == other.vbk;
    }
};

enum class PopTxType {
    UNKNOWN = 0,
    PUBLICATIONS = 1,
    CONTEXT = 2
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_POP_HPP
