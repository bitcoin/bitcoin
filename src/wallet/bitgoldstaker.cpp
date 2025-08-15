#include <chrono>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <interfaces/chain.h>
#include <node/context.h>
#include <pos/stake.h>
#include <util/time.h>
#include <validation.h>
#include <wallet/bitgoldstaker.h>
#include <wallet/wallet.h>
#include <wallet/spend.h>

#include <algorithm>
#include <logging.h>
#include <vector>

namespace wallet {

BitGoldStaker::BitGoldStaker(CWallet& wallet) : m_wallet(wallet) {}

BitGoldStaker::~BitGoldStaker()
{
    Stop();
}

void BitGoldStaker::Start()
{
    if (m_thread.joinable()) return;
    m_stop = false;
    m_thread = std::thread(&BitGoldStaker::ThreadStaker, this);
}

void BitGoldStaker::Stop()
{
    m_stop = true;
    if (m_thread.joinable()) m_thread.join();
}

bool BitGoldStaker::IsActive() const
{
    return m_thread.joinable() && !m_stop;
}

void BitGoldStaker::ThreadStaker()
{
    interfaces::Chain& chain = m_wallet.chain();
    node::NodeContext* node_context = chain.context();
    if (!node_context || !node_context->chainman) {
        LogDebug(BCLog::STAKING, "BitGoldStaker: chainman unavailable\n");
        return;
    }
    ChainstateManager& chainman = *node_context->chainman;
    const Consensus::Params& consensus = chainman.GetParams().GetConsensus();
    const CAmount MIN_STAKE_AMOUNT{1 * COIN};
    const int MIN_STAKE_DEPTH{COINBASE_MATURITY};
    const std::chrono::seconds MIN_COIN_AGE{std::chrono::hours(1)};

    std::chrono::milliseconds sleep_time{500};
    while (!m_stop) {
        bool staked{false};
        try {
            int chain_height;
            {
                LOCK(::cs_main);
                chain_height = chainman.ActiveChain().Height();
            }
            const int min_depth = chain_height < MIN_STAKE_DEPTH ? 0 : MIN_STAKE_DEPTH;
            const std::chrono::seconds min_age =
                chain_height < MIN_STAKE_DEPTH ? std::chrono::seconds{0} : MIN_COIN_AGE;

            std::vector<COutput> candidates;
            {
                LOCK(m_wallet.cs_wallet);
                for (const COutput& out : AvailableCoins(m_wallet).All()) {
                    if (!out.spendable) continue;
                    if (out.depth < min_depth) continue;
                    if (out.txout.nValue < MIN_STAKE_AMOUNT) continue;
                    if (min_age.count() > 0) {
                        int64_t age = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()) - out.time;
                        if (age < min_age.count()) continue;
                    }
                    candidates.push_back(out);
                }
            }

            if (candidates.empty()) {
                LogDebug(BCLog::STAKING, "BitGoldStaker: no eligible UTXOs\n");
            } else {
                LOCK(::cs_main);
                CBlockIndex* pindexPrev = chainman.ActiveChain().Tip();
                if (!pindexPrev) {
                    LogDebug(BCLog::STAKING, "BitGoldStaker: no tip block\n");
                } else {
                    for (const COutput& stake_out : candidates) {
                        uint256 confirmed_block_hash;
                        {
                            LOCK(m_wallet.cs_wallet);
                            const CWalletTx* wtx = m_wallet.GetWalletTx(stake_out.outpoint.hash);
                            if (!wtx) {
                                LogDebug(BCLog::STAKING, "BitGoldStaker: missing wallet tx\n");
                                continue;
                            }
                            auto* conf = wtx->state<TxStateConfirmed>();
                            if (!conf) {
                                LogDebug(BCLog::STAKING, "BitGoldStaker: staking tx not confirmed\n");
                                continue;
                            }
                            confirmed_block_hash = conf->confirmed_block_hash;
                        }
                        const CBlockIndex* pindexFrom = chainman.m_blockman.LookupBlockIndex(confirmed_block_hash);
                        if (!pindexFrom) {
                            LogDebug(BCLog::STAKING, "BitGoldStaker: staking tx block not found\n");
                            continue;
                        }

                        unsigned int nTimeTx = std::max<int64_t>(pindexPrev->GetMedianTimePast() + 1,
                                                                 TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()));
                        nTimeTx &= ~STAKE_TIMESTAMP_MASK;
                        unsigned int nBits = pindexPrev->nBits;
                        uint256 hash_proof;
                        LogTrace(BCLog::STAKING, "BitGoldStaker: checking kernel for %s", stake_out.outpoint.ToString());
                        if (!CheckStakeKernelHash(pindexPrev, nBits, pindexFrom->GetBlockHash(), pindexFrom->nTime,
                                                  stake_out.txout.nValue, stake_out.outpoint, nTimeTx, hash_proof, true)) {
                            LogDebug(BCLog::STAKING, "BitGoldStaker: kernel check failed\n");
                            continue;
                        }

                        CMutableTransaction coinstake;
                        coinstake.nLockTime = pindexPrev->nHeight + 1;
                        coinstake.vin.emplace_back(stake_out.outpoint);
                        coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
                        coinstake.vout.resize(2);
                        coinstake.vout[0].SetNull();
                        coinstake.vout[1].nValue = stake_out.txout.nValue + GetBlockSubsidy(pindexPrev->nHeight + 1, consensus);
                        coinstake.vout[1].scriptPubKey = stake_out.txout.scriptPubKey;
                        {
                            LOCK(m_wallet.cs_wallet);
                            if (!m_wallet.SignTransaction(coinstake)) {
                                LogDebug(BCLog::STAKING, "BitGoldStaker: failed to sign coinstake\n");
                                continue;
                            }
                        }

                        CMutableTransaction coinbase;
                        coinbase.vin.resize(1);
                        coinbase.vin[0].prevout.SetNull();
                        coinbase.vin[0].nSequence = CTxIn::MAX_SEQUENCE_NONFINAL;
                        coinbase.vin[0].scriptSig = CScript() << (pindexPrev->nHeight + 1) << OP_0;
                        coinbase.vout.resize(1);
                        coinbase.vout[0].nValue = 0;
                        coinbase.nLockTime = pindexPrev->nHeight + 1;

                        CBlock block;
                        block.vtx.emplace_back(MakeTransactionRef(std::move(coinbase)));
                        block.vtx.emplace_back(MakeTransactionRef(std::move(coinstake)));
                        block.hashPrevBlock = pindexPrev->GetBlockHash();
                        block.nVersion = chainman.m_versionbitscache.ComputeBlockVersion(pindexPrev, consensus);
                        block.nTime = nTimeTx;
                        block.nBits = pindexPrev->nBits;
                        block.nNonce = 0;
                        block.hashMerkleRoot = BlockMerkleRoot(block);

                        {
                            LOCK(cs_main);
                            if (!ContextualCheckProofOfStake(block, pindexPrev,
                                                              chainman.ActiveChainstate().CoinsTip(),
                                                              chainman.ActiveChain(), consensus)) {
                                LogDebug(BCLog::STAKING, "BitGoldStaker: produced block failed CheckProofOfStake\n");
                                continue;
                            }
                        }

                        bool new_block{false};
                        if (!chainman.ProcessNewBlock(std::make_shared<const CBlock>(block),
                                                      /*force_processing=*/true, /*min_pow_checked=*/true,
                                                      &new_block)) {
                            LogDebug(BCLog::STAKING, "BitGoldStaker: ProcessNewBlock failed\n");
                            continue;
                        }
                        LogPrintLevel(BCLog::STAKING, BCLog::Level::Info,
                                       "BitGoldStaker: staked block %s\n",
                                       block.GetHash().ToString());
                        staked = true;
                        break;
                    }
                }
            }
        } catch (const std::exception& e) {
            LogDebug(BCLog::STAKING, "BitGoldStaker exception: %s\n", e.what());
        }

        if (staked) {
            sleep_time = std::chrono::milliseconds{500};
        } else {
            sleep_time = std::min(sleep_time * 2, std::chrono::milliseconds{8000});
        }
        std::this_thread::sleep_for(sleep_time);
    }
}

} // namespace wallet
