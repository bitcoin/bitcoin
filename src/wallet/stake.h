#ifndef BITCOIN_WALLET_STAKE_H
#define BITCOIN_WALLET_STAKE_H

#include <atomic>
#include <thread>

namespace wallet {

class CWallet;

/**
 * Stake runs a background thread to look for proof-of-stake opportunities
 * by scanning wallet UTXOs for eligible kernels and submitting blocks.
 * The implementation is intentionally simple and experimental.
 */
class Stake
{
public:
    explicit Stake(CWallet& wallet);
    ~Stake();

    /** Start the staking thread. */
    void Start();
    /** Stop the staking thread. */
    void Stop();
    /** Return true if the staking thread is active. */
    bool IsActive() const;

private:
    /** Main staking thread loop. Scans for kernels, creates and signs
     *  coinstake transactions and blocks. */
    void ThreadStakeMiner();

    CWallet& m_wallet;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

} // namespace wallet

#endif // BITCOIN_WALLET_STAKE_H
