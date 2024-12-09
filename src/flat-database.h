// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FLAT_DATABASE_H
#define BITCOIN_FLAT_DATABASE_H

#include <clientversion.h>
#include <chainparams.h>
#include <fs.h>
#include <hash.h>
#include <streams.h>
#include <util/system.h>

/**
*   Generic Dumping and Loading
*   ---------------------------
*/

template<typename T>
class CFlatDB
{
private:
    enum class ReadResult {
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

    [[nodiscard]] bool CoreWrite(const T& objToSave)
    {
        // LOCK(objToSave.cs);

        const auto start{SteadyClock::now()};

        // serialize, checksum data up to that point, then append checksum
        CDataStream ssObj(SER_DISK, CLIENT_VERSION);
        ssObj << strMagicMessage; // specific magic message for this type of object
        ssObj << Params().MessageStart(); // network specific magic number
        ssObj << objToSave;
        uint256 hash = Hash(ssObj);
        ssObj << hash;

        // open output file, and associate with CAutoFile
        FILE *file = fsbridge::fopen(pathDB, "wb");
        CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
        if (fileout.IsNull()) {
            return error("%s: Failed to open file %s", __func__, fs::PathToString(pathDB));
        }

        // Write and commit header, data
        try {
            fileout << ssObj;
        }
        catch (std::exception &e) {
            return error("%s: Serialize or I/O error - %s", __func__, e.what());
        }
        fileout.fclose();

        LogPrintf("Written info to %s  %dms\n", strFilename, Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
        LogPrintf("     %s\n", objToSave.ToString());

        return true;
    }

    [[nodiscard]] ReadResult CoreRead(T& objToLoad)
    {
        //LOCK(objToLoad.cs);

        const auto start{SteadyClock::now()};
        // open input file, and associate with CAutoFile
        FILE *file = fsbridge::fopen(pathDB, "rb");
        CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
        if (filein.IsNull()) {
            // It is not actually error, maybe it's a first initialization of core.
            return ReadResult::FileError;
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
            return ReadResult::HashReadError;
        }
        filein.fclose();

        CDataStream ssObj(vchData, SER_DISK, CLIENT_VERSION);

        // verify stored checksum matches input data
        uint256 hashTmp = Hash(ssObj);
        if (hashIn != hashTmp)
        {
            error("%s: Checksum mismatch, data corrupted", __func__);
            return ReadResult::IncorrectHash;
        }


        try {
            unsigned char pchMsgTmp[4];
            std::string strMagicMessageTmp;
            // de-serialize file header (file specific magic message) and ..
            ssObj >> strMagicMessageTmp;

            // ... verify the message matches predefined one
            if (strMagicMessage != strMagicMessageTmp)
            {
                error("%s: Invalid magic message", __func__);
                return ReadResult::IncorrectMagicMessage;
            }


            // de-serialize file header (network specific magic number) and ..
            ssObj >> pchMsgTmp;

            // ... verify the network matches ours
            if (memcmp(pchMsgTmp, Params().MessageStart(), sizeof(pchMsgTmp)))
            {
                error("%s: Invalid network magic number", __func__);
                return ReadResult::IncorrectMagicNumber;
            }

            // de-serialize data into T object
            ssObj >> objToLoad;
        }
        catch (std::exception &e) {
            objToLoad.Clear();
            error("%s: Deserialize or I/O error - %s", __func__, e.what());
            return ReadResult::IncorrectFormat;
        }

        LogPrintf("Loaded info from %s  %dms\n", strFilename, Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
        LogPrintf("     %s\n", objToLoad.ToString());

        return ReadResult::Ok;
    }

    [[nodiscard]] bool Read(T& objToLoad)
    {
        ReadResult readResult = CoreRead(objToLoad);
        if (readResult == ReadResult::FileError)
            LogPrintf("Missing file %s, will try to recreate\n", strFilename);
        else if (readResult != ReadResult::Ok) {
            LogPrintf("ERROR: CFlatDB::Read Error reading %s: ", strFilename);
            if (readResult == ReadResult::IncorrectFormat) {
                LogPrintf("%s: Magic is ok but data has invalid format, will try to recreate\n", __func__);
            } else {
                LogPrintf("%s: File format is unknown or invalid, please fix it manually\n", __func__);
                // program should exit with an error
                return false;
            }
        }
        return true;
    }

public:
    CFlatDB(std::string&& strFilenameIn, std::string&& strMagicMessageIn) :
        pathDB{gArgs.GetDataDirNet() / strFilenameIn},
        strFilename{strFilenameIn},
        strMagicMessage{strMagicMessageIn}
    {
    }

    [[nodiscard]] bool Load(T& objToLoad)
    {
        LogPrintf("Reading info from %s...\n", strFilename);
        return Read(objToLoad);
    }

    bool Store(const T& objToSave)
    {
        LogPrintf("Verifying %s format...\n", strFilename);
        T tmpObjToLoad;
        if (!Read(tmpObjToLoad)) return false;

        const auto start{SteadyClock::now()};

        LogPrintf("Writing info to %s...\n", strFilename);
        const bool ret = CoreWrite(objToSave);
        LogPrintf("%s dump finished  %dms\n", strFilename, Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));

        return ret;
    }
};


#endif // BITCOIN_FLAT_DATABASE_H
