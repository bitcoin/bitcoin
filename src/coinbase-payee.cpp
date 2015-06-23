// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coinbase-payee.h"
#include "util.h"
#include "addrman.h"
#include "masternode.h"
#include "darksend.h"
#include "masternodeman.h"
#include <boost/filesystem.hpp>

CCoinbasePayee coinbasePayee;

//
// CCoinbasePayeeDB
//

CCoinbasePayeeDB::CCoinbasePayeeDB()
{
    pathDB = GetDataDir() / "coinbase-payee.dat";
    strMagicMessage = "CoinbasePayeeDB";
}

bool CCoinbasePayeeDB::Write(const CCoinbasePayee& objToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssObj(SER_DISK, CLIENT_VERSION);
    ssObj << strMagicMessage; // masternode cache file specific magic message
    ssObj << FLATDATA(Params().MessageStart()); // network specific magic number
    ssObj << objToSave;
    uint256 hash = Hash(ssObj.begin(), ssObj.end());
    ssObj << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(pathDB.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s : Failed to open file %s", __func__, pathDB.string());

    // Write and commit header, data
    try {
        fileout << ssObj;
    }
    catch (std::exception &e) {
        return error("%s : Serialize or I/O error - %s", __func__, e.what());
    }
    fileout.fclose();

    LogPrintf("Written info to coinbase-payee.dat  %dms\n", GetTimeMillis() - nStart);

    return true;
}

CCoinbasePayeeDB::ReadResult CCoinbasePayeeDB::Read(CCoinbasePayee& objToLoad)
{

    int64_t nStart = GetTimeMillis();
    // open input file, and associate with CAutoFile
    FILE *file = fopen(pathDB.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
    {
        error("%s : Failed to open file %s", __func__, pathDB.string());
        return FileError;
    }

    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(pathDB);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    vector<unsigned char> vchData;
    vchData.resize(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try {
        filein.read((char *)&vchData[0], dataSize);
        filein >> hashIn;
    }
    catch (std::exception &e) {
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    CDataStream ssObj(vchData, SER_DISK, CLIENT_VERSION);

    // verify stored checksum matches input data
    uint256 hashTmp = Hash(ssObj.begin(), ssObj.end());
    if (hashIn != hashTmp)
    {
        error("%s : Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }


    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    try {
        // de-serialize file header (masternode cache file specific magic message) and ..
        ssObj >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid masternode cache magic message", __func__);
            return IncorrectMagicMessage;
        }


        // de-serialize file header (network specific magic number) and ..
        ssObj >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s : Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }

        // de-serialize data into CCoinbasePayee object
        ssObj >> objToLoad;
    }
    catch (std::exception &e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }


    objToLoad.CleanUp(); // clean out expired
    LogPrintf("Loaded info from coinbase-payee.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", objToLoad.ToString());

    return Ok;
}

void DumpCoinbasePayees()
{
    int64_t nStart = GetTimeMillis();

    CCoinbasePayeeDB mndb;
    CCoinbasePayee temp;

    LogPrintf("Verifying coinbase-payee.dat format...\n");
    CCoinbasePayeeDB::ReadResult readResult = mndb.Read(temp);
    // there was an error and it was not an error on file openning => do not proceed
    if (readResult == CCoinbasePayeeDB::FileError)
        LogPrintf("Missing payees cache file - coinbase-payee.dat, will try to recreate\n");
    else if (readResult != CCoinbasePayeeDB::Ok)
    {
        LogPrintf("Error reading coinbase-payee.dat: ");
        if(readResult == CCoinbasePayeeDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to coinbase-payee.dat...\n");
    mndb.Write(coinbasePayee);

    LogPrintf("Coinbase payee dump finished  %dms\n", GetTimeMillis() - nStart);
}

void CCoinbasePayee::BuildIndex()
{
    if(mapPaidTime.size() > 0) {
        LogPrintf("CCoinbasePayee::BuildIndex - coinbase cache exists, skipping BuildIndex\n");
        return;
    }

    //scan last 30 days worth of blocks, run processBlockCoinbaseTX for each

    CBlockIndex* pindexPrev = chainActive.Tip();
    int count = 0;

    for (unsigned int i = 1; pindexPrev && pindexPrev->nHeight > 0; i++) {
        count++;
        if(count > 18000) return;

        CBlock block;
        if (ReadBlockFromDisk(block, pindexPrev)) {
            printf("scan block\n");
            ProcessBlockCoinbaseTX(block.vtx[0], block.nTime);
        }

        if (pindexPrev->pprev == NULL) { assert(pindexPrev); break; }
        pindexPrev = pindexPrev->pprev;
    }

    return;
}

void CCoinbasePayee::ProcessBlockCoinbaseTX(CTransaction& txCoinbase, int64_t nTime)
{
    if (!txCoinbase.IsCoinBase()){
        LogPrintf("ERROR: CCoinbasePayee::ProcessBlockCoinbaseTX - tx is not coinbase\n");
        return;
    }

    BOOST_FOREACH(CTxOut out, txCoinbase.vout){
        uint256 h = GetScriptHash(out.scriptPubKey);
        if(mapPaidTime.count(h)){
            if(mapPaidTime[h] < nTime) {
                mapPaidTime[h] = nTime;
            }
        } else {
            mapPaidTime[h] = nTime;
        }
    }
}

int64_t CCoinbasePayee::GetLastPaid(CScript& pubkey)
{
    uint256 h = GetScriptHash(pubkey);

    if(mapPaidTime.count(h)){
        return mapPaidTime[h];
    }

    return 0;
}

void CCoinbasePayee::CleanUp()
{
    std::map<uint256, int64_t>::iterator it = mapPaidTime.begin();
    while(it != mapPaidTime.end())
    {
        //keep 30 days of history
        if((*it).second < GetAdjustedTime() - (60*60*24*30)) {
            mapPaidTime.erase(it++);
        } else {
            ++it;
        }
    }
}