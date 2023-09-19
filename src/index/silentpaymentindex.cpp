#include <index/silentpaymentindex.h>

#include <chainparams.h>
#include <coins.h>
#include <common/args.h>
#include <index/disktxpos.h>
#include <node/blockstorage.h>
#include <pubkey.h>
#include <primitives/transaction.h>
// TODO: move GetSilentPaymentsTweakDataFromTxInputs out of wallet?
#include <wallet/silentpayments.h>
#include <undo.h>
#include <validation.h>

#include <dbwrapper.h>
#include <hash.h>

constexpr uint8_t DB_SILENT_PAYMENT_INDEX{'s'};

std::unique_ptr<SilentPaymentIndex> g_silent_payment_index;

/** Access to the silent payment index database (indexes/silentpaymentindex/) */
class SilentPaymentIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    bool WriteSilentPayments(const std::pair<uint256, std::vector<CPubKey>>& tweaks);
};

SilentPaymentIndex::DB::DB(size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / "silentpaymentindex", n_cache_size, f_memory, f_wipe)
{}

bool SilentPaymentIndex::DB::WriteSilentPayments(const std::pair<uint256, std::vector<CPubKey>>& tweaks)
{
    CDBBatch batch(*this);
    batch.Write(std::make_pair(DB_SILENT_PAYMENT_INDEX, tweaks.first), tweaks.second);
    return WriteBatch(batch);
}

SilentPaymentIndex::SilentPaymentIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), "silentpaymentindex"), m_db(std::make_unique<SilentPaymentIndex::DB>(n_cache_size, f_memory, f_wipe))
{}

SilentPaymentIndex::~SilentPaymentIndex() {}

bool SilentPaymentIndex::GetSilentPaymentKeys(std::vector<CTransactionRef> txs, CBlockUndo& block_undo, std::vector<CPubKey>& tweaked_pub_key_sums)
{
    assert(txs.size() - 1 == block_undo.vtxundo.size());

    for (uint32_t i=0; i < txs.size(); i++) {
        auto& tx = txs.at(i);

        if (tx->IsCoinBase()) {
            continue;
        }

        if (std::none_of(tx->vout.begin(), tx->vout.end(), [](const CTxOut& txout) {
            std::vector<std::vector<unsigned char>> solutions;
            return Solver(txout.scriptPubKey, solutions) == TxoutType::WITNESS_V1_TAPROOT;
        })) {
            continue;
        }

        // -1 as blockundo does not have coinbase tx
        CTxUndo undoTX{block_undo.vtxundo.at(i - 1)};
        std::map<COutPoint, Coin> coins;

        for (uint32_t j = 0; j < tx->vin.size(); j++) {
            coins[tx->vin.at(j).prevout] = undoTX.vprevout.at(j);
        }

        const auto tweak_data = wallet::GetSilentPaymentsTweakDataFromTxInputs(tx->vin, coins);

        if (!tweak_data) continue;

        const uint256 outpoint_hash = tweak_data->first;
        CPubKey tweaked_pub_key_sum = tweak_data->second;
        if (!tweaked_pub_key_sum.TweakAdd(outpoint_hash.begin())) {
            // TODO: Is this bad??
            continue;
        }
        assert(tweaked_pub_key_sum.IsFullyValid());
        tweaked_pub_key_sums.emplace_back(tweaked_pub_key_sum);
    }

    // TODO: so this can never fail?
    return true;


}

bool SilentPaymentIndex::CustomAppend(const interfaces::BlockInfo& block)
{
    // Exclude genesis block transaction because outputs are not spendable.
    if (block.height == 0) return true;

    assert(block.data);

    Consensus::Params consensus = Params().GetConsensus();

    if (block.height < consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height) {
        return true;
    }

    std::vector<std::pair<uint256, CPubKey>> items;

    const CBlockIndex* block_index = WITH_LOCK(cs_main, return m_chainstate->m_blockman.LookupBlockIndex(block.hash));
    // TODO: fix sloppy rebase, DANGER!
    assert(block_index != nullptr);


    CBlockUndo block_undo;

    if (!(m_chainstate->m_blockman.UndoReadFromDisk(block_undo, *block_index))) { // && m_chainstate->m_blockman.ReadBlockFromDisk(*(block.data), *block_index))) {
        // TODO: assert or throw exception? This should be impossible on an unpruned node.
        return false;
    }

    std::vector<CPubKey> tweaked_pub_key_sums;
    GetSilentPaymentKeys(block.data->vtx, block_undo, tweaked_pub_key_sums);

    return m_db->WriteSilentPayments(make_pair(block.hash, tweaked_pub_key_sums));
}

bool SilentPaymentIndex::FindSilentPayment(const uint256& block_hash, std::vector<CPubKey>& tweaked_pub_key_sums) const
{
    return m_db->Read(std::make_pair(DB_SILENT_PAYMENT_INDEX, block_hash), tweaked_pub_key_sums);
}

BaseIndex::DB& SilentPaymentIndex::GetDB() const { return *m_db; }