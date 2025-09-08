#ifndef BITCOIN_ECAI_H2C_H
#define BITCOIN_ECAI_H2C_H
#include <array>
#include <vector>

namespace ecai {

// Hash-to-curve producing a valid compressed secp256k1 point (33 bytes).
// Deterministic try-and-increment on tagged hash "ECAI_H2C".
bool HashToCurve(const std::vector<unsigned char>& msg,
                 std::array<unsigned char,33>& out_compressed);

} // namespace ecai
#endif
