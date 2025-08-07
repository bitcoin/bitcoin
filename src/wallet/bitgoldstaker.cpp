#include <wallet/bitgoldstaker.h>
#include <wallet/wallet.h>
#include <logging.h>
#include <chrono>
#include <interfaces/chain.h>
#include <node/context.h>
#include <pos/stake.h>
#include <pow.h>
#include <validation.h>
#include <util/time.h>

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

void BitGoldStaker::ThreadStaker()
{
    interfaces::Chain& chain = m_wallet.chain();
    node::NodeContext* node_context = chain.context();
    if (!node_context || !node_context->chainman) {
        LogPrintf("BitGoldStaker: chainman unavailable\n");
        return;
    }
    ChainstateManager& chainman = *node_context->chainman;
    const Consensus::Params& consensus = chainman.GetParams().GetConsensus();

    while (!m_stop) {
        try {
            std::optional<COutput> stake_out;
            {
                LOCK(m_wallet.cs_wallet);
                for (const COutput& out : AvailableCoins(m_wallet).All()) {
                    if (!out.spendable || out.depth <= 0) continue;
                    stake_out = out;
                    break;
                }
            }
            if (!stake_out) {
                LogPrintf("BitGoldStaker: no mature UTXOs\n");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            LOCK(::cs_main);
            CBlockIndex* pindexPrev = chainman.ActiveChain().Tip();
            if (!pindexPrev) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            const CWalletTx* wtx = m_wallet.GetWalletTx(stake_out->outpoint.hash);
            if (!wtx) {
                LogPrintf("BitGoldStaker: missing wallet tx\n");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            auto* conf = wtx->state<TxStateConfirmed>();
            if (!conf) {
                LogPrintf("BitGoldStaker: staking tx not confirmed\n");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            const CBlockIndex* pindexFrom = chainman.m_blockman.LookupBlockIndex(conf->confirmed_block_hash);
            if (!pindexFrom) {
                LogPrintf("BitGoldStaker: staking tx block not found\n");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            CBlock block_from;
            block_from.nVersion = pindexFrom->nVersion;
            block_from.hashPrevBlock = pindexFrom->hashPrev;
            block_from.hashMerkleRoot = pindexFrom->hashMerkleRoot;
            block_from.nTime = pindexFrom->nTime;
            block_from.nBits = pindexFrom->nBits;
            block_from.nNonce = pindexFrom->nNonce;

            unsigned int nTimeTx = std::max<int64_t>(pindexPrev->GetMedianTimePast() + 1,
                                                     TicksSinceEpoch<std::chrono::seconds>(NodeClock::now()));
            unsigned int nBits = GetNextWorkRequired(pindexPrev, nullptr, consensus);
            uint256 hash_proof;
            if (!CheckStakeKernelHash(pindexPrev, nBits, block_from, conf->position_in_block,
                                      wtx->tx, stake_out->outpoint, nTimeTx, hash_proof, false)) {
                LogPrintf("BitGoldStaker: kernel check failed\n");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            CMutableTransaction coinstake;
            coinstake.nLockTime = pindexPrev->nHeight + 1;
            coinstake.vin.emplace_back(stake_out->outpoint);
            coinstake.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
            coinstake.vout.resize(2);
            coinstake.vout[0].SetNull();
            coinstake.vout[1].nValue = stake_out->txout.nValue + GetBlockSubsidy(pindexPrev->nHeight + 1, consensus);
            coinstake.vout[1].scriptPubKey = stake_out->txout.scriptPubKey;
            {
                LOCK(m_wallet.cs_wallet);
                if (!m_wallet.SignTransaction(coinstake)) {
                    LogPrintf("BitGoldStaker: failed to sign coinstake\n");
                    std::this_thread::sleep_for(std::chrono::seconds(1));
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
            block.nBits = GetNextWorkRequired(pindexPrev, &block, consensus);
            block.nNonce = 0;
            block.hashMerkleRoot = BlockMerkleRoot(block);

            if (!CheckProofOfStake(block, pindexPrev, consensus)) {
                LogPrintf("BitGoldStaker: produced block failed CheckProofOfStake\n");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            bool new_block{false};
            if (!chainman.ProcessNewBlock(std::make_shared<const CBlock>(block),
                                          /*force_processing=*/true, /*min_pow_checked=*/true,
                                          &new_block)) {
                LogPrintf("BitGoldStaker: ProcessNewBlock failed\n");
            } else {
                LogPrintf("BitGoldStaker: staked block %s\n", block.GetHash().ToString());
            }
        } catch (const std::exception& e) {
            LogPrintf("BitGoldStaker exception: %s\n", e.what());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

} // namespace wallet
