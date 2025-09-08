#ifndef BITCOIN_ECAI_STATE_H
#define BITCOIN_ECAI_STATE_H

#include <atomic>
#include <vector>
#include <array>
#include <uint256.h>

namespace ecai {

struct Policy {
    std::vector<uint8_t> bytes;              // canonical TLV policy
    uint256              id;                 // SHA256(policy_bytes)
    std::array<unsigned char,33> c_pol;      // compressed secp256k1 point
    bool                 loaded{false};
};

struct State {
    std::atomic<bool> enabled{false};
    Policy policy;
};

extern State g_state;

// Toggle helpers
void Enable(bool on);
bool Enabled();

// Load/clear policy (hex TLV)
bool LoadPolicyHex(const std::string& hex, std::string& err);

} // namespace ecai
#endif
