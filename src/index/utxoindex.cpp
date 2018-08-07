#include "index/utxoindex.h"
//#include <util.h>
#include <script/standard.h>
#include <pubkey.h>
#include <txmempool.h>
#include <validation.h>
#include <txdb.h>
#include <ui_interface.h>

#include<boost/thread.hpp>


std::unique_ptr<UtxoIndex> g_utxoindex;


class UtxoIndex::DB : public BaseIndex::DB
{
public:
	explicit DB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

	bool WriteUtxos(std::map<CScriptID, SerializableUtxoSet>& scriptUtxos);
	bool ReadUtxos(const CScriptID&, SerializableUtxoSet& utxoSet);

	UtxoIndexDBCursor* Cursor() const;
};

UtxoIndex::DB::DB(size_t nCacheSize, bool fMemory, bool fWipe) :
	BaseIndex::DB(GetDataDir() / "indexes" / "coinsbyscript", nCacheSize, fMemory, fWipe)
{

}

bool UtxoIndex::DB::WriteUtxos(std::map<CScriptID, SerializableUtxoSet>& scriptUtxos)
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

bool UtxoIndex::DB::ReadUtxos(const CScriptID& scriptId, SerializableUtxoSet& utxoSet)
{
	return Read(std::make_pair(DB_UTXO, scriptId), utxoSet);
}

UtxoIndexDBCursor* UtxoIndex::DB::Cursor() const
{
	UtxoIndexDBCursor* cursor = new UtxoIndexDBCursor(
									const_cast<CDBWrapper*>(
										static_cast<const CDBWrapper*>(this))->NewIterator());
	cursor->pcursor->Seek(DB_UTXO);
	if(not cursor->pcursor->Valid())
		cursor->keyTmp.first = 0;
	else
		cursor->pcursor->GetKey(cursor->keyTmp);
	return cursor;
}

bool UtxoIndexDBCursor::GetKey(CScriptID& key)
{
	if(keyTmp.first == DB_UTXO)
	{
		key = keyTmp.second;
		return true;
	}
	return false;
}

bool UtxoIndexDBCursor::GetValue(SerializableUtxoSet& utxoSet)
{
	return pcursor->GetValue(utxoSet);
}

bool UtxoIndexDBCursor::Valid() const
{
	return keyTmp.first == DB_UTXO;
}

void UtxoIndexDBCursor::Next()
{
	pcursor->Next();
	if(pcursor->Valid() and pcursor->GetKey(keyTmp))
		return;
	else
		keyTmp.first = 0;
}

UtxoIndex::UtxoIndex(size_t n_cache_size, bool f_memory, bool f_wipe) :
	m_db(std::move(MakeUnique<UtxoIndex::DB>(n_cache_size, f_memory, f_wipe)))
{

}

UtxoIndex::~UtxoIndex() {

}

void UtxoIndex::Start()
{
	RegisterValidationInterface(this);
}

void UtxoIndex::TransactionAddedToMempool(const CTransactionRef &ptxn) {
	LOCK(cs_utxoCacheMempool);
	const CTransaction &tx = *ptxn;
	for (unsigned int i = 0; i < tx.vout.size(); i++)
	{
		if (tx.vout[i].IsNull() or tx.vout[i].scriptPubKey.IsUnspendable())
			continue;

		auto utxoKey = CScriptID(tx.vout[i].scriptPubKey);
		utxoCacheMempool[utxoKey].insert(COutPoint(tx.GetHash(), static_cast<uint32_t>(i)));
	}
}

void UtxoIndex::TransactionRemovedFromMempool(const CTransactionRef &ptx) {
	LOCK(cs_utxoCacheMempool);
	const CTransaction &tx = *ptx;
	for (unsigned int i = 0; i < tx.vout.size(); i++)
	{
		if (tx.vout[i].IsNull() or tx.vout[i].scriptPubKey.IsUnspendable())
			continue;
	
		std::map<CScriptID, SerializableUtxoSet>::iterator it = utxoCacheMempool.find(CScriptID(tx.vout[i].scriptPubKey));
	    
		if (it != utxoCacheMempool.end())
		{
	        it->second.erase(COutPoint(tx.GetHash(), static_cast<uint32_t>(i)));
	        if (it->second.empty())
	            utxoCacheMempool.erase(it);
	    }
	}	
}

