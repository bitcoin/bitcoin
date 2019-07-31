// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcointalkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txdb.h>

#include <random.h>
#include <pow.h>
#include <shutdown.h>
#include <uint256.h>
#include <util/system.h>
#include <ui_interface.h>

#include <stdint.h>

#include <boost/thread.hpp>

static const char DB_COIN = 'C';
static const char DB_COINS = 'c';
static const char DB_BLOCK_FILES = 'f';
static const char DB_BLOCK_INDEX = 'b';
#ifdef ENABLE_PROOF_OF_STAKE
//////////////////////////////////////////
static const char DB_HEIGHTINDEX = 'h';
static const char DB_STAKEINDEX = 's';
//////////////////////////////////////////
#endif
static const char DB_BEST_BLOCK = 'B';
static const char DB_HEAD_BLOCKS = 'H';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_LAST_BLOCK = 'l';

namespace {

struct CoinEntry {
    COutPoint* outpoint;
    char key;
    explicit CoinEntry(const COutPoint* ptr) : outpoint(const_cast<COutPoint*>(ptr)), key(DB_COIN)  {}

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

}

CCoinsViewDB::CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe, true)
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
            if (it->second.coin.IsSpent())
                batch.Erase(entry);
            else
                batch.Write(entry, it->second.coin);
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

size_t CCoinsViewDB::EstimateSize() const
{
    return db.EstimateSize(DB_COIN, (char)(DB_COIN+1));
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

CCoinsViewCursor *CCoinsViewDB::Cursor() const
{
    CCoinsViewDBCursor *i = new CCoinsViewDBCursor(const_cast<CDBWrapper&>(db).NewIterator(), GetBestBlock());
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    i->pcursor->Seek(DB_COIN);
    // Cache key of first record
    if (i->pcursor->Valid()) {
        CoinEntry entry(&i->keyTmp.second);
        i->pcursor->GetKey(entry);
        i->keyTmp.first = entry.key;
    } else {
        i->keyTmp.first = 0; // Make sure Valid() and GetKey() return false
    }
    return i;
}

bool CCoinsViewDBCursor::GetKey(COutPoint &key) const
{
    // Return cached key
    if (keyTmp.first == DB_COIN) {
        key = keyTmp.second;
        return true;
    }
    return false;
}

bool CCoinsViewDBCursor::GetValue(Coin &coin) const
{
    return pcursor->GetValue(coin);
}

unsigned int CCoinsViewDBCursor::GetValueSize() const
{
    return pcursor->GetValueSize();
}

bool CCoinsViewDBCursor::Valid() const
{
    return keyTmp.first == DB_COIN;
}

void CCoinsViewDBCursor::Next()
{
    pcursor->Next();
    CoinEntry entry(&keyTmp.second);
    if (!pcursor->Valid() || !pcursor->GetKey(entry)) {
        keyTmp.first = 0; // Invalidate cached key after last record so that Valid() and GetKey() return false
    } else {
        keyTmp.first = entry.key;
    }
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

#ifdef ENABLE_PROOF_OF_STAKE

bool CBlockTreeDB::WriteHeightIndex(const CHeightTxIndexKey &heightIndex, const std::vector<uint256>& hash) {
    CDBBatch batch(*this);
    batch.Write(std::make_pair(DB_HEIGHTINDEX, heightIndex), hash);
    return WriteBatch(batch);
}

int CBlockTreeDB::ReadHeightIndex(int low, int high, int minconf,
        std::vector<std::vector<uint256>> &blocksOfHashes,
        std::set<uint160> const &addresses) {

    if ((high < low && high > -1) || (high == 0 && low == 0) || (high < -1 || low < 0)) {
       return -1;
    }

    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(DB_HEIGHTINDEX, CHeightTxIndexIteratorKey(low)));

    int curheight = 0;

    for (size_t count = 0; pcursor->Valid(); pcursor->Next()) {

        std::pair<char, CHeightTxIndexKey> key;
        if (!pcursor->GetKey(key) || key.first != DB_HEIGHTINDEX) {
            break;
        }

        int nextHeight = key.second.height;

        if (high > -1 && nextHeight > high) {
            break;
        }

        if (minconf > 0) {
            int conf = ::ChainActive().Height() - nextHeight;
            if (conf < minconf) {
                break;
            }
        }

        curheight = nextHeight;

        auto address = key.second.address;
        if (!addresses.empty() && addresses.find(address) == addresses.end()) {
            continue;
        }

        std::vector<uint256> hashesTx;

        if (!pcursor->GetValue(hashesTx)) {
            break;
        }

        count += hashesTx.size();

        blocksOfHashes.push_back(hashesTx);
    }

    return curheight;
}

bool CBlockTreeDB::EraseHeightIndex(const unsigned int &height) {

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    CDBBatch batch(*this);

    pcursor->Seek(std::make_pair(DB_HEIGHTINDEX, CHeightTxIndexIteratorKey(height)));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CHeightTxIndexKey> key;
        if (pcursor->GetKey(key) && key.first == DB_HEIGHTINDEX && key.second.height == height) {
            batch.Erase(key);
            pcursor->Next();
        } else {
            break;
        }
    }

    return WriteBatch(batch);
}

