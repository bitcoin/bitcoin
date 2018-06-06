// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DB_DETAILS_H
#define DB_DETAILS_H

#include "chainparams.h"
#include "clientversion.h"
#include "hash.h"
#include "streams.h"
#include "util.h"
#include <boost/filesystem.hpp>

namespace details
{
    enum ReadResult
    {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    ReadResult ReadStream(CDataStream& stream, const std::string& filename);

    template <typename T>
    ReadResult DeserializeStream(CDataStream& stream, const std::string& magicMessage, T& objToLoad)
    {
        unsigned char pchMsgTmp[4];
        std::string strMagicMessageTmp;
        // de-serialize file header (file specific magic message) and ..
        stream >> strMagicMessageTmp;

        // ... verify the message matches predefined one
        if (magicMessage != strMagicMessageTmp)
        {
            error("%s: Invalid magic message", __func__);
            return IncorrectMagicMessage;
        }

        // de-serialize file header (network specific magic number) and ..
        stream >> FLATDATA(pchMsgTmp);

        // ... verify the network matches ours
        if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
        {
            error("%s: Invalid network magic number", __func__);
            return IncorrectMagicNumber;
        }

        // de-serialize data into T object
        stream >> objToLoad;
        return Ok;
    }

    template <typename T>
    ReadResult Read(T& objToLoad, const std::string& filename, const std::string& magicMessage, bool fDryRun = false)
    {
        CDataStream ssObj(SER_DISK, CLIENT_VERSION);
        ReadResult result = ReadStream(ssObj, filename);
        if (result != Ok)
            return result;
        try
        {
            result = DeserializeStream(ssObj, magicMessage, objToLoad);
            if (result != Ok)
                return result;
        }
        catch (const std::exception &e)
        {
            objToLoad.Clear();
            error("%s: Deserialize or I/O error - %s", __func__, e.what());
            return IncorrectFormat;
        }

        int64_t nStart = GetTimeMillis();
        LogPrintf("Loaded info from %s  %dms\n", filename, GetTimeMillis() - nStart);
        LogPrintf("     %s\n", objToLoad.ToString());
        if(!fDryRun)
        {
            LogPrintf("%s: Cleaning....\n", __func__);
            objToLoad.CheckAndRemove();
            LogPrintf("     %s\n", objToLoad.ToString());
        }

        return Ok;
    }

    template <typename T>
    bool Write(const T& objToSave, const std::string& filename, const std::string& magicMessage)
    {
        int64_t nStart = GetTimeMillis();

        // serialize, checksum data up to that point, then append checksum
        CDataStream ssObj(SER_DISK, CLIENT_VERSION);
        ssObj << magicMessage; // specific magic message for this type of object
        ssObj << FLATDATA(Params().MessageStart()); // network specific magic number
        ssObj << objToSave;
        uint256 hash = Hash(ssObj.begin(), ssObj.end());
        ssObj << hash;

        // open output file, and associate with CAutoFile
        boost::filesystem::path pathDb = GetDataDir() / filename;
        FILE *file = fopen(pathDb.string().c_str(), "wb");
        CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
        if (fileout.IsNull())
            return error("%s: Failed to open file %s", __func__, pathDb.string());

        // Write and commit header, data
        try
        {
            fileout << ssObj;
        }
        catch (const std::exception &e)
        {
            return error("%s: Serialize or I/O error - %s", __func__, e.what());
        }

        fileout.fclose();

        LogPrintf("Written info to %s  %dms\n", filename, GetTimeMillis() - nStart);
        LogPrintf("     %s\n", objToSave.ToString());

        return true;
    }
}

#endif