void UtxoIndex::BlockConnected(	const std::shared_ptr<const CBlock> &block,
								const CBlockIndex *pindex,
								const std::shared_ptr<const CBlockUndo> &blockundo,
								const std::vector<CTransactionRef> &txnConflicted)
{
	assert(block->vtx.size() > 0);
	{
		LOCK(cs_utxoCache);
		removeSpentUtxosOnTipConnected(*block, *blockundo);
		addNewUtxosOnTipConnected(*block);
	}
	WriteBestBlock(block->GetHash());
}

void UtxoIndex::BlockDisconnected(	const std::shared_ptr<const CBlock> &block,
									const std::shared_ptr<const CBlockUndo> &blockundo) {
	assert(block->vtx.size() > 0);
	LOCK(cs_utxoCache);
	restoreSpentUtxosOnTipDisconnected(*block, *blockundo);
	removeUtxosOnTipDisconnected(*block);
}

BaseIndex::DB& UtxoIndex::GetDB() const { return *m_db; }

void UtxoIndex::removeUtxo(const CTxOut& txout, const COutPoint outpoint)
{
	if(txout.IsNull() or txout.scriptPubKey.IsUnspendable())
		return;
	const CScriptID utxoCacheKey = CScriptID(txout.scriptPubKey);
	loadToCache(utxoCacheKey);
	if(utxoCache.count(utxoCacheKey) == 0)
		return;
	SerializableUtxoSet& utxoCacheSet = utxoCache.at(utxoCacheKey);
		utxoCacheSet.erase(outpoint);
}

void UtxoIndex::removeSpentUtxosOnTipConnected(const CBlock& block, const CBlockUndo& blockundo)
{

	for(unsigned int i = 1; i < block.vtx.size(); ++i)
	{
		for(unsigned int j = 0; j < (block.vtx[i])->vin.size(); ++j)
		{
			removeUtxo(	blockundo.vtxundo[i-1].vprevout[j].out,
						block.vtx[i]->vin[j].prevout);
		}
	}
}

void UtxoIndex::removeUtxosOnTipDisconnected(const CBlock& block)
{
	for(unsigned int i = block.vtx.size() - 1; i >= 0; --i)
	{
		for(unsigned int j = 0; j < block.vtx[i]->vout.size(); ++j)
		{
			removeUtxo(	block.vtx[i]->vout[j], 
						COutPoint(block.vtx[i]->GetHash(), static_cast<uint32_t>(j)));
		}
		if(i==0)
			break;
	}
}

void UtxoIndex::loadToCache(const CScriptID& key)
{
	if(not utxoCache[key].empty())
		return;
	m_db->ReadUtxos(key, utxoCache[key]);
}

void UtxoIndex::addUtxo(const CScript& scriptPubKey, COutPoint outpoint)
{
	CTxDestination dest;
	ExtractDestination(scriptPubKey, dest);
	const CScriptID utxoCacheKey(GetScriptForDestination(dest));	
	loadToCache(utxoCacheKey);
	SerializableUtxoSet& utxoCacheSet = utxoCache[utxoCacheKey];
	utxoCacheSet.insert(outpoint);
}

void UtxoIndex::restoreSpentUtxosOnTipDisconnected(const CBlock& block, const CBlockUndo& blockundo)
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

void UtxoIndex::addNewUtxosOnTipConnected(const CBlock& block)
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

void UtxoIndex::getConfirmedUtxos(SerializableUtxoSet& utxoSet, const CScript& script)
{
	const CScriptID key(script);
	loadToCache(key);
	utxoSet.insert(utxoCache[key].begin(), utxoCache[key].end());
}