bool CBlockTreeDB::WipeHeightIndex() {

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    CDBBatch batch(*this);

    pcursor->Seek(DB_HEIGHTINDEX);

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CHeightTxIndexKey> key;
        if (pcursor->GetKey(key) && key.first == DB_HEIGHTINDEX) {
            batch.Erase(key);
            pcursor->Next();
        } else {
            break;
        }
    }

    return WriteBatch(batch);
}


bool CBlockTreeDB::WriteStakeIndex(unsigned int height, uint160 address) {
    CDBBatch batch(*this);
    batch.Write(std::make_pair(DB_STAKEINDEX, height), address);
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadStakeIndex(unsigned int height, uint160& address){
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(DB_STAKEINDEX, height));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CHeightTxIndexKey> key;
        pcursor->GetKey(key); //note: it's apparently ok if this returns an error https://github.com/bitcointalkcoin/bitcointalkcoin/issues/7890
        if (key.first == DB_STAKEINDEX) {
            pcursor->GetValue(address);
            return true;
        }else{
            return false;
        }
    }
    return false;
}
bool CBlockTreeDB::ReadStakeIndex(unsigned int high, unsigned int low, std::vector<uint160> addresses){
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(DB_STAKEINDEX, low));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CHeightTxIndexKey> key;
        pcursor->GetKey(key); //note: it's apparently ok if this returns an error https://github.com/bitcointalkcoin/bitcointalkcoin/issues/7890
        if (key.first == DB_STAKEINDEX && key.second.height < high) {
            uint160 value;
            pcursor->GetValue(value);
            addresses.push_back(value);
            pcursor->Next();
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::EraseStakeIndex(unsigned int height) {

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    CDBBatch batch(*this);

    pcursor->Seek(std::make_pair(DB_STAKEINDEX, height));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, CHeightTxIndexKey> key;
        if (pcursor->GetKey(key) && key.first == DB_HEIGHTINDEX && key.second.height == height) {
            batch.Erase(key);
            pcursor->Next();
        } else {
            break;
        }
    }

    return WriteBatch(batch);
}
#endif

