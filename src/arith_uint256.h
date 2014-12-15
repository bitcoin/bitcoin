#ifndef BITCOIN_ARITH_UINT256_H
#define BITCOIN_ARITH_UINT256_H

// Temporary for migration to opaque uint160/256
#include "uint256.h"

class arith_uint256 : public uint256 {
public:
    arith_uint256() {}
    arith_uint256(const base_uint<256>& b) : uint256(b) {}
    arith_uint256(uint64_t b) : uint256(b) {}
    explicit arith_uint256(const std::string& str) : uint256(str) {}
    explicit arith_uint256(const std::vector<unsigned char>& vch) : uint256(vch) {}
};

#define ArithToUint256(x) (x)
#define UintToArith256(x) (x)

#endif // BITCOIN_UINT256_H
