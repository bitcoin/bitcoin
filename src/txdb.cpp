// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txdb.h>

#include <chainparams.h>
#include <hash.h>
#include <random.h>
#include <shutdown.h>
#include <ui_interface.h>
#include <uint256.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>

#include <algorithm>
#include <exception>
#include <map>
#include <set>
#include <unordered_map>

#include <stdint.h>
#include <inttypes.h>

#include <boost/thread.hpp>

/** UTXO version flag */
static const char DB_COIN = 'C';
static const char DB_BLOCK_FILES = 'f';
static const char DB_BLOCK_INDEX = 'b';

static const char DB_BEST_BLOCK = 'B';
static const char DB_HEAD_BLOCKS = 'H';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_LAST_BLOCK = 'l';

static const char DB_COIN_INDEX = 'c';
static const char DB_COIN_BINDPLOTTER = 'r';
static const char DB_COIN_POINT_SEND = 'P';
static const char DB_COIN_POINT_RECEIVE = 'p';
static const char DB_COIN_STAKING_SEND = 'S';
static const char DB_COIN_STAKING_RECEIVE = 's';

namespace {

struct CoinEntry {
    COutPoint* outpoint;
    char key;
    explicit CoinEntry(const COutPoint* outputIn) : outpoint(const_cast<COutPoint*>(outputIn)), key(DB_COIN) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << key;
        s << outpoint->hash;
        s << VARINT(outpoint->n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> key;
        s >> outpoint->hash;
        s >> VARINT(outpoint->n);
    }
};

struct CoinIndexEntry {
    COutPoint* outpoint;
    CAccountID* accountID;
    char key;
    CoinIndexEntry(const COutPoint* outputIn, const CAccountID* accountIDIn) :
        outpoint(const_cast<COutPoint*>(outputIn)),
        accountID(const_cast<CAccountID*>(accountIDIn)),
        key(DB_COIN_INDEX) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << key;
        s << *accountID;
        s << outpoint->hash;
        s << VARINT(outpoint->n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> key;
        s >> *accountID;
        s >> outpoint->hash;
        s >> VARINT(outpoint->n);
    }
};

struct BindPlotterEntry {
    COutPoint* outpoint;
    CAccountID* accountID;
    char key;
    explicit BindPlotterEntry(const COutPoint* outpointIn, const CAccountID* accountIDIn) :
        outpoint(const_cast<COutPoint*>(outpointIn)),
        accountID(const_cast<CAccountID*>(accountIDIn)),
        key(DB_COIN_BINDPLOTTER) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << key;
        s << *accountID;
        s << outpoint->hash;
        s << VARINT(outpoint->n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> key;
        s >> *accountID;
        s >> outpoint->hash;
        s >> VARINT(outpoint->n);
    }
};

struct BindPlotterValue {
    uint64_t* plotterId;
    uint32_t* nHeight;
    BindPlotterValue(const uint64_t* plotterIdIn, const uint32_t* nHeightIn) :
        plotterId(const_cast<uint64_t*>(plotterIdIn)),
        nHeight(const_cast<uint32_t*>(nHeightIn)) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << VARINT(*plotterId);
        s << VARINT(*nHeight);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> VARINT(*plotterId);
        s >> VARINT(*nHeight);
    }
};

struct PointSendEntry {
    COutPoint* outpoint;
    CAccountID* accountID;
    char key;
    PointSendEntry(const COutPoint* outpointIn, const CAccountID* accountIDIn) :
        outpoint(const_cast<COutPoint*>(outpointIn)),
        accountID(const_cast<CAccountID*>(accountIDIn)),
        key(DB_COIN_POINT_SEND) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << key;
        s << *accountID;
        s << outpoint->hash;
        s << VARINT(outpoint->n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> key;
        s >> *accountID;
        s >> outpoint->hash;
        s >> VARINT(outpoint->n);
    }
};

struct PointReceiveEntry {
    COutPoint* outpoint;
    CAccountID* accountID;
    char key;
    PointReceiveEntry(const COutPoint* outpointIn, const CAccountID* accountIDIn) :
        outpoint(const_cast<COutPoint*>(outpointIn)),
        accountID(const_cast<CAccountID*>(accountIDIn)),
        key(DB_COIN_POINT_RECEIVE) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << key;
        s << *accountID;
        s << outpoint->hash;
        s << VARINT(outpoint->n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> key;
        s >> *accountID;
        s >> outpoint->hash;
        s >> VARINT(outpoint->n);
    }
};

