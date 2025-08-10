#ifndef BITCOIN_POS_SLASHING_H
#define BITCOIN_POS_SLASHING_H

#include <set>
#include <string>

namespace pos {

// Tracks validator evidence to detect double-signing events.
class SlashingTracker {
public:
    // Returns true if the validator was seen before (double-sign detected).
    bool DetectDoubleSign(const std::string& validator_id);

private:
    std::set<std::string> m_seen;
};

} // namespace pos

#endif // BITCOIN_POS_SLASHING_H
