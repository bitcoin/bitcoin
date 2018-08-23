// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/utxoscriptindex.h>
#include <util.h>
#include <script/standard.h>
#include <pubkey.h>
#include <txmempool.h>
#include <validation.h>
#include <txdb.h>
#include <ui_interface.h>

#include<boost/thread.hpp>


std::unique_ptr<UtxoScriptIndex> g_utxoscriptindex;

static ScriptHash ScriptIndexHash(const CScript& in){
    return Hash160(in.begin(), in.end());
}

class UtxoScriptIndexDBCursor
{
public:
    UtxoScriptIndexDBCursor(CDBIterator* cursor) : pcursor(cursor){}

    bool GetKey(ScriptHash& key);
    bool GetValue(SerializableUtxoSet& utxoSet);
    unsigned int GetValueSize() const;

    bool Valid() const;
    void Next();

    std::pair<char, ScriptHash> keyTmp;
    std::unique_ptr<CDBIterator> pcursor;
};

class UtxoScriptIndex::DB : public BaseIndex::DB
{
public:
    explicit DB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool WriteUtxos(std::map<ScriptHash, SerializableUtxoSet>& scriptUtxos);
    bool ReadUtxos(const ScriptHash&, SerializableUtxoSet& utxoSet);

    UtxoScriptIndexDBCursor* Cursor() const;
};

UtxoScriptIndex::DB::DB(size_t nCacheSize, bool fMemory, bool fWipe) :
    BaseIndex::DB(GetDataDir() / "indexes" / "coinsbyscript", nCacheSize, fMemory, fWipe)
{

}

bool UtxoScriptIndex::DB::WriteUtxos(std::map<ScriptHash, SerializableUtxoSet>& scriptUtxos)
{
    CDBBatch batch(*this);
    for(const auto& scriptUtxo: scriptUtxos)
    {
        auto key = std::make_pair(DB_UTXO, std::ref(scriptUtxo.first));
        auto& value = scriptUtxo.second;
        if(scriptUtxo.second.empty())
            batch.Erase(key);
        else
            batch.Write(key,value);
    }
    return WriteBatch(batch);
}

bool UtxoScriptIndex::DB::ReadUtxos(const ScriptHash& scriptId, SerializableUtxoSet& utxoSet)
{
    return Read(std::make_pair(DB_UTXO, scriptId), utxoSet);
}

UtxoScriptIndexDBCursor* UtxoScriptIndex::DB::Cursor() const
{
    UtxoScriptIndexDBCursor* cursor = new UtxoScriptIndexDBCursor(
                                    const_cast<CDBWrapper*>(
                                        static_cast<const CDBWrapper*>(this))->NewIterator());
    cursor->pcursor->Seek(DB_UTXO);
    if(!(cursor->pcursor->Valid()))
        cursor->keyTmp.first = 0;
    else
        cursor->pcursor->GetKey(cursor->keyTmp);
    return cursor;
}

bool UtxoScriptIndexDBCursor::GetKey(ScriptHash& key)
{
    if(keyTmp.first == DB_UTXO)
    {
        key = keyTmp.second;
        return true;
    }
    return false;
}

bool UtxoScriptIndexDBCursor::GetValue(SerializableUtxoSet& utxoSet)
{
    return pcursor->GetValue(utxoSet);
}

bool UtxoScriptIndexDBCursor::Valid() const
{
    return keyTmp.first == DB_UTXO;
}

void UtxoScriptIndexDBCursor::Next()
{
    pcursor->Next();
    if(pcursor->Valid() && pcursor->GetKey(keyTmp))
        return;
    else
        keyTmp.first = 0;
}

UtxoScriptIndex::UtxoScriptIndex(size_t n_cache_size, bool f_memory, bool f_wipe) :
    m_db(std::move(MakeUnique<UtxoScriptIndex::DB>(n_cache_size, f_memory, f_wipe)))
{

}

