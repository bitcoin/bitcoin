// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <platform_util.h>

bool RenameOver(fs::path src, fs::path dest)
{
  int rc = std::rename(src.string().c_str(), dest.string().c_str());
  return (rc == 0);
}

bool FileCommit(FILE *file)
{
  // This is now repeated in platform_windows.cpp
  if (fflush(file) != 0) { // harmless if redundantly called
      LogPrintf("%s: fflush failed: %d\n", __func__, errno);
      return false;
  }
  //--

  #if defined(__linux__) || defined(__NetBSD__)
  if (fdatasync(fileno(file)) != 0 && errno != EINVAL) { // Ignore EINVAL for filesystems that don't support sync
      LogPrintf("%s: fdatasync failed: %d\n", __func__, errno);
      return false;
  }
  #elif defined(__APPLE__) && defined(F_FULLFSYNC)
  if (fcntl(fileno(file), F_FULLFSYNC, 0) == -1) { // Manpage says "value other than -1" is returned on success
      LogPrintf("%s: fcntl F_FULLFSYNC failed: %d\n", __func__, errno);
      return false;
  }
  #else
  if (fsync(fileno(file)) != 0 && errno != EINVAL) {
      LogPrintf("%s: fsync failed: %d\n", __func__, errno);
      return false;
  }
  #endif

  return true;
}
