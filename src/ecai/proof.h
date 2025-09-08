#ifndef BITCOIN_ECAI_PROOF_H
#define BITCOIN_ECAI_PROOF_H
#include <array>
#include <vector>
#include <uint256.h>
#include <string>

namespace ecai {

struct Ctx {
    int32_t  height{0};
    int64_t  mediantime{0};
};

struct Proof {
    uint256 txid;
    uint256 policy_id;
    unsigned char verdict_tag;        // 1/2/3
    std::array<unsigned char,33> c_pol;
    std::array<unsigned char,33> p_tx;
    Ctx     ctx;
    std::array<unsigned char,64> sig; // Schnorr 64B; empty if no key configured
};

bool SignAndAssemble(const uint256& txid,
                     const uint256& policy_id,
                     unsigned char verdict_tag,
                     const std::array<unsigned char,33>& c_pol,
                     const std::array<unsigned char,33>& p_tx,
                     const Ctx& ctx,
                     Proof& out);

// ZMQ publisher (topic: "ecai.verdict")
void Publish(const Proof& p);

} // namespace ecai
#endif