struct StakingSendEntry {
    COutPoint* outpoint;
    CAccountID* accountID;
    char key;
    StakingSendEntry(const COutPoint* outpointIn, const CAccountID* accountIDIn) :
        outpoint(const_cast<COutPoint*>(outpointIn)),
        accountID(const_cast<CAccountID*>(accountIDIn)),
        key(DB_COIN_STAKING_SEND) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << key;
        s << *accountID;
        s << outpoint->hash;
        s << VARINT(outpoint->n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> key;
        s >> *accountID;
        s >> outpoint->hash;
        s >> VARINT(outpoint->n);
    }
};

struct StakingReceiveEntry {
    COutPoint* outpoint;
    CAccountID* accountID;
    char key;
    StakingReceiveEntry(const COutPoint* outpointIn, const CAccountID* accountIDIn) :
        outpoint(const_cast<COutPoint*>(outpointIn)),
        accountID(const_cast<CAccountID*>(accountIDIn)),
        key(DB_COIN_STAKING_RECEIVE) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << key;
        s << *accountID;
        s << outpoint->hash;
        s << VARINT(outpoint->n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> key;
        s >> *accountID;
        s >> outpoint->hash;
        s >> VARINT(outpoint->n);
    }
};

struct AccountEntry {
    char key;
    COutPoint* outpoint;
    CAccountID* accountID;
    AccountEntry(char keyIn, const COutPoint* outpointIn, const CAccountID* accountIDIn) :
        key(keyIn),
        outpoint(const_cast<COutPoint*>(outpointIn)),
        accountID(const_cast<CAccountID*>(accountIDIn)) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << key;
        s << *accountID;
        s << outpoint->hash;
        s << VARINT(outpoint->n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> key;
        s >> *accountID;
        s >> outpoint->hash;
        s >> VARINT(outpoint->n);
    }
};

template <char DBPrefix>
class CAccountCoinsViewDBCursor : public CCoinsViewCursor
{
public:
    CAccountCoinsViewDBCursor(const CAccountID& accountIDIn, const CCoinsViewDB* pcoinviewdbIn, CDBIterator* pcursorIn, const uint256& hashBlockIn)
            : CCoinsViewCursor(hashBlockIn), accountID(accountIDIn), pcoinviewdb(pcoinviewdbIn), pcursor(pcursorIn), outpoint(uint256(), 0) {
        // Seek cursor
        pcursor->Seek(AccountEntry(DBPrefix, &outpoint, &accountID));
        TestKey();
    }

    bool GetKey(COutPoint &key) const override {
        // Return cached key
        if (!outpoint.IsNull()) {
            key = outpoint;
            return true;
        }
        return false;
    }

    bool GetValue(Coin &coin) const override { return pcoinviewdb->GetCoin(outpoint, coin); }
    unsigned int GetValueSize() const override { return pcursor->GetValueSize(); }

    bool Valid() const override { return !outpoint.IsNull(); }
    void Next() override {
        pcursor->Next();
        TestKey();
    }

private:
    void TestKey() {
        CAccountID tempAccountID;
        AccountEntry entry(DBPrefix, &outpoint, &tempAccountID);
        if (!pcursor->Valid() || !pcursor->GetKey(entry) || entry.key != DBPrefix || tempAccountID != accountID) {
            outpoint.SetNull();
        }
    }

    const CAccountID accountID;
    const CCoinsViewDB* pcoinviewdb;
    std::unique_ptr<CDBIterator> pcursor;
    COutPoint outpoint;
};

class CAccountIDHasher
{
public:
    size_t operator()(const CAccountID& id) const noexcept {
        return (size_t) id.GetUint64(0);
    }
};

class COutPointHasher
{
public:
    size_t operator()(const COutPoint& outpoint) const noexcept {
        uint64_t d = outpoint.hash.GetUint64(0);
        d ^= outpoint.n;
        return (size_t) d;
    }
};

}

CCoinsViewDB::CCoinsViewDB(fs::path ldb_path, size_t nCacheSize, bool fMemory, bool fWipe) : db(ldb_path, nCacheSize, fMemory, fWipe, true)
{
}

bool CCoinsViewDB::GetCoin(const COutPoint &outpoint, Coin &coin) const {
    return db.Read(CoinEntry(&outpoint), coin);
}

bool CCoinsViewDB::HaveCoin(const COutPoint &outpoint) const {
    return db.Exists(CoinEntry(&outpoint));
}

uint256 CCoinsViewDB::GetBestBlock() const {
    uint256 hashBestChain;
    if (!db.Read(DB_BEST_BLOCK, hashBestChain))
        return uint256();
    return hashBestChain;
}

