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
    m_thread = std::thread(&BitGoldStaker::ThreadStakeMiner, this);
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

void BitGoldStaker::ThreadStakeMiner()
{
    interfaces::Chain& chain = m_wallet.chain();
    node::NodeContext* node_context = chain.context();
    if (!node_context || !node_context->chainman) {
        LogDebug(BCLog::STAKING, "ThreadStakeMiner: chainman unavailable\n");
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

            std::vector<COutput> candidates = m_wallet.GetStakeableCoins(min_depth, min_age, MIN_STAKE_AMOUNT);

            if (candidates.empty()) {
                LogDebug(BCLog::STAKING, "ThreadStakeMiner: no eligible UTXOs\n");
            } else {
                CBlockIndex* pindexPrev;
                {
                    LOCK(::cs_main);
                    pindexPrev = chainman.ActiveChain().Tip();
                }
                if (!pindexPrev) {
                    LogDebug(BCLog::STAKING, "ThreadStakeMiner: no tip block\n");
                } else {
                    for (const COutput& stake_out : candidates) {
                        uint256 confirmed_block_hash;
                        {
                            LOCK(m_wallet.cs_wallet);
                            const CWalletTx* wtx = m_wallet.GetWalletTx(stake_out.outpoint.hash);
                            if (!wtx) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: missing wallet tx\n");
                                continue;
                            }
                            auto* conf = wtx->state<TxStateConfirmed>();
                            if (!conf) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: staking tx not confirmed\n");
                                continue;
                            }
                            confirmed_block_hash = conf->confirmed_block_hash;
                        }
                        const CBlockIndex* pindexFrom;
                        {
                            LOCK(::cs_main);
                            pindexFrom = chainman.m_blockman.LookupBlockIndex(confirmed_block_hash);
                        }
                        if (!pindexFrom) {
                            LogDebug(BCLog::STAKING, "ThreadStakeMiner: staking tx block not found\n");
                            continue;
                        }

                        unsigned int nTimeTx = std::max<int64_t>(pindexPrev->GetMedianTimePast() + 1,
                                                                 TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()));
                        nTimeTx &= ~consensus.nStakeTimestampMask;
                        unsigned int nBits = pindexPrev->nBits;
                        uint256 hash_proof;
                        LogTrace(BCLog::STAKING, "ThreadStakeMiner: checking kernel for %s", stake_out.outpoint.ToString());
                        if (!CheckStakeKernelHash(pindexPrev, nBits, pindexFrom->GetBlockHash(), pindexFrom->nTime,
                                                  stake_out.txout.nValue, stake_out.outpoint, nTimeTx, hash_proof, true,
                                                  consensus)) {
                            LogDebug(BCLog::STAKING, "ThreadStakeMiner: kernel check failed\n");
                            continue;
                        }

                        CMutableTransaction coinstake;
                        coinstake.nLockTime = nTimeTx;
                        coinstake.vin.emplace_back(stake_out.outpoint);
                        coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
                        coinstake.vout.resize(2);
                        coinstake.vout[0].SetNull();
                        coinstake.vout[1].nValue = stake_out.txout.nValue + GetBlockSubsidy(pindexPrev->nHeight + 1, consensus);
                        coinstake.vout[1].scriptPubKey = stake_out.txout.scriptPubKey;
                        {
                            LOCK(m_wallet.cs_wallet);
                            if (!m_wallet.SignTransaction(coinstake)) {
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: failed to sign coinstake\n");
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
                                LogDebug(BCLog::STAKING, "ThreadStakeMiner: produced block failed CheckProofOfStake\n");
                                continue;
                            }
                        }

                        bool new_block{false};
                        if (!chainman.ProcessNewBlock(std::make_shared<const CBlock>(block),
                                                      /*force_processing=*/true, /*min_pow_checked=*/true,
                                                      &new_block)) {
                            LogDebug(BCLog::STAKING, "ThreadStakeMiner: ProcessNewBlock failed\n");
                            continue;
                        }
                        LogPrintLevel(BCLog::STAKING, BCLog::Level::Info,
                                       "ThreadStakeMiner: staked block %s\n",
                                       block.GetHash().ToString());
                        staked = true;
                        break;
                    }
                }
            }
        } catch (const std::exception& e) {
            LogDebug(BCLog::STAKING, "ThreadStakeMiner exception: %s\n", e.what());
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
