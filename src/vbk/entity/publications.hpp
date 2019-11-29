#ifndef INTEGRATION_REFERENCE_BTC_PUBLICATIONS_HPP
#define INTEGRATION_REFERENCE_BTC_PUBLICATIONS_HPP

#include <vector>

namespace VeriBlock {

using ByteArray = std::vector<uint8_t>;

struct Publications {
    ByteArray atv;
    std::vector<ByteArray> vtbs;
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_PUBLICATIONS_HPP