std::vector<uint256> CCoinsViewDB::GetHeadBlocks() const {
    std::vector<uint256> vhashHeadBlocks;
    if (!db.Read(DB_HEAD_BLOCKS, vhashHeadBlocks)) {
        return std::vector<uint256>();
    }
    return vhashHeadBlocks;
}

bool CCoinsViewDB::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) {
    CDBBatch batch(db);
    size_t count = 0;
    size_t changed = 0;
    size_t batch_size = (size_t)gArgs.GetArg("-dbbatchsize", nDefaultDbBatchSize);
    int crash_simulate = gArgs.GetArg("-dbcrashratio", 0);
    assert(!hashBlock.IsNull());

    uint256 old_tip = GetBestBlock();
    if (old_tip.IsNull()) {
        // We may be in the middle of replaying.
        std::vector<uint256> old_heads = GetHeadBlocks();
        if (old_heads.size() == 2) {
            assert(old_heads[0] == hashBlock);
            old_tip = old_heads[1];
        }
    }

    // In the first batch, mark the database as being in the middle of a
    // transition from old_tip to hashBlock.
    // A vector is used for future extensibility, as we may want to support
    // interrupting after partial writes from multiple independent reorgs.
    batch.Erase(DB_BEST_BLOCK);
    batch.Write(DB_HEAD_BLOCKS, std::vector<uint256>{hashBlock, old_tip});

    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) {
            CoinEntry entry(&it->first);
            if (it->second.coin.IsSpent()) {
                batch.Erase(entry);

                // payload
                if (!it->second.coin.refOutAccountID.IsNull()) {
                    batch.Erase(CoinIndexEntry(&it->first, &it->second.coin.refOutAccountID));
                    if (it->second.coin.IsBindPlotter()) {
                        batch.Erase(BindPlotterEntry(&it->first, &it->second.coin.refOutAccountID));
                    } else if (it->second.coin.IsPoint()) {
                        auto payload = PointPayload::As(it->second.coin.payload);
                        batch.Erase(PointSendEntry(&it->first, &it->second.coin.refOutAccountID));
                        batch.Erase(PointReceiveEntry(&it->first, &payload->GetReceiverID()));
                    } else if (it->second.coin.IsStaking()) {
                        auto payload = StakingPayload::As(it->second.coin.payload);
                        batch.Erase(StakingSendEntry(&it->first, &it->second.coin.refOutAccountID));
                        batch.Erase(StakingReceiveEntry(&it->first, &payload->GetReceiverID()));
                    }
                }
            } else {
                batch.Write(entry, it->second.coin);

                // payload
                if (!it->second.coin.refOutAccountID.IsNull()) {
                    batch.Write(CoinIndexEntry(&it->first, &it->second.coin.refOutAccountID), VARINT(it->second.coin.out.nValue, VarIntMode::NONNEGATIVE_SIGNED));
                    if (it->second.coin.IsBindPlotter()) {
                        auto payload = BindPlotterPayload::As(it->second.coin.payload);
                        uint32_t nHeight = it->second.coin.nHeight;
                        batch.Write(BindPlotterEntry(&it->first, &it->second.coin.refOutAccountID), BindPlotterValue(&payload->GetId(), &nHeight));
                    } else if (it->second.coin.IsPoint()) {
                        auto payload = PointPayload::As(it->second.coin.payload);
                        batch.Write(PointSendEntry(&it->first, &it->second.coin.refOutAccountID), VARINT(it->second.coin.out.nValue, VarIntMode::NONNEGATIVE_SIGNED));
                        batch.Write(PointReceiveEntry(&it->first, &payload->GetReceiverID()), VARINT(payload->GetAmount(), VarIntMode::NONNEGATIVE_SIGNED));
                    } else if (it->second.coin.IsStaking()) {
                        auto payload = StakingPayload::As(it->second.coin.payload);
                        batch.Write(StakingSendEntry(&it->first, &it->second.coin.refOutAccountID), VARINT(it->second.coin.out.nValue, VarIntMode::NONNEGATIVE_SIGNED));
                        batch.Write(StakingReceiveEntry(&it->first, &payload->GetReceiverID()), VARINT(payload->GetAmount(), VarIntMode::NONNEGATIVE_SIGNED));
                    }
                }
            }
            changed++;
        }

        count++;
        CCoinsMap::iterator itOld = it++;
        mapCoins.erase(itOld);
        if (batch.SizeEstimate() > batch_size) {
            LogPrint(BCLog::COINDB, "Writing partial batch of %.2f MiB\n", batch.SizeEstimate() * (1.0 / 1048576.0));
            db.WriteBatch(batch);
            batch.Clear();
            if (crash_simulate) {
                static FastRandomContext rng;
                if (rng.randrange(crash_simulate) == 0) {
                    LogPrintf("Simulating a crash. Goodbye.\n");
                    _Exit(0);
                }
            }
        }
    }

    // In the last batch, mark the database as consistent with hashBlock again.
    batch.Erase(DB_HEAD_BLOCKS);
    batch.Write(DB_BEST_BLOCK, hashBlock);

    LogPrint(BCLog::COINDB, "Writing final batch of %.2f MiB\n", batch.SizeEstimate() * (1.0 / 1048576.0));
    bool ret = db.WriteBatch(batch);
    LogPrint(BCLog::COINDB, "Committed %u changed transaction outputs (out of %u) to coin database...\n", (unsigned int)changed, (unsigned int)count);
    return ret;
}

