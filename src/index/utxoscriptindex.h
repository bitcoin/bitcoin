// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_UTXOSCRIPTINDEX_H
#define BITCOIN_INDEX_UTXOSCRIPTINDEX_H

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
class UtxoScriptIndexDBCursor;

class SerializableUtxoSet : public std::set<COutPoint>
{
public:
	ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
		READWRITE(static_cast<std::set<COutPoint>&>(*this));
    }
};

typedef uint160 ScriptHash;

class UtxoScriptIndex final : public BaseIndex
{
protected:
	class DB;
public:
	explicit UtxoScriptIndex(size_t n_cache_size, bool f_memory, bool f_wipe);

	~UtxoScriptIndex() override;

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
    const char* GetName() const override { return "utxoscriptindex"; }

	SerializableUtxoSet getUtxosForScript(const CScript& script, unsigned int minConf);
	bool Flush();
	bool GenerateIndex(const std::unique_ptr<CCoinsViewDB>&);
	bool DeleteIndex();
	bool WriteBestBlock(const uint256& value);
	bool ReadBestBlock(uint256& value);

private:
	const std::unique_ptr<DB> m_db;
	std::map<ScriptHash, SerializableUtxoSet> utxoCache;
	std::map<ScriptHash, SerializableUtxoSet> utxoCacheMempool;
	CCriticalSection cs_utxoCache;
	CCriticalSection cs_utxoCacheMempool;

	void loadToCache(const ScriptHash& key);
	int64_t countCoins(const std::unique_ptr<CCoinsViewDB>& coins);
	void removeSpentUtxosOnTipConnected(const CBlock& block, const CBlockUndo& blockundo);
	void addNewUtxosOnTipConnected(const CBlock& block);
	void restoreSpentUtxosOnTipDisconnected(const CBlock& block, const CBlockUndo& blockundo);
	void removeUtxosOnTipDisconnected(const CBlock& block);

	void removeUtxo(const CTxOut& txout, COutPoint outpoint);
	void addUtxo(const CScript& scriptPubKey, COutPoint outpoint);

	void getConfirmedUtxos(SerializableUtxoSet& utxoSet, const CScript& script);

	void appendMempoolUtxos(SerializableUtxoSet& utxoSet, const ScriptHash& key);
	bool createUtxoRecordFromCoin(const std::unique_ptr<CCoinsViewCursor>& coinsCursor);
	bool removeUtxoRecord(	const std::unique_ptr<UtxoScriptIndexDBCursor>& cursor,
							std::map<ScriptHash, SerializableUtxoSet>& keysToDelete,
							int64_t& counter);
};

extern std::unique_ptr<UtxoScriptIndex> g_utxoscriptindex;

#endif
