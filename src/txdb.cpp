// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txdb.h>

#include <chainparams.h>
#include <fs.h>
#include <hash.h>
#include <init.h>
#include <memusage.h>
#include <random.h>
#include <uint256.h>
#include <util.h>
#include <ui_interface.h>
#include <validation.h>

#include <exception>
#include <map>
#include <set>
#include <unordered_map>

#include <stdint.h>
#include <inttypes.h>

#include <boost/thread.hpp>

/** UTXO version flag */
static const char DB_COIN_VERSION = 'V';
static const uint32_t DB_VERSION = 0x11;

static const char DB_COIN = 'C';
static const char DB_BLOCK_FILES = 'f';
static const char DB_TXINDEX = 't';
static const char DB_BLOCK_INDEX = 'b';
static const char DB_BLOCK_GENERATOR_INDEX = 'g';

static const char DB_BEST_BLOCK = 'B';
static const char DB_HEAD_BLOCKS = 'H';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_LAST_BLOCK = 'l';

static const char DB_COIN_INDEX = 'T';
static const char DB_COIN_BINDPLOTTER = 'P';
static const char DB_COIN_RENTAL = 'E';
static const char DB_COIN_BORROW = 'e'; //! DEPRECTED

namespace {

struct CoinEntry {
    COutPoint* outpoint;
    char key;
    explicit CoinEntry(const COutPoint* outputIn) :
        outpoint(const_cast<COutPoint*>(outputIn)),
        key(DB_COIN) {}

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
    bool *valid;
    BindPlotterValue(const uint64_t* plotterIdIn, const uint32_t* nHeightIn, const bool* validIn) :
        plotterId(const_cast<uint64_t*>(plotterIdIn)),
        nHeight(const_cast<uint32_t*>(nHeightIn)),
        valid(const_cast<bool*>(validIn)) {}

