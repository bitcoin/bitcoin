#include "pos/slashing.h"

namespace pos {

bool SlashingTracker::DetectDoubleSign(const std::string& validator_id)
{
    if (m_seen.count(validator_id)) {
        return true;
    }
    m_seen.insert(validator_id);
    return false;
}

} // namespace pos
