// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockstorage.h>

#include <chain.h>
#include <chainparams.h>
#include <flatfile.h>
#include <fs.h>
#include <pow.h>
#include <shutdown.h>
#include <signet.h>
#include <streams.h>
#include <util/system.h>
#include <validation.h>
// SYSCOIN
#include <evo/deterministicmns.h>
#include <dsnotificationinterface.h>
#include <walletinitinterface.h>
#include <primitives/block.h>
// From validation. TODO move here
bool FindBlockPos(FlatFilePos& pos, unsigned int nAddSize, unsigned int nHeight, CChain& active_chain, uint64_t nTime, bool fKnown = false);

/* Generic implementation of block reading that can handle
   both a block and its header.  */

template<typename T>
static bool ReadBlockOrHeader(T& block, const FlatFilePos& pos, const Consensus::Params& consensusParams)
{
    block.SetNull();

    // Open history file to read
    CAutoFile filein(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
        return error("ReadBlockFromDisk: OpenBlockFile failed for %s", pos.ToString());

    // Read block
    try {
        filein >> block;
    }
    catch (const std::exception& e) {
        return error("%s: Deserialize or I/O error - %s at %s", __func__, e.what(), pos.ToString());
    }

    // Check the header
    if (!CheckProofOfWork(block, consensusParams))
        return error("ReadBlockFromDisk: Errors in block header at %s", pos.ToString());

    // Signet only: check block solution
    if (consensusParams.signet_blocks && !CheckSignetBlockSolution(block, consensusParams)) {
        return error("ReadBlockFromDisk: Errors in block solution at %s", pos.ToString());
    }

    return true;
}
template<typename T>
static bool ReadBlockOrHeader(T& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams)
{
    FlatFilePos blockPos;
    {
        LOCK(cs_main);
        blockPos = pindex->GetBlockPos();
    }

    if (!ReadBlockOrHeader(block, blockPos, consensusParams))
        return false;
    if (block.GetHash() != pindex->GetBlockHash())
        return error("ReadBlockFromDisk(CBlock&, CBlockIndex*): GetHash() doesn't match index for %s at %s",
                pindex->ToString(), pindex->GetBlockPos().ToString());
    return true;
}
bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos, const Consensus::Params& consensusParams)
{
    return ReadBlockOrHeader(block, pos, consensusParams);
}

 bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams)
{
    return ReadBlockOrHeader(block, pindex, consensusParams);
}

 bool ReadBlockHeaderFromDisk(CBlockHeader& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams)
{
    return ReadBlockOrHeader(block, pindex, consensusParams);
}

static bool WriteBlockToDisk(const CBlock& block, FlatFilePos& pos, const CMessageHeader::MessageStartChars& messageStart)
{
    // Open history file to append
    CAutoFile fileout(OpenBlockFile(pos), SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull()) {
        return error("WriteBlockToDisk: OpenBlockFile failed");
    }

    // Write index header
    unsigned int nSize = GetSerializeSize(block, fileout.GetVersion());
    fileout << messageStart << nSize;

    // Write block
    long fileOutPos = ftell(fileout.Get());
    if (fileOutPos < 0) {
        return error("WriteBlockToDisk: ftell failed");
    }
    pos.nPos = (unsigned int)fileOutPos;
    fileout << block;

    return true;
}

bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos, const CMessageHeader::MessageStartChars& message_start)
{
    FlatFilePos hpos = pos;
    hpos.nPos -= 8; // Seek back 8 bytes for meta header
    CAutoFile filein(OpenBlockFile(hpos, true), SER_DISK, CLIENT_VERSION);
    if (filein.IsNull()) {
        return error("%s: OpenBlockFile failed for %s", __func__, pos.ToString());
    }

    try {
        CMessageHeader::MessageStartChars blk_start;
        unsigned int blk_size;

        filein >> blk_start >> blk_size;

        if (memcmp(blk_start, message_start, CMessageHeader::MESSAGE_START_SIZE)) {
            return error("%s: Block magic mismatch for %s: %s versus expected %s", __func__, pos.ToString(),
                         HexStr(blk_start),
                         HexStr(message_start));
        }

        if (blk_size > MAX_SIZE) {
            return error("%s: Block data is larger than maximum deserialization size for %s: %s versus %s", __func__, pos.ToString(),
                         blk_size, MAX_SIZE);
        }

        block.resize(blk_size); // Zeroing of memory is intentional here
        filein.read((char*)block.data(), blk_size);
    } catch (const std::exception& e) {
        return error("%s: Read from block file failed: %s for %s", __func__, e.what(), pos.ToString());
    }

    return true;
}

bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const CBlockIndex* pindex, const CMessageHeader::MessageStartChars& message_start)
{
    FlatFilePos block_pos;
    {
        LOCK(cs_main);
        block_pos = pindex->GetBlockPos();
    }

    return ReadRawBlockFromDisk(block, block_pos, message_start);
}

