#ifndef BITCOIN_ECAI_POLICY_H
#define BITCOIN_ECAI_POLICY_H

#include <vector>
#include <string>

namespace ecai {

// Very small boolean evaluator for a canonical TLV policy.
// For brevity we support a few rule opcodes:
//
//  0x10 REQUIRE_FEERATE_GE  (u64)
//  0x11 REJECT_IF_RBF_AND_FEERATE_LT (u64)
//  0x12 CAP_DUST_LE (u16)
//  0x13 ALLOW_SCRIPT_MASK (u16 bitmask)  -- if nonzero, require (mask & script_mask)!=0
//
// Returns "accept", "reject", or "queue".
enum class Verdict { ACCEPT=1, REJECT=2, QUEUE=3 };

Verdict Evaluate(const std::vector<unsigned char>& policy_tlv,
                 const std::vector<unsigned char>& feat_tlv);

} // namespace ecai
#endif
