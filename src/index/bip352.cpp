#include <index/bip352.h>

#include <common/bip352.h>
#include <chainparams.h>
#include <coins.h>
#include <common/args.h>
#include <index/disktxpos.h>
#include <node/blockstorage.h>
#include <pubkey.h>
#include <primitives/transaction.h>

#include <undo.h>
#include <util/fs.h>
#include <validation.h>

#include <dbwrapper.h>
#include <hash.h>

constexpr uint8_t DB_SILENT_PAYMENT_INDEX{'s'};
/* Save space on mainnet by starting the index at Taproot activation.
 * Copying the height here assuming DEPLOYMENT_TAPROOT will be dropped:
 * https://github.com/bitcoin/bitcoin/pull/26201/
 * Only apply this storage optimization on mainnet.
 */
const int TAPROOT_MAINNET_ACTIVATION_HEIGHT{709632};

std::unique_ptr<BIP352Index> g_bip352_index;
std::unique_ptr<BIP352Index> g_bip352_ct_index;

/** Access to the silent payment index database (indexes/bip352/) */
class BIP352Index::DB : public BaseIndex::DB
{
public:
    explicit DB(fs::path file_name, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    bool WriteSilentPayments(const std::pair<uint256, tweak_index_entry>& kv);
};

BIP352Index::DB::DB(fs::path file_name, size_t n_cache_size, bool f_memory, bool f_wipe) :
    BaseIndex::DB(gArgs.GetDataDirNet() / "indexes" / file_name, n_cache_size, f_memory, f_wipe)
{}

bool BIP352Index::DB::WriteSilentPayments(const std::pair<uint256, tweak_index_entry>& kv)
{
    CDBBatch batch(*this);
    batch.Write(std::make_pair(DB_SILENT_PAYMENT_INDEX, kv.first), kv.second);
    return WriteBatch(batch);
}

BIP352Index::BIP352Index(bool cut_through, std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex(std::move(chain), strprintf("bip352 %sindex", cut_through ? "cut-through " : ""), /*start_height=*/Params().IsTestChain() ? 0 : TAPROOT_MAINNET_ACTIVATION_HEIGHT), m_db(std::make_unique<BIP352Index::DB>(fs::u8path(strprintf("bip352%s", cut_through ? "ct" : "")), n_cache_size, f_memory, f_wipe))
{
    m_cut_through = cut_through;
}

BIP352Index::~BIP352Index() = default;

bool BIP352Index::GetSilentPaymentKeys(std::vector<CTransactionRef> txs, CBlockUndo& block_undo, tweak_index_entry& index_entry)
{
    assert(txs.size() - 1 == block_undo.vtxundo.size());

    for (uint32_t i=0; i < txs.size(); i++) {
        auto& tx = txs.at(i);

        if (!BIP352::MaybeSilentPayment(tx)) continue;

        // -1 as blockundo does not have coinbase tx
        CTxUndo undoTX{block_undo.vtxundo.at(i - 1)};
        std::map<COutPoint, Coin> coins;

        for (uint32_t j = 0; j < tx->vin.size(); j++) {
            coins[tx->vin.at(j).prevout] = undoTX.vprevout.at(j);
        }

        std::optional<CPubKey> tweaked_pk = BIP352::GetSerializedSilentPaymentsPublicData(tx->vin, coins);
        if (tweaked_pk) {
            // Used to filter dust. To keep the index small we use only one byte
            // and measure in hexasats.
            uint8_t max_output_hsat = 0;
            for (const CTxOut& txout : tx->vout) {
                if (!txout.scriptPubKey.IsPayToTaproot()) continue;
                uint8_t output_hsat = txout.nValue > max_dust_threshold ? UINT8_MAX : txout.nValue >> dust_shift;
                max_output_hsat = std::max(output_hsat, max_output_hsat);
            }

            if (m_cut_through) {
                // Skip entry if all outputs have been spent.
                // This is only effective when the index is generated while
                // the tip is far ahead.
                //
                // This is done after calculating the tweak in order to minimize
                // the number of UTXO lookups.
                LOCK(cs_main);
                const CCoinsViewCache& coins_cache = m_chainstate->CoinsTip();

                uint32_t spent{0};
                for (uint32_t i{0}; i < tx->vout.size(); i++) {
                    COutPoint outpoint(tx->GetHash(), i);
                    // Many new blocks may be processed while generating the index,
                    // in between HaveCoin calls. This is not a problem, because
                    // the cut-through index can safely have false positives.
                    if (!coins_cache.HaveCoin(outpoint)) spent++;
                }
                if (spent == tx->vout.size()) continue;
            }
            index_entry.emplace_back(std::make_pair(tweaked_pk.value(), max_output_hsat));
        }
    }

    return true;
}

bool BIP352Index::CustomAppend(const interfaces::BlockInfo& block)
{
    // Exclude genesis block transaction because outputs are not spendable. This
    // is needed on non-mainnet chains because m_start_height is 0 by default.
    if (block.height == 0) return true;

    // Exclude pre-taproot
    if (block.height < m_start_height) return true;

    assert(block.data);

    std::vector<std::pair<uint256, CPubKey>> items;

    const CBlockIndex* block_index = WITH_LOCK(cs_main, return m_chainstate->m_blockman.LookupBlockIndex(block.hash));
    // TODO: fix sloppy rebase, DANGER!
    assert(block_index != nullptr);


    CBlockUndo block_undo;

    if (!(m_chainstate->m_blockman.UndoReadFromDisk(block_undo, *block_index))) {
        // Should be impossible on an unpruned node
        FatalErrorf("Failed to read undo file for %s", GetName());
        return false;
    };

    tweak_index_entry index_entry;
    GetSilentPaymentKeys(block.data->vtx, block_undo, index_entry);

    return m_db->WriteSilentPayments(make_pair(block.hash, index_entry));
}

bool BIP352Index::FindSilentPayment(const uint256& block_hash, tweak_index_entry& index_entry) const
{
    return m_db->Read(std::make_pair(DB_SILENT_PAYMENT_INDEX, block_hash), index_entry);
}

BaseIndex::DB& BIP352Index::GetDB() const { return *m_db; }