CCoinsViewCursorRef CCoinsViewDB::Cursor() const {
    /** Specialization of CCoinsViewCursor to iterate over a CCoinsViewDB */
    class CCoinsViewDBCursor : public CCoinsViewCursor
    {
    public:
        CCoinsViewDBCursor(CDBIterator* pcursorIn, const uint256 &hashBlockIn) : CCoinsViewCursor(hashBlockIn), pcursor(pcursorIn) {
            /* It seems that there are no "const iterators" for LevelDB.  Since we
               only need read operations on it, use a const-cast to get around
               that restriction.  */
            pcursor->Seek(DB_COIN);
            // Cache key of first record
            if (pcursor->Valid()) {
                CoinEntry entry(&keyTmp.second);
                pcursor->GetKey(entry);
                keyTmp.first = entry.key;
            }
            else {
                keyTmp.first = 0; // Make sure Valid() and GetKey() return false
            }
        }

        bool GetKey(COutPoint &key) const override {
            // Return cached key
            if (keyTmp.first == DB_COIN) {
                key = keyTmp.second;
                return true;
            }
            return false;
        }

        bool GetValue(Coin &coin) const override { return pcursor->GetValue(coin); }
        unsigned int GetValueSize() const override { return pcursor->GetValueSize(); }

        bool Valid() const override { return keyTmp.first == DB_COIN; }
        void Next() override {
            pcursor->Next();
            CoinEntry entry(&keyTmp.second);
            if (!pcursor->Valid() || !pcursor->GetKey(entry)) {
                keyTmp.first = 0; // Invalidate cached key after last record so that Valid() and GetKey() return false
            }
            else {
                keyTmp.first = entry.key;
            }
        }

    private:
        std::unique_ptr<CDBIterator> pcursor;
        std::pair<char, COutPoint> keyTmp;
    };

    return std::make_shared<CCoinsViewDBCursor>(db.NewIterator(), GetBestBlock());
}

CCoinsViewCursorRef CCoinsViewDB::Cursor(const CAccountID &accountID) const {
    return std::make_shared< CAccountCoinsViewDBCursor<DB_COIN_INDEX> >(accountID, this, db.NewIterator(), GetBestBlock());
}

CCoinsViewCursorRef CCoinsViewDB::PointSendCursor(const CAccountID &accountID) const {
    return std::make_shared< CAccountCoinsViewDBCursor<DB_COIN_POINT_SEND> >(accountID, this, db.NewIterator(), GetBestBlock());
}

CCoinsViewCursorRef CCoinsViewDB::PointReceiveCursor(const CAccountID &accountID) const {
    return std::make_shared< CAccountCoinsViewDBCursor<DB_COIN_POINT_RECEIVE> >(accountID, this, db.NewIterator(), GetBestBlock());
}

CCoinsViewCursorRef CCoinsViewDB::StakingSendCursor(const CAccountID &accountID) const {
    return std::make_shared< CAccountCoinsViewDBCursor<DB_COIN_STAKING_SEND> >(accountID, this, db.NewIterator(), GetBestBlock());
}

CCoinsViewCursorRef CCoinsViewDB::StakingReceiveCursor(const CAccountID &accountID) const {
    return std::make_shared< CAccountCoinsViewDBCursor<DB_COIN_STAKING_RECEIVE> >(accountID, this, db.NewIterator(), GetBestBlock());
}

size_t CCoinsViewDB::EstimateSize() const
{
    return db.EstimateSize(DB_COIN, (char)(DB_COIN+1));
}

