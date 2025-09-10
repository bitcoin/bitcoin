#ifndef BITCOIN_WALLET_BITGOLDSTAKER_H
#define BITCOIN_WALLET_BITGOLDSTAKER_H

#include <atomic>
#include <thread>

namespace wallet {

class CWallet;

/** BitGoldStaker runs a background thread performing simple proof-of-stake
 *  block creation by selecting mature UTXOs and submitting new blocks. The
 *  implementation is minimal and intended for experimental use only. */
class BitGoldStaker
{
public:
    explicit BitGoldStaker(CWallet& wallet);
    ~BitGoldStaker();

    /** Start the staking thread. */
    void Start();
    /** Stop the staking thread. */
    void Stop();
    /** Return true if the staking thread is active. */
    bool IsActive() const;

private:
    /** Main staking thread loop. Gathers eligible UTXOs, checks stake kernels,
     *  builds coinstake transactions and blocks, and broadcasts them with
     *  basic back-off handling.
     */
    void ThreadStakeMiner();

    CWallet& m_wallet;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

} // namespace wallet

#endif // BITCOIN_WALLET_BITGOLDSTAKER_H
