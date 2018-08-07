#include <coins.h>
#include <dbwrapper.h>
#include <validationinterface.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <script/standard.h>
#include <sync.h>
#include <undo.h>
#include <index/base.h>

#include <map>
#include <memory>

static const char DB_UTXO = 'd';
static const char DB_UTXO_BEST_BLOCK = 'D';

const unsigned int DB_UTXO_FLUSH_FREQUENCY = 10000;

class CCoinsViewDB;
class UtxoIndexDBCursor;

class SerializableUtxoSet : public std::set<COutPoint>
{
public:
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        //READWRITE(setCoins);
		READWRITE(static_cast<std::set<COutPoint>&>(*this));
    }	
};


class UtxoIndex final : public BaseIndex
{
protected:
	class DB;
public:
	explicit UtxoIndex(size_t n_cache_size, bool f_memory, bool f_wipe);

	~UtxoIndex() override;
//	UtxoIndex(UtxoIndex&) = delete;	
//	UtxoIndex& operator=(const UtxoIndex&) = delete;

	void Start();

	void TransactionAddedToMempool(const CTransactionRef &ptxn) override;
    void TransactionRemovedFromMempool(const CTransactionRef &ptx) override;
	
	void BlockConnected(const std::shared_ptr<const CBlock> &block, 
						const CBlockIndex *pindex, 
						const std::shared_ptr<const CBlockUndo> &blockundo, 
						const std::vector<CTransactionRef> &txnConflicted) override;
	
	void BlockDisconnected(	const std::shared_ptr<const CBlock> &block,
							const std::shared_ptr<const CBlockUndo> &blockundo) override;

	BaseIndex::DB& GetDB() const;
    const char* GetName() const override { return "utxoindex"; }
	
	/*void UpdateUtxoIndexOnTipConnected(const CBlock& block, const CBlockUndo& blockundo);
	void UpdateUtxoIndexOnTipDisconnected(const CBlock& block, const CBlockUndo& blockundo);
	void updateIndexOnTransactionAddedToMempool(const CTransaction& tx); 
    void updateIndexOnTransactionRemovedFromMempool(const CTransaction& tx);
*/	SerializableUtxoSet getUtxosForScript(const CScript& script, unsigned int minConf);
	bool Flush();
	bool GenerateUtxoIndex(const std::unique_ptr<CCoinsViewDB>&);
	bool DeleteUtxoIndex();
	bool WriteBestBlock(const uint256& value);
	bool ReadBestBlock(uint256& value);

private:
	const std::unique_ptr<DB> m_db;
	std::map<CScriptID, SerializableUtxoSet> utxoCache;
	std::map<CScriptID, SerializableUtxoSet> utxoCacheMempool;
	CCriticalSection cs_utxoCache;
	CCriticalSection cs_utxoCacheMempool;
	
	void loadToCache(const CScriptID& key);
	int64_t countCoins(const std::unique_ptr<CCoinsViewDB>& coins);
	void removeSpentUtxosOnTipConnected(const CBlock& block, const CBlockUndo& blockundo);
	void addNewUtxosOnTipConnected(const CBlock& block);
	void restoreSpentUtxosOnTipDisconnected(const CBlock& block, const CBlockUndo& blockundo);
	void removeUtxosOnTipDisconnected(const CBlock& block);
	void removeUtxo(const CTxOut& txout, COutPoint outpoint);
	void addUtxo(const CScript& scriptPubKey, COutPoint outpoint);
	void getConfirmedUtxos(SerializableUtxoSet& utxoSet, const CScript& script);
	void appendMempoolUtxos(SerializableUtxoSet& utxoSet, const CScriptID& key);
	bool createUtxoRecordFromCoin(const std::unique_ptr<CCoinsViewCursor>& coinsCursor);
	bool removeUtxoRecord(	const std::unique_ptr<UtxoIndexDBCursor>& cursor,
							std::map<CScriptID, SerializableUtxoSet>& keysToDelete,
							int64_t counter);
};

class UtxoIndexDBCursor
{
public:
	UtxoIndexDBCursor(CDBIterator* cursor) : pcursor(cursor){}

	bool GetKey(CScriptID& key);
	bool GetValue(SerializableUtxoSet& utxoSet);
	unsigned int GetValueSize() const;

	bool Valid() const;
	void Next();

//private:
	std::pair<char, CScriptID> keyTmp;	
	std::unique_ptr<CDBIterator> pcursor;

//	friend class UtxoIndex::DB;
};
extern std::unique_ptr<UtxoIndex> g_utxoindex;