CAmount CCoinsViewDB::GetAccountBalance(const CAccountID &accountID, CAmount *balanceBindPlotter, CAmount balancePoint[2], CAmount balanceStaking[2], const CCoinsMap &mapModifiedCoins) const {
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    // Balance
    CAmount availableBalance = 0;
    {
        // Read from database
        CAmount tempAmount = 0;
        COutPoint tempOutpoint(uint256(), 0);
        CAccountID tempAccountID = accountID;
        CoinIndexEntry entry(&tempOutpoint, &tempAccountID);
        for (pcursor->Seek(entry); pcursor->Valid();  pcursor->Next()) {
            if (pcursor->GetKey(entry) && entry.key == DB_COIN_INDEX && tempAccountID == accountID) {
                if (!pcursor->GetValue(REF(VARINT(tempAmount, VarIntMode::NONNEGATIVE_SIGNED))))
                    throw std::runtime_error("Database read error");
                availableBalance += tempAmount;
            } else {
                break;
            }
        }

        // Apply modified coin
        for (CCoinsMap::const_iterator it = mapModifiedCoins.cbegin(); it != mapModifiedCoins.cend(); it++) {
            if (!(it->second.flags & CCoinsCacheEntry::DIRTY))
                continue;

            if (it->second.coin.refOutAccountID == accountID) {
                if (it->second.coin.IsSpent()) {
                    if (db.Exists(CoinIndexEntry(&it->first, &it->second.coin.refOutAccountID)))
                        availableBalance -= it->second.coin.out.nValue;
                } else {
                    if (!db.Exists(CoinIndexEntry(&it->first, &it->second.coin.refOutAccountID)))
                        availableBalance += it->second.coin.out.nValue;
                }
            }
        }
        assert(availableBalance >= 0);
    }

    // Balance of bind plotter
    if (balanceBindPlotter != nullptr) {
        *balanceBindPlotter = 0;

        // Read from database
        std::unordered_set<COutPoint, COutPointHasher> selected;
        CAccountID tempAccountID = accountID;
        COutPoint tempOutpoint(uint256(), 0);
        uint64_t tempPlotterId = 0;
        uint32_t tempHeight = 0;
        BindPlotterEntry entry(&tempOutpoint, &tempAccountID);
        BindPlotterValue value(&tempPlotterId, &tempHeight);
        for (pcursor->Seek(entry); pcursor->Valid();  pcursor->Next()) {
            if (pcursor->GetKey(entry) && entry.key == DB_COIN_BINDPLOTTER && tempAccountID == accountID) {
                if (!pcursor->GetValue(value))
                    throw std::runtime_error("Database read error");
                *balanceBindPlotter += PROTOCOL_BINDPLOTTER_LOCKAMOUNT;
                selected.insert(tempOutpoint);
            } else {
                break;
            }
        }

        // Apply modified coin
        for (CCoinsMap::const_iterator it = mapModifiedCoins.cbegin(); it != mapModifiedCoins.cend(); it++) {
            if (!(it->second.flags & CCoinsCacheEntry::DIRTY) || !it->second.coin.IsBindPlotter())
                continue;

            if (selected.count(it->first)) {
                if (it->second.coin.IsSpent()) {
                    *balanceBindPlotter -= PROTOCOL_BINDPLOTTER_LOCKAMOUNT;
                }
            } else if (it->second.coin.refOutAccountID == accountID && !it->second.coin.IsSpent()) {
                *balanceBindPlotter += PROTOCOL_BINDPLOTTER_LOCKAMOUNT;
            }
        }

        assert(*balanceBindPlotter >= 0);
    }

    // Balance of point
    if (balancePoint != nullptr) {
        // send
        if (balancePoint[0] != -1) {
            balancePoint[0] = 0;

            // Read from database
            std::unordered_map<COutPoint,CAmount,COutPointHasher> selected;
            CAccountID tempAccountID = accountID;
            COutPoint tempOutpoint(uint256(), 0);
            Coin coin;
            PointSendEntry entry(&tempOutpoint, &tempAccountID);
            for (pcursor->Seek(entry); pcursor->Valid();  pcursor->Next()) {
                if (pcursor->GetKey(entry) && entry.key == DB_COIN_POINT_SEND && tempAccountID == accountID) {
                    if (!db.Read(CoinEntry(entry.outpoint), coin))
                        throw std::runtime_error("Database read error");
                    balancePoint[0] += coin.out.nValue;
                    selected[tempOutpoint] = coin.out.nValue;
                } else {
                    break;
                }
            }

            // Apply modified coin
            for (CCoinsMap::const_iterator it = mapModifiedCoins.cbegin(); it != mapModifiedCoins.cend(); it++) {
                if (!(it->second.flags & CCoinsCacheEntry::DIRTY) || !it->second.coin.IsPoint())
                    continue;

                auto itSelected = selected.find(it->first);
                if (itSelected != selected.cend()) {
                    if (it->second.coin.IsSpent()) {
                        balancePoint[0] -= itSelected->second;
                    }
                } else if (it->second.coin.refOutAccountID == accountID && !it->second.coin.IsSpent()) {
                    balancePoint[0] += it->second.coin.out.nValue;
                }
            }

            assert(balancePoint[0] >= 0);
        }

        // receive
        if (balancePoint[1] != -1) {
            balancePoint[1] = 0;

            // Read from database
            std::unordered_map<COutPoint,CAmount,COutPointHasher> selected;
            CAccountID tempAccountID = accountID;
            COutPoint tempOutpoint(uint256(), 0);
            PointReceiveEntry entry(&tempOutpoint, &tempAccountID);
            for (pcursor->Seek(entry); pcursor->Valid();  pcursor->Next()) {
                if (pcursor->GetKey(entry) && entry.key == DB_COIN_POINT_RECEIVE && tempAccountID == accountID) {
                    CAmount value = 0;
                    if (!pcursor->GetValue(REF(VARINT(value, VarIntMode::NONNEGATIVE_SIGNED))))
                        throw std::runtime_error("Database read error");
                    balancePoint[1] += value;
                    selected[tempOutpoint] = value;
                } else {
                    break;
                }
            }

            // Apply modified coin
            for (CCoinsMap::const_iterator it = mapModifiedCoins.cbegin(); it != mapModifiedCoins.cend(); it++) {
                if (!(it->second.flags & CCoinsCacheEntry::DIRTY) || !it->second.coin.IsPoint())
                    continue;

                auto payload = PointPayload::As(it->second.coin.payload);
                auto itSelected = selected.find(it->first);
                if (itSelected != selected.cend()) {
                    if (it->second.coin.IsSpent()) {
                        balancePoint[1] -= itSelected->second;
                    }
                } else if (payload->GetReceiverID() == accountID && !it->second.coin.IsSpent()) {
                    balancePoint[1] += payload->GetAmount();
                }
            }

            assert(balancePoint[1] >= 0);
        }
    }

    // Balance of staking
    if (balanceStaking != nullptr) {
        // send
        if (balanceStaking[0] != -1) {
            balanceStaking[0] = 0;

            // Read from database
            std::unordered_map<COutPoint,CAmount,COutPointHasher> selected;
            CAccountID tempAccountID = accountID;
            COutPoint tempOutpoint(uint256(), 0);
            Coin coin;
            StakingSendEntry entry(&tempOutpoint, &tempAccountID);
            for (pcursor->Seek(entry); pcursor->Valid();  pcursor->Next()) {
                if (pcursor->GetKey(entry) && entry.key == DB_COIN_STAKING_SEND && tempAccountID == accountID) {
                    if (!db.Read(CoinEntry(entry.outpoint), coin))
                        throw std::runtime_error("Database read error");
                    balanceStaking[0] += coin.out.nValue;
                    selected[tempOutpoint] = coin.out.nValue;
                } else {
                    break;
                }
            }

            // Apply modified coin
            for (CCoinsMap::const_iterator it = mapModifiedCoins.cbegin(); it != mapModifiedCoins.cend(); it++) {
                if (!(it->second.flags & CCoinsCacheEntry::DIRTY) || !it->second.coin.IsStaking())
                    continue;

                auto itSelected = selected.find(it->first);
                if (itSelected != selected.cend()) {
                    if (it->second.coin.IsSpent()) {
                        balanceStaking[0] -= itSelected->second;
                    }
                } else if (it->second.coin.refOutAccountID == accountID && !it->second.coin.IsSpent()) {
                    balanceStaking[0] += it->second.coin.out.nValue;
                }
            }

            assert(balanceStaking[0] >= 0);
        }

        // receive
        if (balanceStaking[1] != -1) {
            balanceStaking[1] = 0;

            // Read from database
            std::unordered_map<COutPoint,CAmount,COutPointHasher> selected;
            CAccountID tempAccountID = accountID;
            COutPoint tempOutpoint(uint256(), 0);
            StakingReceiveEntry entry(&tempOutpoint, &tempAccountID);
            for (pcursor->Seek(entry); pcursor->Valid();  pcursor->Next()) {
                if (pcursor->GetKey(entry) && entry.key == DB_COIN_STAKING_RECEIVE && tempAccountID == accountID) {
                    CAmount value = 0;
                    if (!pcursor->GetValue(REF(VARINT(value, VarIntMode::NONNEGATIVE_SIGNED))))
                        throw std::runtime_error("Database read error");
                    balanceStaking[1] += value;
                    selected[tempOutpoint] = value;
                } else {
                    break;
                }
            }

            // Apply modified coin
            for (CCoinsMap::const_iterator it = mapModifiedCoins.cbegin(); it != mapModifiedCoins.cend(); it++) {
                if (!(it->second.flags & CCoinsCacheEntry::DIRTY) || !it->second.coin.IsStaking())
                    continue;

                auto payload = StakingPayload::As(it->second.coin.payload);
                auto itSelected = selected.find(it->first);
                if (itSelected != selected.cend()) {
                    if (it->second.coin.IsSpent()) {
                        balanceStaking[1] -= itSelected->second;
                    }
                } else if (payload->GetReceiverID() == accountID && !it->second.coin.IsSpent()) {
                    balanceStaking[1] += payload->GetAmount();
                }
            }

            assert(balanceStaking[1] >= 0);
        }
    }


    return availableBalance;
}