/** Store block on disk. If dbp is non-nullptr, the file is known to already reside on disk */
FlatFilePos SaveBlockToDisk(const CBlock& block, int nHeight, CChain& active_chain, const CChainParams& chainparams, const FlatFilePos* dbp)
{
    unsigned int nBlockSize = ::GetSerializeSize(block, CLIENT_VERSION);
    FlatFilePos blockPos;
    if (dbp != nullptr) {
        blockPos = *dbp;
    }
    if (!FindBlockPos(blockPos, nBlockSize + 8, nHeight, active_chain, block.GetBlockTime(), dbp != nullptr)) {
        error("%s: FindBlockPos failed", __func__);
        return FlatFilePos();
    }
    if (dbp == nullptr) {
        if (!WriteBlockToDisk(block, blockPos, chainparams.MessageStart())) {
            AbortNode("Failed to write block");
            return FlatFilePos();
        }
    }
    return blockPos;
}

struct CImportingNow {
    CImportingNow()
    {
        assert(fImporting == false);
        fImporting = true;
    }

    ~CImportingNow()
    {
        assert(fImporting == true);
        fImporting = false;
    }
};

void ThreadImport(ChainstateManager& chainman, std::vector<fs::path> vImportFiles, const ArgsManager& args, CDSNotificationInterface* pdsNotificationInterface, std::unique_ptr<CDeterministicMNManager> &deterministicMNManager, const WalletInitInterface &g_wallet_init_interface)
{
    const CChainParams& chainparams = Params();
    ScheduleBatchPriority();

    {
        CImportingNow imp;

        // -reindex
        if (fReindex) {
            int nFile = 0;
            while (true) {
                FlatFilePos pos(nFile, 0);
                if (!fs::exists(GetBlockPosFilename(pos))) {
                    break; // No block files left to reindex
                }
                FILE* file = OpenBlockFile(pos, true);
                if (!file) {
                    break; // This error is logged in OpenBlockFile
                }
                LogPrintf("Reindexing block file blk%05u.dat...\n", (unsigned int)nFile);
                chainman.ActiveChainstate().LoadExternalBlockFile(chainparams, file, &pos);
                if (ShutdownRequested()) {
                    LogPrintf("Shutdown requested. Exit %s\n", __func__);
                    return;
                }
                nFile++;
            }
            pblocktree->WriteReindexing(false);
            fReindex = false;
            LogPrintf("Reindexing finished\n");
            // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
            chainman.ActiveChainstate().LoadGenesisBlock(chainparams);
        }

        // -loadblock=
        for (const fs::path& path : vImportFiles) {
            FILE* file = fsbridge::fopen(path, "rb");
            if (file) {
                LogPrintf("Importing blocks file %s...\n", path.string());
                chainman.ActiveChainstate().LoadExternalBlockFile(chainparams, file);
                if (ShutdownRequested()) {
                    LogPrintf("Shutdown requested. Exit %s\n", __func__);
                    return;
                }
            } else {
                LogPrintf("Warning: Could not open blocks file %s\n", path.string());
            }
        }

        // scan for better chains in the block chain database, that are not yet connected in the active best chain

        // We can't hold cs_main during ActivateBestChain even though we're accessing
        // the chainman unique_ptrs since ABC requires us not to be holding cs_main, so retrieve
        // the relevant pointers before the ABC call.
        for (CChainState* chainstate : WITH_LOCK(::cs_main, return chainman.GetAll())) {
            BlockValidationState state;
            if (!chainstate->ActivateBestChain(state, chainparams, nullptr)) {
                LogPrintf("Failed to connect best block (%s)\n", state.ToString());
                StartShutdown();
                return;
            }
        }
        // SYSCOIN
        if(pdsNotificationInterface)
            pdsNotificationInterface->InitializeCurrentBlockTip();
        // Get all UTXOs for each MN collateral in one go so that we can fill coin cache early
        // and reduce further locking overhead for cs_main in other parts of code including GUI
        LogPrintf("Filling coin cache with masternode UTXOs...\n");
        int64_t nStart = GetTimeMillis();
        CDeterministicMNList mnList;
        if(deterministicMNManager)
            deterministicMNManager->GetListAtChainTip(mnList);
        mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
            Coin coin;
            GetUTXOCoin(dmn->collateralOutpoint, coin);
        });
        LogPrintf("Filling coin cache with masternode UTXOs: done in %dms\n", GetTimeMillis() - nStart);

        
        g_wallet_init_interface.AutoLockMasternodeCollaterals();
        if (args.GetBoolArg("-stopafterblockimport", DEFAULT_STOPAFTERBLOCKIMPORT)) {
            LogPrintf("Stopping after block import\n");
            StartShutdown();
            return;
        }
    } // End scope of CImportingNow
    chainman.ActiveChainstate().LoadMempool(args);
}