bool CBlockTreeDB::LoadBlockIndexGuts(const Consensus::Params& consensusParams, std::function<CBlockIndex*(const uint256&)> insertBlockIndex)
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(DB_BLOCK_INDEX, uint256()));

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        if (ShutdownRequested()) return false;
        std::pair<char, uint256> key;
        if (pcursor->GetKey(key) && key.first == DB_BLOCK_INDEX) {
            CDiskBlockIndex diskindex;
            if (pcursor->GetValue(diskindex)) {
                // Construct block index object
                CBlockIndex* pindexNew = insertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev          = insertBlockIndex(diskindex.hashPrev);
                pindexNew->nHeight        = diskindex.nHeight;
                pindexNew->nFile          = diskindex.nFile;
                pindexNew->nDataPos       = diskindex.nDataPos;
                pindexNew->nUndoPos       = diskindex.nUndoPos;
                pindexNew->nVersion       = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->nTime          = diskindex.nTime;
                pindexNew->nBits          = diskindex.nBits;
                pindexNew->nNonce         = diskindex.nNonce;
#ifdef ENABLE_MOMENTUM_HASH_ALGO
                pindexNew->nBirthdayA     = diskindex.nBirthdayA;
                pindexNew->nBirthdayB     = diskindex.nBirthdayB;
#endif
                pindexNew->nStatus        = diskindex.nStatus;
                pindexNew->nTx            = diskindex.nTx;
#ifdef ENABLE_PROOF_OF_STAKE
                pindexNew->nMoneySupply   = diskindex.nMoneySupply;
                pindexNew->nStakeModifier = diskindex.nStakeModifier;
                pindexNew->prevoutStake   = diskindex.prevoutStake;
                pindexNew->vchBlockSig    = diskindex.vchBlockSig;

                if (!CheckIndexProof(*pindexNew, consensusParams))
                    return error("%s: CheckIndexProof failed: %s", __func__, pindexNew->ToString());

                // NovaCoin: build setStakeSeen
                if (pindexNew->IsProofOfStake())
                    setStakeSeen.insert(std::make_pair(pindexNew->prevoutStake, pindexNew->nTime));
#else
                if (!CheckProofOfWork(pindexNew->GetBlockHash(), pindexNew->nBits, consensusParams))
                    return error("%s: CheckProofOfWork failed: %s", __func__, pindexNew->ToString());
#endif

                pcursor->Next();
            } else {
                return error("%s: failed to read value", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}

namespace {

//! Legacy class to deserialize pre-pertxout database entries without reindex.
class CCoins
{
public:
    //! whether transaction is a coinbase
    bool fCoinBase;
#ifdef ENABLE_PROOF_OF_STAKE
    bool fCoinStake;
#endif

    //! unspent transaction outputs; spent outputs are .IsNull(); spent outputs at the end of the array are dropped
    std::vector<CTxOut> vout;

    //! at which height this transaction was included in the active block chain
    int nHeight;

    //! empty constructor
#ifdef ENABLE_PROOF_OF_STAKE
    CCoins() : fCoinBase(false), fCoinStake(false),vout(0), nHeight(0) { }
#else
    CCoins() : fCoinBase(false), vout(0), nHeight(0) { }
#endif

    template<typename Stream>
    void Unserialize(Stream &s) {
        unsigned int nCode = 0;
        // version
        unsigned int nVersionDummy;
        ::Unserialize(s, VARINT(nVersionDummy));
        // header code
        ::Unserialize(s, VARINT(nCode));
        fCoinBase = nCode & 1;
#ifdef ENABLE_PROOF_OF_STAKE
        fCoinStake = nCode & 16;
#endif
        std::vector<bool> vAvail(2, false);
        vAvail[0] = (nCode & 2) != 0;
        vAvail[1] = (nCode & 4) != 0;
        unsigned int nMaskCode = (nCode / 8) + ((nCode & 6) != 0 ? 0 : 1);
        // spentness bitmask
        while (nMaskCode > 0) {
            unsigned char chAvail = 0;
            ::Unserialize(s, chAvail);
            for (unsigned int p = 0; p < 8; p++) {
                bool f = (chAvail & (1 << p)) != 0;
                vAvail.push_back(f);
            }
            if (chAvail != 0)
                nMaskCode--;
        }
        // txouts themself
        vout.assign(vAvail.size(), CTxOut());
        for (unsigned int i = 0; i < vAvail.size(); i++) {
            if (vAvail[i])
                ::Unserialize(s, CTxOutCompressor(vout[i]));
        }
        // coinbase height
        ::Unserialize(s, VARINT(nHeight, VarIntMode::NONNEGATIVE_SIGNED));
    }
};

}

/** Upgrade the database from older formats.
 *
 * Currently implemented: from the per-tx utxo model (0.8..0.14.x) to per-txout.
 */
bool CCoinsViewDB::Upgrade() {
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());
    pcursor->Seek(std::make_pair(DB_COINS, uint256()));
    if (!pcursor->Valid()) {
        return true;
    }

    int64_t count = 0;
    LogPrintf("Upgrading utxo-set database...\n");
    LogPrintf("[0%%]..."); /* Continued */
    uiInterface.ShowProgress(_("Upgrading UTXO database"), 0, true);
    size_t batch_size = 1 << 24;
    CDBBatch batch(db);
    int reportDone = 0;
    std::pair<unsigned char, uint256> key;
    std::pair<unsigned char, uint256> prev_key = {DB_COINS, uint256()};
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        if (ShutdownRequested()) {
            break;
        }
        if (pcursor->GetKey(key) && key.first == DB_COINS) {
            if (count++ % 256 == 0) {
                uint32_t high = 0x100 * *key.second.begin() + *(key.second.begin() + 1);
                int percentageDone = (int)(high * 100.0 / 65536.0 + 0.5);
                uiInterface.ShowProgress(_("Upgrading UTXO database"), percentageDone, true);
                if (reportDone < percentageDone/10) {
                    // report max. every 10% step
                    LogPrintf("[%d%%]...", percentageDone); /* Continued */
                    reportDone = percentageDone/10;
                }
            }
            CCoins old_coins;
            if (!pcursor->GetValue(old_coins)) {
                return error("%s: cannot parse CCoins record", __func__);
            }
            COutPoint outpoint(key.second, 0);
            for (size_t i = 0; i < old_coins.vout.size(); ++i) {
                if (!old_coins.vout[i].IsNull() && !old_coins.vout[i].scriptPubKey.IsUnspendable()) {
#ifdef ENABLE_PROOF_OF_STAKE
                    Coin newcoin(std::move(old_coins.vout[i]), old_coins.nHeight, old_coins.fCoinBase, old_coins.fCoinStake);
#else
                    Coin newcoin(std::move(old_coins.vout[i]), old_coins.nHeight, old_coins.fCoinBase);
#endif
                    outpoint.n = i;
                    CoinEntry entry(&outpoint);
                    batch.Write(entry, newcoin);
                }
            }
            batch.Erase(key);
            if (batch.SizeEstimate() > batch_size) {
                db.WriteBatch(batch);
                batch.Clear();
                db.CompactRange(prev_key, key);
                prev_key = key;
            }
            pcursor->Next();
        } else {
            break;
        }
    }
    db.WriteBatch(batch);
    db.CompactRange({DB_COINS, uint256()}, key);
    uiInterface.ShowProgress("", 100, false);
    LogPrintf("[%s].\n", ShutdownRequested() ? "CANCELLED" : "DONE");
    return !ShutdownRequested();
}