CBindPlotterCoinsMap CCoinsViewDB::GetAccountBindPlotterEntries(const CAccountID &accountID, const uint64_t &plotterId) const {
    CBindPlotterCoinsMap outpoints;

    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());
    COutPoint tempOutpoint(uint256(), 0);
    CAccountID tempAccountID = accountID;
    uint64_t tempPlotterId = 0;
    uint32_t tempHeight = 0;
    BindPlotterEntry entry(&tempOutpoint, &tempAccountID);
    BindPlotterValue value(&tempPlotterId, &tempHeight);
    for (pcursor->Seek(entry); pcursor->Valid();  pcursor->Next()) {
        if (pcursor->GetKey(entry) && entry.key == DB_COIN_BINDPLOTTER && tempAccountID == accountID) {
            if (!pcursor->GetValue(value))
                throw std::runtime_error("Database read error");
            if (plotterId == 0 || tempPlotterId == plotterId) {
                CBindPlotterCoinInfo &info = outpoints[tempOutpoint];
                info.nHeight = static_cast<int>(tempHeight);
                info.accountID = tempAccountID;
                info.plotterId = tempPlotterId;
            }
        } else {
            break;
        }
    }

    return outpoints;
}

CBindPlotterCoinsMap CCoinsViewDB::GetBindPlotterEntries(const uint64_t &plotterId) const {
    CBindPlotterCoinsMap outpoints;

    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());
    COutPoint tempOutpoint(uint256(), 0);
    CAccountID tempAccountID;
    uint64_t tempPlotterId = 0;
    uint32_t tempHeight = 0;
    BindPlotterEntry entry(&tempOutpoint, &tempAccountID);
    BindPlotterValue value(&tempPlotterId, &tempHeight);
    for (pcursor->Seek(entry); pcursor->Valid(); pcursor->Next()) {
        if (pcursor->GetKey(entry) && entry.key == DB_COIN_BINDPLOTTER) {
            if (!pcursor->GetValue(value))
                throw std::runtime_error("Database read error");
            if (tempPlotterId == plotterId) {
                CBindPlotterCoinInfo &info = outpoints[tempOutpoint];
                info.nHeight = static_cast<int>(tempHeight);
                info.accountID = tempAccountID;
                info.plotterId = tempPlotterId;
            }
        } else {
            break;
        }
    }

    return outpoints;
}