UtxoScriptIndex::~UtxoScriptIndex() {
    UnregisterValidationInterface(this);
}

void UtxoScriptIndex::Start()
{
    RegisterValidationInterface(this);
}

void UtxoScriptIndex::TransactionAddedToMempool(const CTransactionRef &ptxn) {
    LOCK(cs_utxoCacheMempool);
    const CTransaction &tx = *ptxn;
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        if (tx.vout[i].IsNull() || tx.vout[i].scriptPubKey.IsUnspendable())
            continue;

        auto utxoKey = ScriptIndexHash(tx.vout[i].scriptPubKey);
        utxoCacheMempool[utxoKey].insert(COutPoint(tx.GetHash(), static_cast<uint32_t>(i)));
    }
}

void UtxoScriptIndex::TransactionRemovedFromMempool(const CTransactionRef &ptx) {
    LOCK(cs_utxoCacheMempool);
    const CTransaction &tx = *ptx;
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        if (tx.vout[i].IsNull() || tx.vout[i].scriptPubKey.IsUnspendable())
            continue;

        std::map<ScriptHash, SerializableUtxoSet>::iterator it = utxoCacheMempool.find(ScriptIndexHash(tx.vout[i].scriptPubKey));

        if (it != utxoCacheMempool.end())
        {
            it->second.erase(COutPoint(tx.GetHash(), static_cast<uint32_t>(i)));
            if (it->second.empty())
                utxoCacheMempool.erase(it);
        }
    }
}

void UtxoScriptIndex::BlockConnected(const std::shared_ptr<const CBlock> &block,
                                     const CBlockIndex *pindex,
                                     const std::shared_ptr<const CBlockUndo> &blockundo,
                                     const std::vector<CTransactionRef> &txnConflicted)
{
    {
        LOCK(cs_utxoCache);
        removeSpentUtxosOnTipConnected(*block, *blockundo);
        addNewUtxosOnTipConnected(*block);
    }
    WriteBestBlock(block->GetHash());
}

void UtxoScriptIndex::BlockDisconnected(const std::shared_ptr<const CBlock> &block,
                                        const std::shared_ptr<const CBlockUndo> &blockundo) {
    assert(block->vtx.size() > 0);
    LOCK(cs_utxoCache);
    restoreSpentUtxosOnTipDisconnected(*block, *blockundo);
    removeUtxosOnTipDisconnected(*block);
}

BaseIndex::DB& UtxoScriptIndex::GetDB() const { return *m_db; }

void UtxoScriptIndex::removeUtxo(const CTxOut& txout, const COutPoint outpoint)
{
    if(txout.IsNull() || txout.scriptPubKey.IsUnspendable())
        return;
    const ScriptHash utxoCacheKey = ScriptIndexHash(txout.scriptPubKey);
    loadToCache(utxoCacheKey);
    if(utxoCache.count(utxoCacheKey) == 0)
        return;
    SerializableUtxoSet& utxoCacheSet = utxoCache.at(utxoCacheKey);
        utxoCacheSet.erase(outpoint);
}

void UtxoScriptIndex::removeSpentUtxosOnTipConnected(const CBlock& block, const CBlockUndo& blockundo)
{

    for(unsigned int i = 1; i < block.vtx.size(); ++i)
    {
        for(unsigned int j = 0; j < (block.vtx[i])->vin.size(); ++j)
        {
            removeUtxo(blockundo.vtxundo[i-1].vprevout[j].out,
                       block.vtx[i]->vin[j].prevout);
        }
    }
}

void UtxoScriptIndex::removeUtxosOnTipDisconnected(const CBlock& block)
{
    if(block.vtx.size() == 0)
        return;

    unsigned int i = block.vtx.size() - 1;
    while(true)
    {
        for(unsigned int j = 0; j < block.vtx[i]->vout.size(); ++j)
        {
            removeUtxo(block.vtx[i]->vout[j],
                       COutPoint(block.vtx[i]->GetHash(), static_cast<uint32_t>(j)));
        }
        if(i==0)
            break;
        --i;
    }
}

