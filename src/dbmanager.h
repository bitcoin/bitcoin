// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include "dbdetails.h"

template <typename T>
bool Load(T& objToLoad, const std::string& filename, const std::string& magicMessage, bool fDryRun = false)
{
    LogPrintf("Reading info from %s...\n", filename);
    ::details::ReadResult readResult = ::details::Read(objToLoad, filename, magicMessage, fDryRun);
    switch(readResult)
    {
        case ::details::Ok:
            return true;
        case ::details::FileError:
            LogPrintf("Missing file %s, will try to recreate\n", filename);
            break;
        case ::details::IncorrectFormat:
            LogPrintf("Error reading %s: ", filename);
            LogPrintf("%s: Magic is ok but data has invalid format, will try to recreate\n", __func__);
            break;
        default:
            LogPrintf("Error reading %s: ", filename);
            LogPrintf("%s: File format is unknown or invalid, please fix it manually\n", __func__);
            return false;

    }
    return true;
}

template <typename T>
bool Dump(const T& objToSave, const std::string& filename, const std::string& magicMessage)
{
    int64_t nStart = GetTimeMillis();

    LogPrintf("Verifying %s format...\n", filename);
    T tmpObjToLoad;
    if (!Load(tmpObjToLoad, filename, magicMessage, true))
        return false;

    LogPrintf("Writing info to %s...\n", filename);
    ::details::Write(objToSave, filename, magicMessage);
    LogPrintf("%s dump finished  %dms\n", filename, GetTimeMillis() - nStart);

    return true;
}

#endif
