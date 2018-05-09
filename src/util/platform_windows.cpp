// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <platform_util.h>

bool RenameOver(fs::path src, fs::path dest)
{
  return MoveFileExA(src.string().c_str(), dest.string().c_str(),
                     MOVEFILE_REPLACE_EXISTING) != 0;
}

bool FileCommit(FILE *file)
{
    //-- This is now repeated in platform_posix.cpp
    if (fflush(file) != 0) { // harmless if redundantly called
        LogPrintf("%s: fflush failed: %d\n", __func__, errno);
        return false;
    }
    //--

    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    if (FlushFileBuffers(hFile) == 0) {
        LogPrintf("%s: FlushFileBuffers failed: %d\n", __func__, GetLastError());
        return false;
    }
    return true;
}