void UtxoScriptIndex::loadToCache(const ScriptHash& key)
{
    if(!(utxoCache[key].empty()))
        return;
    m_db->ReadUtxos(key, utxoCache[key]);
}

void UtxoScriptIndex::addUtxo(const CScript& scriptPubKey, COutPoint outpoint)
{
    CTxDestination dest;
    ExtractDestination(scriptPubKey, dest);
    const ScriptHash utxoCacheKey = ScriptIndexHash(GetScriptForDestination(dest));
    loadToCache(utxoCacheKey);
    SerializableUtxoSet& utxoCacheSet = utxoCache[utxoCacheKey];
    utxoCacheSet.insert(outpoint);
}

void UtxoScriptIndex::restoreSpentUtxosOnTipDisconnected(const CBlock& block, const CBlockUndo& blockundo)
{
    for(unsigned int i = block.vtx.size() - 1; i > 0; --i)
    {
        for(unsigned int j = 0; j < (block.vtx[i])->vin.size(); ++j)
        {
            addUtxo(blockundo.vtxundo[i-1].vprevout[j].out.scriptPubKey,
                    block.vtx[i]->vin[j].prevout);
        }
    }
}

void UtxoScriptIndex::addNewUtxosOnTipConnected(const CBlock& block)
{
    for(unsigned int i = 0; i < block.vtx.size(); ++i)
    {
        for(unsigned int j = 0; j < block.vtx[i]->vout.size(); ++j)
        {
            addUtxo(block.vtx[i]->vout[j].scriptPubKey,
                    COutPoint(block.vtx[i]->GetHash(), static_cast<uint32_t>(j)));
        }
    }
}

void UtxoScriptIndex::getConfirmedUtxos(SerializableUtxoSet& utxoSet, const CScript& script)
{
    const ScriptHash key = ScriptIndexHash(script);
    loadToCache(key);
    utxoSet.insert(utxoCache[key].begin(), utxoCache[key].end());
}

void UtxoScriptIndex::appendMempoolUtxos(SerializableUtxoSet& utxoSet, const ScriptHash& key)
{
        LOCK(cs_utxoCacheMempool);
        Coin dummyCoin;
        utxoSet.insert(utxoCacheMempool[key].begin(), utxoCacheMempool[key].end());
        LOCK(mempool.cs);
        CCoinsViewMemPool view(pcoinsTip.get(), mempool);
        std::set<COutPoint> outpointsToErase{};
        for(const COutPoint& outpoint: utxoSet)
        {
            if(!(view.GetCoin(outpoint, dummyCoin)))
                outpointsToErase.insert(outpoint);
            if(mempool.isSpent(outpoint))
                outpointsToErase.insert(outpoint);
        }
        for(const COutPoint& outpoint: outpointsToErase)
        {
            utxoSet.erase(outpoint);
        }
}

SerializableUtxoSet UtxoScriptIndex::getUtxosForScript(const CScript& script, unsigned int minConf)
{
    const ScriptHash key = ScriptIndexHash(script);
    LOCK(cs_utxoCache);

    SerializableUtxoSet retSet{};
    getConfirmedUtxos(retSet, script);
    if(minConf == 0)
        appendMempoolUtxos(retSet, key);

    return retSet;
}

bool UtxoScriptIndex::Flush()
{
    LOCK(cs_utxoCache);
    bool writeResult = m_db->WriteUtxos(utxoCache);
    if(writeResult == true)
        utxoCache.clear();
    return writeResult;
}

int64_t UtxoScriptIndex::countCoins(const std::unique_ptr<CCoinsViewDB>& coins)
{
    std::unique_ptr<CCoinsViewCursor> coinsCursor(coins->Cursor());
    int64_t coinsCount = 0;
    while(coinsCursor->Valid())
    {
        ++coinsCount;
        coinsCursor->Next();
    }
    return coinsCount;
}

