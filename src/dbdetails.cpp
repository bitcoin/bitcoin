// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbdetails.h"

namespace details
{
    ReadResult ReadStream(CDataStream& stream, const std::string& filename)
    {
        boost::filesystem::path pathDb = GetDataDir() / filename;
        // open input file, and associate with CAutoFile
        FILE *file = fopen(pathDb.string().c_str(), "rb");
        CAutoFile filein(file, SER_DISK, CLIENT_VERSION);
        if (filein.IsNull())
        {
            error("%s: Failed to open file %s", __func__, pathDb.string());
            return FileError;
        }
        // use file size to size memory buffer
        int fileSize = boost::filesystem::file_size(pathDb);
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
        catch (const std::exception &e)
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
}