    template<typename Stream>
    void Serialize(Stream &s) const {
        s << VARINT(*plotterId);
        s << VARINT(*nHeight);
        s << *valid;
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        s >> VARINT(*plotterId);
        s >> VARINT(*nHeight);
        s >> *valid;
    }
};

struct PledgeEntry {
    COutPoint* outpoint;
    CAccountID* accountID;
    char key;
    PledgeEntry(const COutPoint* outpointIn, const CAccountID* accountIDIn) :
        outpoint(const_cast<COutPoint*>(outpointIn)),
        accountID(const_cast<CAccountID*>(accountIDIn)),
        key(DB_COIN_RENTAL) {}

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

}

CCoinsViewDB::CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe, true) { }

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
            if (it->second.coin.IsSpent()) {
                batch.Erase(CoinEntry(&it->first));
                if (!it->second.coin.refOutAccountID.IsNull())
                    batch.Erase(CoinIndexEntry(&it->first, &it->second.coin.refOutAccountID));
            } else {
                batch.Write(CoinEntry(&it->first), it->second.coin);
                if (!it->second.coin.refOutAccountID.IsNull())
                    batch.Write(CoinIndexEntry(&it->first, &it->second.coin.refOutAccountID), VARINT(it->second.coin.out.nValue));
            }
            changed++;

            // Extra indexes. ONLY FOR vout[0]
            if (it->first.n == 0 && !it->second.coin.refOutAccountID.IsNull()) {
                bool fTryEraseBind = true, fTryErasePledge = true;
                if (it->second.coin.IsSpent()) {
                    if (it->second.coin.IsBindPlotter() && (it->second.flags & CCoinsCacheEntry::UNBIND)) {
                        fTryEraseBind = false;

                        const uint64_t &plotterId = BindPlotterPayload::As(it->second.coin.extraData)->id;
                        uint32_t nHeight = it->second.coin.nHeight;
                        bool valid = false;
                        batch.Write(BindPlotterEntry(&it->first, &it->second.coin.refOutAccountID), BindPlotterValue(&plotterId, &nHeight, &valid));
                    }
                } else {
                    if (it->second.coin.IsBindPlotter()) {
                        fTryEraseBind = false;

                        const uint64_t &plotterId = BindPlotterPayload::As(it->second.coin.extraData)->id;
                        uint32_t nHeight = it->second.coin.nHeight;
                        bool valid = true;
                        batch.Write(BindPlotterEntry(&it->first, &it->second.coin.refOutAccountID), BindPlotterValue(&plotterId, &nHeight, &valid));
                    }
                    else if (it->second.coin.IsPledge()) {
                        fTryErasePledge = false;

                        batch.Write(PledgeEntry(&it->first, &it->second.coin.refOutAccountID), REF(RentalPayload::As(it->second.coin.extraData)->GetBorrowerAccountID()));
                    }
                }

                // Try erase coin
                if (fTryEraseBind) {
                    batch.Erase(BindPlotterEntry(&it->first, &it->second.coin.refOutAccountID));
                }
                if (fTryErasePledge) {
                    batch.Erase(PledgeEntry(&it->first, &it->second.coin.refOutAccountID));
                }
            }
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
    class CCoinsViewDBCursor : public CCoinsViewCursor
    {
    public:
        CCoinsViewDBCursor(const CAccountID& accountIDIn, const CCoinsViewDB* pcoinviewdbIn, CDBIterator* pcursorIn, const uint256& hashBlockIn)
                : CCoinsViewCursor(hashBlockIn), accountID(accountIDIn), pcoinviewdb(pcoinviewdbIn), pcursor(pcursorIn), outpoint(uint256(), 0) {
            // Seek cursor
            pcursor->Seek(CoinIndexEntry(&outpoint, &accountID));
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
            CoinIndexEntry entry(&outpoint, &tempAccountID);
            if (!pcursor->Valid() || !pcursor->GetKey(entry) || entry.key != DB_COIN_INDEX || tempAccountID != accountID) {
                outpoint.SetNull();
            }
        }

        const CAccountID accountID;
        const CCoinsViewDB* pcoinviewdb;
        std::unique_ptr<CDBIterator> pcursor;
        COutPoint outpoint;
    };

    return std::make_shared<CCoinsViewDBCursor>(accountID, this, db.NewIterator(), GetBestBlock());
}

CCoinsViewCursorRef CCoinsViewDB::RentalLoanCursor(const CAccountID &accountID) const {
    class CCoinsViewDBPledgeCreditCursor : public CCoinsViewCursor
    {
    public:
        CCoinsViewDBPledgeCreditCursor(const CAccountID& accountIDIn, const CCoinsViewDB* pcoinviewdbIn, CDBIterator* pcursorIn, const uint256& hashBlockIn)
                : CCoinsViewCursor(hashBlockIn), accountID(accountIDIn), pcoinviewdb(pcoinviewdbIn), pcursor(pcursorIn), outpoint(uint256(), 0) {
            // Seek cursor
            pcursor->Seek(PledgeEntry(&outpoint, &accountID));
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
            PledgeEntry entry(&outpoint, &tempAccountID);
            if (!pcursor->Valid() || !pcursor->GetKey(entry) || entry.key != DB_COIN_RENTAL || tempAccountID != accountID) {
                outpoint.SetNull();
            }
        }

        const CAccountID accountID;
        const CCoinsViewDB* pcoinviewdb;
        std::unique_ptr<CDBIterator> pcursor;
        COutPoint outpoint;
    };

    return std::make_shared<CCoinsViewDBPledgeCreditCursor>(accountID, this, db.NewIterator(), GetBestBlock());
}

