// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include "chainparams.h"
#include "clientversion.h"
#include "hash.h"
#include "streams.h"
#include "util.h"

#include <boost/filesystem.hpp>

/** 
*   Generic Dumping and Loading
*   ---------------------------
*/

template<typename T>
class DbManager
{
public:
    DbManager(std::string filename, std::string magicMessage);
    bool Load(T& objToLoad, bool fDryRun = false);
    bool Dump(const T& objToSave);

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

private:
    bool Write(const T& objToSave);
    ReadResult Read(T& objToLoad, bool fDryRun = false);
    ReadResult DeserializeStream(CDataStream& stream, T& objToLoad) const;
    ReadResult ReadStream(CDataStream& stream) const;

private:
    boost::filesystem::path m_pathDb;
    std::string m_filename;
    std::string m_magicMessage;
};

template <typename T>
DbManager<T>::DbManager(std::string filename, std::string magicMessage)
{
    m_pathDb = GetDataDir() / filename;
    m_filename = filename;
    m_magicMessage = magicMessage;
}

template <typename T>
bool DbManager<T>::Write(const T& objToSave)
{
    int64_t nStart = GetTimeMillis();

    // serialize, checksum data up to that point, then append checksum
    CDataStream ssObj(SER_DISK, CLIENT_VERSION);
    ssObj << m_magicMessage; // specific magic message for this type of object
    ssObj << FLATDATA(Params().MessageStart()); // network specific magic number
    ssObj << objToSave;
    uint256 hash = Hash(ssObj.begin(), ssObj.end());
    ssObj << hash;

    // open output file, and associate with CAutoFile
    FILE *file = fopen(m_pathDb.string().c_str(), "wb");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
    {
        return error("%s: Failed to open file %s", __func__, m_pathDb.string());
    }

    // Write and commit header, data
    try
    {
        fileout << ssObj;
    }
    catch (std::exception &e)
    {
        return error("%s: Serialize or I/O error - %s", __func__, e.what());
    }
    fileout.fclose();

    LogPrintf("Written info to %s  %dms\n", m_filename, GetTimeMillis() - nStart);
    LogPrintf("     %s\n", objToSave.ToString());

    return true;
}

template <typename T>
typename DbManager<T>::ReadResult DbManager<T>::ReadStream(CDataStream& stream) const
{
    // open input file, and associate with CAutoFile
    FILE *file = fopen(m_pathDb.string().c_str(), "rb");
    CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
    if (filein.IsNull())
    {
        error("%s: Failed to open file %s", __func__, m_pathDb.string());
        return FileError;
    }
    // use file size to size memory buffer
    int fileSize = boost::filesystem::file_size(m_pathDb);
    int dataSize = fileSize - sizeof(uint256);
    // Don't try to resize to a negative number if file is small
    if (dataSize < 0)
        dataSize = 0;
    std::vector<unsigned char> vchData(dataSize);
    uint256 hashIn;

    // read data and checksum from file
    try
    {
        filein.read((char *)&vchData[0], dataSize);
        filein >> hashIn;
    }
    catch (std::exception &e)
    {
        error("%s: Deserialize or I/O error - %s", __func__, e.what());
        return HashReadError;
    }
    filein.fclose();

    stream = CDataStream(vchData, SER_DISK, CLIENT_VERSION);
    // verify stored checksum matches input data
    uint256 hashTmp = Hash(stream.begin(), stream.end());
    if (hashIn != hashTmp)
    {
        error("%s: Checksum mismatch, data corrupted", __func__);
        return IncorrectHash;
    }
    return Ok;
}

template <typename T>
typename DbManager<T>::ReadResult DbManager<T>::DeserializeStream(CDataStream& stream, T& objToLoad) const
{
    unsigned char pchMsgTmp[4];
    std::string strMagicMessageTmp;
    // de-serialize file header (file specific magic message) and ..
    stream >> strMagicMessageTmp;

    // ... verify the message matches predefined one
    if (m_magicMessage != strMagicMessageTmp)
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
typename DbManager<T>::ReadResult DbManager<T>::Read(T& objToLoad, bool fDryRun)
{
    CDataStream ssObj(SER_DISK, CLIENT_VERSION);
    ReadResult result = ReadStream(ssObj);
    if (result != Ok)
        return result;
    try
    {
        result = DeserializeStream(ssObj, objToLoad);
        if (result != Ok)
            return result;
    }
    catch (std::exception &e)
    {
        objToLoad.Clear();
        error("%s: Deserialize or I/O error - %s", __func__, e.what());
        return IncorrectFormat;
    }

    int64_t nStart = GetTimeMillis();
    LogPrintf("Loaded info from %s  %dms\n", m_filename, GetTimeMillis() - nStart);
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
bool DbManager<T>::Load(T& objToLoad, bool fDryRun)
{
    LogPrintf("Reading info from %s...\n", m_filename);
    ReadResult readResult = Read(objToLoad, fDryRun);
    switch(readResult)
    {
        case Ok:
            return true;
        case FileError:
            LogPrintf("Missing file %s, will try to recreate\n", m_filename);
            break;
        case IncorrectFormat:
            LogPrintf("Error reading %s: ", m_filename);
            LogPrintf("%s: Magic is ok but data has invalid format, will try to recreate\n", __func__);
            break;
        default:
            LogPrintf("Error reading %s: ", m_filename);
            LogPrintf("%s: File format is unknown or invalid, please fix it manually\n", __func__);
            return false;

    }
    return true;
}

template <typename T>
bool DbManager<T>::Dump(const T& objToSave)
{
    int64_t nStart = GetTimeMillis();

    LogPrintf("Verifying %s format...\n", m_filename);
    T tmpObjToLoad;
    if (!Load(tmpObjToLoad, true))
    {
        return false;
    }
    LogPrintf("Writing info to %s...\n", m_filename);
    Write(objToSave);
    LogPrintf("%s dump finished  %dms\n", m_filename, GetTimeMillis() - nStart);

    return true;
}

#endif
