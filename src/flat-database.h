// Copyright (c) 2014-2016 The Dash Core developers

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef FLAT_DATABASE_H
#define FLAT_DATABASE_H

#include "main.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "util.h"
#include "base58.h"
#include "masternode.h"
#include <boost/lexical_cast.hpp>
#include "init.h"

#include "clientversion.h"

#
using namespace std;

template<typename T>
class CFlatDB;


/** 
*   Generic Dumping and Loading Functions
*   --------------------------------------
*
*/

template<typename T, typename S>
bool LoadFlatDB(T& objToLoad, CFlatDB<S>& flatdb);

template<typename T, typename S>
bool DumpFlatDB(T& objToSave, CFlatDB<S>& flatdb);

/**
*   Reusable flat database
*    -----------------------------------------
*
*    Usage:
*
*        CFlatDB govdb("budget.dat", "MagicalString");
*
*        CFlatDB::ReadResult readResult = govdb.Read(tempBudget, true);
*        // there was an error and it was not an error on file opening => do not proceed
*        if (readResult == CFlatDB::FileError)
*            LogPrintf("Missing budgets file - budget.dat, will try to recreate\n");
*        else if (readResult != CFlatDB::Ok)
*        {
*            LogPrintf("Error reading budget.dat: ");
*            if(readResult == CFlatDB::IncorrectFormat)
*                LogPrintf("magic is ok but data has invalid format, will try to recreate\n");
*            else
*            {
*                LogPrintf("file format is unknown or invalid, please fix it manually\n");
*                return;
*            }
*        }
*        LogPrintf("Writting info to budget.dat...\n");
*        govdb.Write(governance);
*
*        LogPrintf("Budget dump finished  %dms\n", GetTimeMillis() - nStart);
*    
*/

enum CFlatDB_ReadResult {
    Ok,
    FileError,
    HashReadError,
    IncorrectHash,
    IncorrectMagicMessage,
    IncorrectMagicNumber,
    IncorrectFormat
};

template<typename T>
class CFlatDB
{
private:
    boost::filesystem::path pathDB;
    std::string strFilename;
    std::string strMagicMessage;

public:

    CFlatDB(std::string strFilenameIn, std::string strMagicMessageIn);
    bool Write(const T &objToSave);
    CFlatDB_ReadResult Read(T& objToLoad, bool fDryRun = false);

    std::string& GetFilename() {return strFilename;}
};


#endif