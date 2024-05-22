// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_NEVMCONSENSUS_H
#define SYSCOIN_SERVICES_NEVMCONSENSUS_H
#include <primitives/transaction.h>
#include <dbwrapper.h>
#include <consensus/params.h>
class TxValidationState;
class CCoinsViewCache;
class CTxUndo;
class CBlock;
class BlockValidationState;
class CBlockIndexDB;
class CNEVMData;
class CNEVMTxRootsDB : public CDBWrapper {
    NEVMTxRootMap mapCache;
public:
    using CDBWrapper::CDBWrapper;
    bool FlushErase(const std::vector<uint256> &vecBlockHashes);
    bool ReadTxRoots(const uint256& nBlockHash, NEVMTxRoot& txRoot);
    bool FlushCacheToDisk();
    void FlushDataToCache(const NEVMTxRootMap &mapNEVMTxRoots);
};

class CNEVMMintedTxDB : public CDBWrapper {
    NEVMMintTxMap mapCache;
public:
    using CDBWrapper::CDBWrapper;
    bool FlushErase(const NEVMMintTxMap &mapMintKeys);
    bool FlushWrite(const NEVMMintTxMap &mapMintKeys);
    bool FlushCacheToDisk();
    void FlushDataToCache(const NEVMMintTxMap &mapNEVMTxRoots);
    bool ExistsTx(const uint256& nTxHash);
};
class CNEVMDataDB : public CDBWrapper {
private:
    PoDAMAP mapCache;
public:
    using CDBWrapper::CDBWrapper;
    bool FlushErase(const NEVMDataVec &vecDataKeys);
    bool FlushEraseMTPs(const NEVMDataVec &vecDataKeys);
    bool FlushCacheToDisk();
    void FlushDataToCache(const PoDAMAPMemory &mapPoDA, const int64_t &nMedianTime);
    bool ReadData(const std::vector<uint8_t>& nVersionHash, std::vector<uint8_t>& vchData);
    bool ReadDataSize(const std::vector<uint8_t>& nVersionHash, uint32_t &nSize);
    bool ReadMTP(const std::vector<uint8_t>& nVersionHash, int64_t &nMedianTime);
    bool Prune(int64_t nMedianTime);
    bool ClearZeroMPT();
    bool BlobExists(const std::vector<uint8_t>& vchVersionhash);
    const PoDAMAP& GetMapCache() const { return mapCache;}
};
extern std::unique_ptr<CNEVMTxRootsDB> pnevmtxrootsdb;
extern std::unique_ptr<CNEVMMintedTxDB> pnevmtxmintdb;
extern std::unique_ptr<CNEVMDataDB> pnevmdatadb;
bool DisconnectMint(const CTransaction &tx, const uint256& txHash, NEVMMintTxMap &mapMintKeys);
bool DisconnectSyscoinTransaction(const CTransaction& tx, const uint256& txHash, const CTxUndo& txundo, CCoinsViewCache& view, NEVMMintTxMap &mapMintKeys, NEVMDataVec &NEVMDataVecOut);
bool CheckSyscoinMint(const CTransaction& tx, TxValidationState &tstate, const bool &fJustCheck, const uint32_t& nHeight, NEVMMintTxMap &mapMintKeys, CAmount &nValue);
#endif // SYSCOIN_SERVICES_NEVMCONSENSUS_H