CCoinsViewCursorRef CCoinsViewDB::RentalBorrowCursor(const CAccountID &accountID) const {
    class CCoinsViewDBPledgeDebitCursor : public CCoinsViewCursor
    {
    public:
        CCoinsViewDBPledgeDebitCursor(const CAccountID& accountIDIn, const CCoinsViewDB* pcoinviewdbIn, CDBIterator* pcursorIn, const uint256& hashBlockIn)
                : CCoinsViewCursor(hashBlockIn), accountID(accountIDIn), pcoinviewdb(pcoinviewdbIn), pcursor(pcursorIn), outpoint(uint256(), 0), loanAccountID() {
            // Seek cursor to first pledge coin
            pcursor->Seek(DB_COIN_RENTAL);
            GotoValidEntry();
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
            GotoValidEntry();
        }

    private:
        void GotoValidEntry() {
            CAccountID borrowerAccountID;
            PledgeEntry entry(&outpoint, &loanAccountID);
            while (true) {
                if (!pcursor->Valid() || !pcursor->GetKey(entry) || entry.key != DB_COIN_RENTAL || !pcursor->GetValue(borrowerAccountID)) {
                    outpoint.SetNull();
                    break;
                }
                if (borrowerAccountID == accountID)
                    break;
                pcursor->Next();
            }
        }

        const CAccountID accountID;
        const CCoinsViewDB* pcoinviewdb;
        std::unique_ptr<CDBIterator> pcursor;
        COutPoint outpoint;
        CAccountID loanAccountID;
    };

    return std::make_shared<CCoinsViewDBPledgeDebitCursor>(accountID, this, db.NewIterator(), GetBestBlock());
}

size_t CCoinsViewDB::EstimateSize() const {
    return db.EstimateSize(DB_COIN, (char)(DB_COIN+1));
}

