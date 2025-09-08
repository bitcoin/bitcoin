#include <ecai/state.h>
#include <ecai/h2c.h>
#include <ecai/util.h>
#include <hash.h>

namespace ecai {

State g_state;

void Enable(bool on) { g_state.enabled.store(on, std::memory_order_relaxed); }
bool Enabled() { return g_state.enabled.load(std::memory_order_relaxed); }

bool LoadPolicyHex(const std::string& hex, std::string& err)
{
    std::vector<unsigned char> bytes;
    if (!ParseHexStr(hex, bytes)) { err = "invalid hex"; return false; }
    if (bytes.empty()) { g_state.policy = Policy{}; return true; }

    g_state.policy.bytes = bytes;
    g_state.policy.id    = Hash(bytes.begin(), bytes.end());

    // Commitment point: H2C(policy_bytes) -> compressed
    if (!HashToCurve(bytes, g_state.policy.c_pol)) {
        err = "h2c failed";
        return false;
    }
    g_state.policy.loaded = true;
    return true;
}

} // namespace ecai
