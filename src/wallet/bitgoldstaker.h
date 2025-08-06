#ifndef BITCOIN_WALLET_BITGOLDSTAKER_H
#define BITCOIN_WALLET_BITGOLDSTAKER_H

#include <atomic>
#include <thread>

namespace wallet {

class CWallet;

/** BitGoldStaker is a placeholder thread that would handle proof-of-stake
 *  block creation. The actual staking logic is not implemented here and should
 *  be provided by a full PoS implementation. */
class BitGoldStaker {
public:
    explicit BitGoldStaker(CWallet& wallet);
    ~BitGoldStaker();

    /** Start the staking thread. */
    void Start();
    /** Stop the staking thread. */
    void Stop();

private:
    /** Main thread loop. Selects UTXOs, calls CheckStakeKernelHash and crafts
     *  coinstake transactions. Currently only a stub.
     */
    void ThreadStaker();

    CWallet& m_wallet;
    std::thread m_thread;
    std::atomic<bool> m_stop{false};
};

/** Placeholder for stake kernel hash check. */
bool CheckStakeKernelHash();

} // namespace wallet

#endif // BITCOIN_WALLET_BITGOLDSTAKER_H