CAccountBalanceList CCoinsViewDB::GetTopStakingAccounts(int n, const CCoinsMap &mapModifiedCoins) const {
    assert(n > 0);
    std::unordered_map<CAccountID, CAmount, CAccountIDHasher> allStakings;
    
    // all
    {
        // Read from database
        std::unordered_map<COutPoint,CAmount,COutPointHasher> selected;
        CAccountID tempAccountID;
        COutPoint tempOutpoint(uint256(), 0);
        StakingReceiveEntry entry(&tempOutpoint, &tempAccountID);
        std::unique_ptr<CDBIterator> pcursor(db.NewIterator());
        for (pcursor->Seek(entry); pcursor->Valid();  pcursor->Next()) {
            if (pcursor->GetKey(entry) && entry.key == DB_COIN_STAKING_RECEIVE) {
                CAmount value = 0;
                if (!pcursor->GetValue(REF(VARINT(value, VarIntMode::NONNEGATIVE_SIGNED))))
                    throw std::runtime_error("Database read error");
                allStakings[tempAccountID] += value;
                selected[tempOutpoint] = value;
            } else {
                break;
            }
        }

        // Apply modified coin
        for (CCoinsMap::const_iterator it = mapModifiedCoins.cbegin(); it != mapModifiedCoins.cend(); it++) {
            if (!(it->second.flags & CCoinsCacheEntry::DIRTY) || !it->second.coin.IsStaking())
                continue;

            auto payload = StakingPayload::As(it->second.coin.payload);
            auto itSelected = selected.find(it->first);
            if (itSelected != selected.cend()) {
                if (it->second.coin.IsSpent()) {
                    auto it2 = allStakings.find(payload->GetReceiverID());
                    assert(it2 != allStakings.end());
                    it2->second -= itSelected->second;
                    assert(it2->second >= 0);
                    if (it2->second == 0)
                        allStakings.erase(it2);
                }
            } else if (!it->second.coin.IsSpent()) {
                allStakings[payload->GetReceiverID()] += payload->GetAmount();
            }
        }
    }

    // sort
    CAccountBalanceList topN(std::min((size_t) n, allStakings.size()));
    std::partial_sort_copy(allStakings.begin(), allStakings.end(), topN.begin(), topN.end(),
        [](const CAccountBalance &l, const CAccountBalance &r) {
            if (l.second == r.second)
                return l.first < r.first;
            return l.second > r.second;
        });
    return topN;
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(gArgs.IsArgSet("-blocksdir") ? GetDataDir() / "blocks" / "index" : GetBlocksDir() / "index", nCacheSize, fMemory, fWipe) {
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(std::make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return Write(DB_REINDEX_FLAG, '1');
    else
        return Erase(DB_REINDEX_FLAG);
}

void CBlockTreeDB::ReadReindexing(bool &fReindexing) {
    fReindexing = Exists(DB_REINDEX_FLAG);
}

bool CBlockTreeDB::ReadLastBlockFile(int &nFile) {
    return Read(DB_LAST_BLOCK, nFile);
}

bool CBlockTreeDB::WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<int, const CBlockFileInfo*> >::const_iterator it=fileInfo.begin(); it != fileInfo.end(); it++) {
        batch.Write(std::make_pair(DB_BLOCK_FILES, it->first), *it->second);
    }
    batch.Write(DB_LAST_BLOCK, nLastFile);
    for (std::vector<const CBlockIndex*>::const_iterator it=blockinfo.begin(); it != blockinfo.end(); it++) {
        batch.Write(std::make_pair(DB_BLOCK_INDEX, (*it)->GetBlockHash()), CDiskBlockIndex(*it));
    }
    return WriteBatch(batch, true);
}