bool UtxoScriptIndex::createUtxoRecordFromCoin(const std::unique_ptr<CCoinsViewCursor>& coinsCursor)
{
    COutPoint outpoint;
    Coin coin;

    if(!(coinsCursor->GetKey(outpoint)) || !(coinsCursor->GetValue(coin)))
        return false;

    if(!(coin.out.IsNull()) && !(coin.out.scriptPubKey.IsUnspendable()))
    {
        LOCK(cs_utxoCache);
        const ScriptHash key = ScriptIndexHash(coin.out.scriptPubKey);
        if(utxoCache.count(key) == 0)
        {
            SerializableUtxoSet utxos;
            m_db->ReadUtxos(key, utxos);
            utxoCache.insert(std::make_pair(key, utxos));
        }
        loadToCache(key);
        utxoCache[key].insert(outpoint);
    }
    return true;
}

bool UtxoScriptIndex::GenerateIndex(const std::unique_ptr<CCoinsViewDB>& coins)
{
    int64_t coinsCount = countCoins(coins);
    int64_t coinsProcessed = 0;

    LogPrintf("Generating utxo script index, coinsCount:%d\n", coinsCount);

    std::unique_ptr<CCoinsViewCursor> coinsCursor(coins->Cursor());

    while(coinsCursor->Valid())
    {
        try{
            boost::this_thread::interruption_point();

            if(coinsCount > 0 && coinsProcessed % 1000 == 0)
                uiInterface.ShowProgress(_("Building address index..."), (int)(((double)coinsProcessed / (double)coinsCount) * (double)100), false);

            if(!createUtxoRecordFromCoin(coinsCursor)){
                LogPrintf("Error creating utxo record from coin\n");
                return false;
            }

            if(utxoCache.size() > DB_UTXO_FLUSH_FREQUENCY)
                Flush();

            coinsProcessed++;
            coinsCursor->Next();
        }
        catch(std::exception &e){
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    WriteBestBlock(coins->GetBestBlock());
    Flush();
    return true;
}

bool UtxoScriptIndex::removeUtxoRecord(const std::unique_ptr<UtxoScriptIndexDBCursor>& cursor,
                                       std::map<ScriptHash, SerializableUtxoSet>& keysToDelete,
                                       int64_t& counter)
{
    ScriptHash key;
    if(!(cursor->GetKey(key)))
        return false;

    keysToDelete.emplace(std::make_pair(key, SerializableUtxoSet{}));
    ++counter;
    if(keysToDelete.size() > DB_UTXO_FLUSH_FREQUENCY)
    {
        m_db->WriteUtxos(keysToDelete);
        keysToDelete.clear();
    }
    cursor->Next();
    return true;
}

bool UtxoScriptIndex::DeleteIndex()
{
    LogPrintf("Deleting utxo script index\n");
    std::unique_ptr<UtxoScriptIndexDBCursor> cursor(m_db->Cursor());
    std::map<ScriptHash, SerializableUtxoSet> keysToDelete;

    int64_t counter = 0;
    while(cursor->Valid())
    {
        boost::this_thread::interruption_point();
        try
        {
            if(!removeUtxoRecord(cursor, keysToDelete, counter))
                break;
        }
        catch(std::exception &e)
        {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    if(!keysToDelete.empty())
    {
        m_db->WriteUtxos(keysToDelete);
        keysToDelete.clear();
    }
    WriteBestBlock(uint256());
    LogPrintf("Address index with %d addresses successfully deleted.\n", counter);
    return true;
}

bool UtxoScriptIndex::WriteBestBlock(const uint256& value)
{
    CDBBatch batch(*m_db);
    batch.Write(DB_UTXO_BEST_BLOCK, value);
    return m_db->WriteBatch(batch);
}

bool UtxoScriptIndex::ReadBestBlock(uint256& value)
{
    return m_db->Read(DB_UTXO_BEST_BLOCK, value);
}
