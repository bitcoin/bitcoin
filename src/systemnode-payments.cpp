// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "systemnode-payments.h"
#include "systemnode-sync.h"
#include "systemnodeman.h"
#include "legacysigner.h"
#include "util.h"
#include "sync.h"
#include "spork.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

/** Object for who's going to get paid on which blocks */
CSystemnodePayments systemnodePayments;

CCriticalSection cs_vecSNPayments;
CCriticalSection cs_mapSystemnodeBlocks;
CCriticalSection cs_mapSystemnodePayeeVotes;

//
// CSystemnodePaymentDB
//

CSystemnodePaymentDB::CSystemnodePaymentDB()
{
    pathDB = GetDataDir() / "snpayments.dat";
    strMagicMessage = "SystemnodePayments";
}

bool CSystemnodePaymentDB::Write(const CSystemnodePayments& objToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssObj(SER_DISK, CLIENT_VERSION);
    ssObj << strMagicMessage; // systemnode cache file specific magic message
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

    LogPrintf("Written info to snpayments.dat  %dms\n", GetTimeMillis() - nStart);

    return true;
}

CSystemnodePaymentDB::ReadResult CSystemnodePaymentDB::Read(CSystemnodePayments& objToLoad, bool fDryRun)
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
        // de-serialize file header (systemnode cache file specific magic message) and ..
        ssObj >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (strMagicMessage != strMagicMessageTmp)
        {
            error("%s : Invalid systemnode payement cache magic message", __func__);
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

        // de-serialize data into CSystemnodePayments object
        ssObj >> objToLoad;
    }
    catch (std::exception &e) {
        objToLoad.Clear();
        error("%s : Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    LogPrintf("Loaded info from snpayments.dat  %dms\n", GetTimeMillis() - nStart);
    LogPrintf("  %s\n", objToLoad.ToString());
    if(!fDryRun) {
        LogPrintf("Systemnode payments manager - cleaning....\n");
        objToLoad.CleanPaymentList();
        LogPrintf("Systemnode payments manager - result:\n");
        LogPrintf("  %s\n", objToLoad.ToString());
    }

    return Ok;
}

void DumpSystemnodePayments()
{
    int64_t nStart = GetTimeMillis();

    CSystemnodePaymentDB paymentdb;
    CSystemnodePayments tempPayments;

    LogPrintf("Verifying mnpayments.dat format...\n");
    CSystemnodePaymentDB::ReadResult readResult = paymentdb.Read(tempPayments, true);
    // there was an error and it was not an error on file opening => do not proceed
    if (readResult == CSystemnodePaymentDB::FileError)
        LogPrintf("Missing budgets file - mnpayments.dat, will try to recreate\n");
    else if (readResult != CSystemnodePaymentDB::Ok)
    {
        LogPrintf("Error reading mnpayments.dat: ");
        if(readResult == CSystemnodePaymentDB::IncorrectFormat)
            LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
        else
        {
            LogPrintf("file format is unknown or invalid, please fix it manually\n");
            return;
        }
    }
    LogPrintf("Writting info to mnpayments.dat...\n");
    paymentdb.Write(systemnodePayments);

    LogPrintf("Budget dump finished  %dms\n", GetTimeMillis() - nStart);
}

void SNFillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;
    systemnodePayments.FillBlockPayee(txNew, nFees);
}

void CSystemnodePayments::FillBlockPayee(CMutableTransaction& txNew, int64_t nFees)
{
    CBlockIndex* pindexPrev = chainActive.Tip();
    if(!pindexPrev) return;

    bool hasPayment = true;
    CScript payee;

    //spork
    if(!systemnodePayments.GetBlockPayee(pindexPrev->nHeight+1, payee)){
        //no systemnode detected
        CSystemnode* winningNode = snodeman.GetCurrentSystemNode(1);
        if(winningNode) {
            payee = GetScriptForDestination(winningNode->pubkey.GetID());
        } else {
            LogPrintf("CreateNewBlock: Failed to detect systemnode to pay\n");
            hasPayment = false;
        }
    }

    CAmount blockValue = GetBlockValue(pindexPrev->nBits, pindexPrev->nHeight, nFees);
    CAmount systemnodePayment = GetSystemnodePayment(pindexPrev->nHeight+1, blockValue);

    // This is already done in masternode
    //txNew.vout[0].nValue = blockValue;

    if(hasPayment) {
        txNew.vout.resize(3);

        // [0] is for miner, [1] masternode, [2] systemnode
        txNew.vout[2].scriptPubKey = payee;
        txNew.vout[2].nValue = systemnodePayment;

        txNew.vout[0].nValue -= systemnodePayment;

        CTxDestination address1;
        ExtractDestination(payee, address1);
        CBitcoinAddress address2(address1);

        LogPrintf("Systemnode payment to %s\n", address2.ToString().c_str());
    }
}

bool CSystemnodePayments::GetBlockPayee(int nBlockHeight, CScript& payee)
{
    if(mapSystemnodeBlocks.count(nBlockHeight)){
        return mapSystemnodeBlocks[nBlockHeight].GetPayee(payee);
    }

    return false;
}

void CSystemnodePayments::CleanPaymentList()
{
}

std::string CSystemnodePayments::ToString() const
{
}