bool CBlockTreeDB::WriteFlag(const std::string &name, bool fValue) {
    return Write(std::make_pair(DB_FLAG, name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const std::string &name, bool &fValue) {
    char ch;
    if (!Read(std::make_pair(DB_FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CBlockTreeDB::LoadBlockIndexGuts(const Consensus::Params& consensusParams, std::function<CBlockIndex*(const uint256&)> insertBlockIndex)
{
    size_t batch_size = (size_t) gArgs.GetArg("-dbbatchsize", nDefaultDbBatchSize);
    CDBBatch batch(*this);

    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(DB_BLOCK_INDEX, uint256()));

    // Load m_block_index
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        if (ShutdownRequested()) return false;
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_BLOCK_INDEX) {
            CDiskBlockIndex diskindex;
            if (pcursor->GetValue(diskindex)) {
                // Construct block index object
                CBlockIndex* pindexNew = insertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev              = insertBlockIndex(diskindex.hashPrev);
                pindexNew->nHeight            = diskindex.nHeight;
                pindexNew->nFile              = diskindex.nFile;
                pindexNew->nDataPos           = diskindex.nDataPos;
                pindexNew->nUndoPos           = diskindex.nUndoPos;
                pindexNew->nVersion           = diskindex.nVersion;
                pindexNew->hashMerkleRoot     = diskindex.hashMerkleRoot;
                pindexNew->nTime              = diskindex.nTime;
                pindexNew->nBaseTarget        = diskindex.nBaseTarget;
                pindexNew->nNonce             = diskindex.nNonce;
                pindexNew->nPlotterId         = diskindex.nPlotterId;
                pindexNew->nStatus            = diskindex.nStatus;
                pindexNew->nTx                = diskindex.nTx;
                pindexNew->minerRewardTxOut   = diskindex.minerRewardTxOut;
                pindexNew->vchPubKey          = diskindex.vchPubKey;
                pindexNew->vchSignature       = diskindex.vchSignature;
                pcursor->Next();
            } else {
                return error("%s: failed to read value", __func__);
            }
        } else {
            break;
        }
    }

    return WriteBatch(batch);
}

/** Upgrade the database from older formats */
bool CCoinsViewDB::Upgrade(bool &fUpgraded) {
    fUpgraded = false;
    return true;
}