void UtxoIndex::appendMempoolUtxos(SerializableUtxoSet& utxoSet, const CScriptID& key)
{
		LOCK(cs_utxoCacheMempool);
		Coin dummyCoin;
		utxoSet.insert(utxoCacheMempool[key].begin(), utxoCacheMempool[key].end());
		LOCK(mempool.cs);
		CCoinsViewMemPool view(pcoinsTip.get(), mempool);
		for(const COutPoint& outpoint: utxoSet)
		{
			if(not view.GetCoin(outpoint, dummyCoin))
				utxoSet.erase(outpoint);
			if(mempool.isSpent(outpoint))
				utxoSet.erase(outpoint);
		}
}

SerializableUtxoSet UtxoIndex::getUtxosForScript(const CScript& script, unsigned int minConf)
{
	const CScriptID key(script);
	LOCK(cs_utxoCache);
	
	SerializableUtxoSet retSet{};
	getConfirmedUtxos(retSet, script);
	if(minConf == 0)
		appendMempoolUtxos(retSet, key);
	
	return retSet;
}

bool UtxoIndex::Flush()
{
	LOCK(cs_utxoCache);
	bool writeResult = m_db->WriteUtxos(utxoCache);
	if(writeResult == true)
		utxoCache.clear();	
	return writeResult;
}

int64_t UtxoIndex::countCoins(const std::unique_ptr<CCoinsViewDB>& coins)
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

bool UtxoIndex::createUtxoRecordFromCoin(const std::unique_ptr<CCoinsViewCursor>& coinsCursor)
{
	COutPoint outpoint;
	Coin coin;

	if(not coinsCursor->GetKey(outpoint) or not coinsCursor->GetValue(coin))
		return false;

	if(not coin.out.IsNull() and not coin.out.scriptPubKey.IsUnspendable())
	{
		LOCK(cs_utxoCache);
		const CScriptID key = CScriptID(coin.out.scriptPubKey);
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

bool UtxoIndex::GenerateUtxoIndex(const std::unique_ptr<CCoinsViewDB>& coins)
{
    int64_t coinsCount = countCoins(coins);
    int64_t coinsProcessed = 0;
	
    LogPrintf("GenerateUtxoIndex, coinsCount:%d\n", coinsCount);
	
    std::unique_ptr<CCoinsViewCursor> coinsCursor(coins->Cursor());

    while(coinsCursor->Valid())
    {
	    try{
            boost::this_thread::interruption_point();
		
            if(coinsCount > 0 and coinsProcessed % 1000 == 0)
                uiInterface.ShowProgress(_("Building address index..."), (int)(((double)coinsProcessed / (double)coinsCount) * (double)100), false);

            if(not createUtxoRecordFromCoin(coinsCursor)){
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

bool UtxoIndex::removeUtxoRecord(	const std::unique_ptr<UtxoIndexDBCursor>& cursor,
									std::map<CScriptID, SerializableUtxoSet>& keysToDelete,
									int64_t counter)
{
	CScriptID key;
	if(not cursor->GetKey(key))
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

bool UtxoIndex::DeleteUtxoIndex()
{
	LogPrintf("DeleteUtxoIndex\n");
	std::unique_ptr<UtxoIndexDBCursor> cursor(m_db->Cursor());
	std::map<CScriptID, SerializableUtxoSet> keysToDelete;
	
	int64_t counter = 0;
	while(cursor->Valid())
	{
		boost::this_thread::interruption_point();
		try
		{
			if(not removeUtxoRecord(cursor, keysToDelete, counter))
				break;
		}
		catch(std::exception &e)
		{
			return error("%s : Deserialize or I/O error - %s", __func__, e.what());
		}
	}
	if(not keysToDelete.empty())
	{
		m_db->WriteUtxos(keysToDelete);
		keysToDelete.clear();
	}
	WriteBestBlock(uint256());
	LogPrintf("Address index with %d addresses successfully deleted.\n", counter);	
	return true;
}
	
bool UtxoIndex::WriteBestBlock(const uint256& value)
{
	CDBBatch batch(*m_db);
	batch.Write(DB_UTXO_BEST_BLOCK, value);
	return m_db->WriteBatch(batch);
}
	
bool UtxoIndex::ReadBestBlock(uint256& value)
{
	return m_db->Read(DB_UTXO_BEST_BLOCK, value);
}
