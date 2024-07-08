// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_FLATDATABASE_H
#define SYSCOIN_FLATDATABASE_H

#include <clientversion.h>
#include <chainparams.h>
#include <util/fs.h>
#include <hash.h>
#include <streams.h>
#include <common/args.h>
#include <logging.h>
/**
*   Generic Dumping and Loading
*   ---------------------------
*/

template<typename T>
class CFlatDB
{
private:

    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    fs::path pathDB;
    std::string strFilename;
    std::string strMagicMessage;

    bool CoreWrite(const T& objToSave)
    {
        // LOCK(objToSave.cs);

        int64_t nStart = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());

        // serialize, checksum data up to that point, then append checksum
        CDataStream ssObj(SER_DISK, CLIENT_VERSION);
        ssObj << strMagicMessage; // specific magic message for this type of object
        ssObj << Params().MessageStart(); // network specific magic number
        ssObj << objToSave;
        uint256 hash = Hash(ssObj);
        ssObj << hash;

        // open output file, and associate with CAutoFile
        FILE *file = fopen(pathDB.u8string().c_str(), "wb");
        CAutoFile fileout(file, CLIENT_VERSION);
        if (fileout.IsNull())
            return error("%s: Failed to open file %s", __func__, pathDB.u8string());

        // Write and commit header, data
        try {
            fileout << ssObj;
        }
        catch (std::exception &e) {
            return error("%s: Serialize or I/O error - %s", __func__, e.what());
        }
        fileout.fclose();

        LogPrintf("Written info to %s  %dms\n", strFilename, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - nStart);
        LogPrintf("     %s\n", objToSave.ToString());

        return true;
    }

    ReadResult CoreRead(T& objToLoad)
    {
        //LOCK(objToLoad.cs);

        int64_t nStart = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
        // open input file, and associate with CAutoFile
        FILE *file = fopen(pathDB.u8string().c_str(), "rb");
        CAutoFile filein(file, CLIENT_VERSION);
        if (filein.IsNull())
        {
            error("%s: Failed to open file %s", __func__, pathDB.u8string());
            return FileError;
        }

        // use file size to size memory buffer
        int fileSize = fs::file_size(pathDB);
        int dataSize = fileSize - sizeof(uint256);
        // Don't try to resize to a negative number if file is small
        if (dataSize < 0)
            dataSize = 0;
        std::vector<unsigned char> vchData;
        vchData.resize(dataSize);
        uint256 hashIn;

        // read data and checksum from file
        try {
            filein.read(MakeWritableByteSpan(vchData));
            filein >> hashIn;
        }
        catch (std::exception &e) {
            error("%s: Deserialize or I/O error - %s", __func__, e.what());
            return HashReadError;
        }
        filein.fclose();

        CDataStream ssObj(vchData, SER_DISK, CLIENT_VERSION);

        // verify stored checksum matches input data
        uint256 hashTmp = Hash(ssObj);
        if (hashIn != hashTmp)
        {
            error("%s: Checksum mismatch, data corrupted", __func__);
            return IncorrectHash;
        }


        MessageStartChars pchMsgTmp;
        std::string strMagicMessageTmp;
        try {
            // de-serialize file header (file specific magic message) and ..
            ssObj >> strMagicMessageTmp;

            // ... verify the message matches predefined one
            if (strMagicMessage != strMagicMessageTmp)
            {
                error("%s: Invalid magic message", __func__);
                return IncorrectMagicMessage;
            }


            // de-serialize file header (network specific magic number) and ..
            ssObj >> pchMsgTmp;

            // ... verify the network matches ours
            if (pchMsgTmp != Params().MessageStart())
            {
                error("%s: Invalid network magic number", __func__);
                return IncorrectMagicNumber;
            }

            // de-serialize data into T object
            ssObj >> objToLoad;
        }
        catch (std::exception &e) {
            objToLoad.Clear();
            error("%s: Deserialize or I/O error - %s", __func__, e.what());
            return IncorrectFormat;
        }

        LogPrintf("Loaded info from %s  %dms\n", strFilename, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - nStart);
        LogPrintf("     %s\n", objToLoad.ToString());

        return Ok;
    }

    bool Read(T& objToLoad)
    {
        ReadResult readResult = CoreRead(objToLoad);
        if (readResult == FileError)
            LogPrintf("Missing file %s, will try to recreate\n", strFilename);
        else if (readResult != Ok)
        {
            LogPrintf("Error reading %s: ", strFilename);
            if(readResult == IncorrectFormat)
            {
                LogPrintf("%s: Magic is ok but data has invalid format, will try to recreate\n", __func__);
            }
            else {
                LogPrintf("%s: File format is unknown or invalid, please fix it manually\n", __func__);
                // program should exit with an error
                return false;
            }
        }
        return true;
    }

public:
    CFlatDB(std::string strFilenameIn, std::string strMagicMessageIn)
    {
        pathDB = gArgs.GetDataDirNet() / fs::u8path(strFilenameIn);
        strFilename = strFilenameIn;
        strMagicMessage = strMagicMessageIn;
    }

    bool Load(T& objToLoad)
    {
        LogPrintf("Reading info from %s...\n", strFilename);
        return Read(objToLoad);
    }

    bool Store(T& objToSave)
    {
        LogPrintf("Verifying %s format...\n", strFilename);
        T tmpObjToLoad;
        if (!Read(tmpObjToLoad)) return false;

        int64_t nStart = TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());

        LogPrintf("Writing info to %s...\n", strFilename);
        CoreWrite(objToSave);
        LogPrintf("%s dump finished  %dms\n", strFilename, TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now()) - nStart);

        return true;
    }
};


#endif // SYSCOIN_FLATDATABASE_H