CAmount CCoinsViewDB::GetBalance(const CAccountID &accountID, const CCoinsMap &mapChildCoins,
    CAmount *balanceBindPlotter, CAmount *balanceLoan, CAmount *balanceBorrow) const
{
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    // Balance
    CAmount availableBalance = 0;
    {
        CAmount tempAmount = 0;
        COutPoint tempOutpoint(uint256(), 0);
        CAccountID tempAccountID = accountID;
        CoinIndexEntry entry(&tempOutpoint, &tempAccountID);

        // Read from database
        pcursor->Seek(entry);
        while (pcursor->Valid()) {
            if (pcursor->GetKey(entry) && entry.key == DB_COIN_INDEX && *entry.accountID == accountID) {
                if (!pcursor->GetValue(VARINT(tempAmount)))
                    throw std::runtime_error("Database read error");
                availableBalance += tempAmount;
            } else {
                break;
            }
            pcursor->Next();
        }

        // Apply modified coin
        for (CCoinsMap::const_iterator it = mapChildCoins.cbegin(); it != mapChildCoins.cend(); it++) {
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
        std::set<COutPoint> selected;
        CAccountID tempAccountID = accountID;
        COutPoint tempOutpoint(uint256(), 0);
        uint64_t tempPlotterId = 0;
        uint32_t tempHeight = 0;
        bool tempValid = true;
        BindPlotterEntry entry(&tempOutpoint, &tempAccountID);
        BindPlotterValue value(&tempPlotterId, &tempHeight, &tempValid);
        pcursor->Seek(entry);
        while (pcursor->Valid()) {
            if (pcursor->GetKey(entry) && entry.key == DB_COIN_BINDPLOTTER && *entry.accountID == accountID) {
                if (!pcursor->GetValue(value))
                    throw std::runtime_error("Database read error");
                if (*value.valid) {
                    *balanceBindPlotter += PROTOCOL_BINDPLOTTER_LOCKAMOUNT;
                    selected.insert(*entry.outpoint);
                }
            } else {
                break;
            }
            pcursor->Next();
        }

        // Apply modified coin
        for (CCoinsMap::const_iterator it = mapChildCoins.cbegin(); it != mapChildCoins.cend(); it++) {
            if (!(it->second.flags & CCoinsCacheEntry::DIRTY))
                continue;

            if (selected.count(it->first)) {
                if (it->second.coin.IsSpent()) {
                    *balanceBindPlotter -= PROTOCOL_BINDPLOTTER_LOCKAMOUNT;
                }
            } else if (it->second.coin.refOutAccountID == accountID && it->second.coin.IsBindPlotter() && !it->second.coin.IsSpent()) {
                *balanceBindPlotter += PROTOCOL_BINDPLOTTER_LOCKAMOUNT;
            }
        }

        assert(*balanceBindPlotter >= 0);
    }

    // Balance of pledge ->
    if (balanceLoan != nullptr) {
        *balanceLoan = 0;

        // Read from database
        std::map<COutPoint, CAmount> selected;
        CAccountID tempAccountID = accountID;
        COutPoint tempOutpoint(uint256(), 0);
        Coin coin;
        PledgeEntry entry(&tempOutpoint, &tempAccountID);
        pcursor->Seek(entry);
        while (pcursor->Valid()) {
            if (pcursor->GetKey(entry) && entry.key == DB_COIN_RENTAL && *entry.accountID == accountID) {
                if (!db.Read(CoinEntry(entry.outpoint), coin))
                    throw std::runtime_error("Database read error");
                *balanceLoan += coin.out.nValue;
                selected[*entry.outpoint] = coin.out.nValue;
            } else {
                break;
            }
            pcursor->Next();
        }

        // Apply modified coin
        for (CCoinsMap::const_iterator it = mapChildCoins.cbegin(); it != mapChildCoins.cend(); it++) {
            if (!(it->second.flags & CCoinsCacheEntry::DIRTY))
                continue;

            auto itSelected = selected.find(it->first);
            if (itSelected != selected.cend()) {
                if (it->second.coin.IsSpent()) {
                    *balanceLoan -= itSelected->second;
                }
            } else if (it->second.coin.refOutAccountID == accountID && it->second.coin.IsPledge() && !it->second.coin.IsSpent()) {
                *balanceLoan += it->second.coin.out.nValue;
            }
        }

        assert(*balanceLoan >= 0);
    }

    // Balance of pledge <-
    if (balanceBorrow != nullptr) {
        *balanceBorrow = 0;

        // Read from database
        std::map<COutPoint, CAmount> selected;
        CAccountID tempDebitAccountID;
        CAccountID tempAccountID;
        COutPoint tempOutpoint(uint256(), 0);
        Coin coin;
        PledgeEntry entry(&tempOutpoint, &tempAccountID);
        pcursor->Seek(entry);
        while (pcursor->Valid()) {
            if (pcursor->GetKey(entry) && entry.key == DB_COIN_RENTAL) {
                if (!pcursor->GetValue(tempDebitAccountID))
                    throw std::runtime_error("Database read error");
                if (tempDebitAccountID == accountID) {
                    if (!db.Read(CoinEntry(entry.outpoint), coin))
                        throw std::runtime_error("Database read error");
                    *balanceBorrow += coin.out.nValue;
                    selected[*entry.outpoint] = coin.out.nValue;
                }
            } else {
                break;
            }
            pcursor->Next();
        }

        // Apply modified coin
        for (CCoinsMap::const_iterator it = mapChildCoins.cbegin(); it != mapChildCoins.cend(); it++) {
            if (!(it->second.flags & CCoinsCacheEntry::DIRTY))
                continue;

            auto itSelected = selected.find(it->first);
            if (itSelected != selected.cend()) {
                if (it->second.coin.IsSpent()) {
                    *balanceBorrow -= itSelected->second;
                }
            } else if (it->second.coin.IsPledge() && RentalPayload::As(it->second.coin.extraData)->GetBorrowerAccountID() == accountID && !it->second.coin.IsSpent()) {
                *balanceBorrow += it->second.coin.out.nValue;
            }
        }

        assert(*balanceBorrow >= 0);
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
    bool tempValid = 0;
    BindPlotterEntry entry(&tempOutpoint, &tempAccountID);
    BindPlotterValue value(&tempPlotterId, &tempHeight, &tempValid);
    pcursor->Seek(entry);
    while (pcursor->Valid()) {
        if (pcursor->GetKey(entry) && entry.key == DB_COIN_BINDPLOTTER && *entry.accountID == accountID) {
            if (!pcursor->GetValue(value))
                throw std::runtime_error("Database read error");
            if (plotterId == 0 || *value.plotterId == plotterId) {
                CBindPlotterCoinInfo &info = outpoints[*entry.outpoint];
                info.nHeight = static_cast<int>(*value.nHeight);
                info.accountID = *entry.accountID;
                info.plotterId = *value.plotterId;
                info.valid = *value.valid;
            }
        } else {
            break;
        }
        pcursor->Next();
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
    bool tempValid = 0;
    BindPlotterEntry entry(&tempOutpoint, &tempAccountID);
    BindPlotterValue value(&tempPlotterId, &tempHeight, &tempValid);
    pcursor->Seek(entry);
    while (pcursor->Valid()) {
        if (pcursor->GetKey(entry) && entry.key == DB_COIN_BINDPLOTTER) {
            if (!pcursor->GetValue(value))
                throw std::runtime_error("Database read error");
            if (*value.plotterId == plotterId) {
                CBindPlotterCoinInfo &info = outpoints[*entry.outpoint];
                info.nHeight = static_cast<int>(*value.nHeight);
                info.accountID = *entry.accountID;
                info.plotterId = *value.plotterId;
                info.valid = *value.valid;
            }
        } else {
            break;
        }
        pcursor->Next();
    }

    return outpoints;
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe) { }

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(std::make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return Write(DB_REINDEX_FLAG, '1');
    else
        return Erase(DB_REINDEX_FLAG);
}

bool CBlockTreeDB::ReadReindexing(bool &fReindexing) {
    fReindexing = Exists(DB_REINDEX_FLAG);
    return true;
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
        if ((*it)->vchPubKey.empty() && !(*it)->generatorAccountID.IsNull())
            batch.Write(std::make_pair(DB_BLOCK_GENERATOR_INDEX, (*it)->GetBlockHash()), REF((*it)->generatorAccountID));
    }
    return WriteBatch(batch, true);
}

bool CBlockTreeDB::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return Read(std::make_pair(DB_TXINDEX, txid), pos);
}

bool CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >&vect) {
    CDBBatch batch(*this);
    for (std::vector<std::pair<uint256,CDiskTxPos> >::const_iterator it=vect.begin(); it!=vect.end(); it++)
        batch.Write(std::make_pair(DB_TXINDEX, it->first), it->second);
    return WriteBatch(batch);
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

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
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
                pindexNew->generatorAccountID = diskindex.generatorAccountID;
                pindexNew->vchPubKey          = diskindex.vchPubKey;
                pindexNew->vchSignature       = diskindex.vchSignature;

                // Load external generator
                if ((pindexNew->nStatus & BLOCK_HAVE_DATA) && pindexNew->vchPubKey.empty() && pindexNew->nHeight > 0) {
                    bool fRequireStore = false;
                    CAccountID generatorAccountID;
                    if (!Read(std::make_pair(DB_BLOCK_GENERATOR_INDEX, pindexNew->GetBlockHash()), REF(generatorAccountID))) {
                        //! Slowly: Read from full block data
                        CBlock block;
                        if (!ReadBlockFromDisk(block, pindexNew, consensusParams))
                            return error("%s: failed to read block value", __func__);
                        generatorAccountID = ExtractAccountID(block.vtx[0]->vout[0].scriptPubKey);
                        fRequireStore = !generatorAccountID.IsNull();
                    }
                    if (generatorAccountID.GetUint64(0) != pindexNew->generatorAccountID.GetUint64(0))
                        return error("%s: failed to read external generator value", __func__);
                    pindexNew->generatorAccountID = generatorAccountID;

                    if (fRequireStore) {
                        batch.Write(std::make_pair(DB_BLOCK_GENERATOR_INDEX, pindexNew->GetBlockHash()), REF(generatorAccountID));
                        if (batch.SizeEstimate() > batch_size) {
                            WriteBatch(batch);
                            batch.Clear();
                        }
                    }
                }

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
    // Check coin database version
    uint32_t coinDbVersion = 0;
    if (db.Read(DB_COIN_VERSION, VARINT(coinDbVersion)) && coinDbVersion == DB_VERSION)
        return true;
    db.Erase(DB_COIN_VERSION);
    fUpgraded = true;

    // Reindex UTXO for address
    uiInterface.ShowProgress(_("Upgrading UTXO database"), 0, true);
    LogPrintf("Upgrading UTXO database to %08x: [0%%]...", DB_VERSION);

    size_t batch_size = (size_t) gArgs.GetArg("-dbbatchsize", nDefaultDbBatchSize);
    int remove = 0, add = 0;
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    // Clear old data
    pcursor->SeekToFirst();
    if (pcursor->Valid()) {
        CDBBatch batch(db);
        for (; pcursor->Valid(); pcursor->Next()) {
            const leveldb::Slice key = pcursor->GetKey();
            if (key.size() > 32 && (key[0] == DB_COIN_INDEX || key[0] == DB_COIN_BINDPLOTTER || key[0] == DB_COIN_RENTAL || key[0] == DB_COIN_BORROW)) {
                batch.EraseSlice(key);
                remove++;

                if (batch.SizeEstimate() > batch_size) {
                    db.WriteBatch(batch);
                    batch.Clear();
                }
            }
        }
        db.WriteBatch(batch);
    }

    // Create coin index data
    pcursor->Seek(DB_COIN);
    if (pcursor->Valid()) {
        int utxo_bucket = 173000 / 100;
        int indexProgress = -1;
        CDBBatch batch(db);
        COutPoint outpoint;
        CoinEntry entry(&outpoint);
        for (; pcursor->Valid(); pcursor->Next()) {
            if (pcursor->GetKey(entry) && entry.key == DB_COIN) {
                Coin coin;
                if (!pcursor->GetValue(coin))
                    return error("%s: cannot parse coin record", __func__);

                if (!coin.refOutAccountID.IsNull()) {
                    // Coin index
                    batch.Write(CoinIndexEntry(&outpoint, &coin.refOutAccountID), VARINT(coin.out.nValue));
                    add++;

                    // Extra data
                    if (coin.IsBindPlotter()) {
                        const uint64_t &plotterId = BindPlotterPayload::As(coin.extraData)->id;
                        uint32_t nHeight = coin.nHeight;
                        bool valid = true;
                        batch.Write(BindPlotterEntry(&outpoint, &coin.refOutAccountID), BindPlotterValue(&plotterId, &nHeight, &valid));
                        add++;
                    }
                    else if (coin.IsPledge()) {
                        batch.Write(PledgeEntry(&outpoint, &coin.refOutAccountID), REF(RentalPayload::As(coin.extraData)->GetBorrowerAccountID()));
                        add++;
                    }

                    if (batch.SizeEstimate() > batch_size) {
                        db.WriteBatch(batch);
                        batch.Clear();
                    }

                    if (add % (utxo_bucket/10) == 0) {
                        int newProgress = std::min(90, add / utxo_bucket);
                        if (newProgress/10 != indexProgress/10) {
                            indexProgress = newProgress;
                            uiInterface.ShowProgress(_("Upgrading UTXO database"), indexProgress, true);
                            LogPrintf("[%d%%]...", indexProgress);
                        }
                    }
                }
            } else {
                break;
            }
        }
        db.WriteBatch(batch);
    }

    // Update coin version
    if (!db.Write(DB_COIN_VERSION, VARINT(DB_VERSION)))
        return error("%s: cannot write UTXO version", __func__);

    uiInterface.ShowProgress("", 100, false);
    LogPrintf("[%s]. remove utxo %d, add utxo %d\n", ShutdownRequested() ? "CANCELLED" : "DONE", remove, add);

    return !ShutdownRequested();
}