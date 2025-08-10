#ifndef BITCOIN_POS_VALIDATOR_H
#define BITCOIN_POS_VALIDATOR_H

#include <cstdint>

namespace pos {

// Simplified validator representation for Proof-of-Stake systems.
class Validator {
public:
    explicit Validator(uint64_t stake_amount);

    // Attempt to activate the validator if stake and time requirements are met.
    void Activate(int64_t current_time);

    // Schedule an unstake request; validator becomes inactive until time passes.
    void ScheduleUnstake(int64_t current_time);

    bool IsActive() const { return m_active; }

private:
    uint64_t m_stake_amount;
    int64_t m_locked_until;
    bool m_active;
};

constexpr uint64_t MIN_STAKE = 1000;
constexpr int64_t UNSTAKE_DELAY = 60 * 60 * 24 * 7; // one week

} // namespace pos

#endif // BITCOIN_POS_VALIDATOR_H
