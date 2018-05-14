// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "platform_common.h"

#include <chainparamsbase.h>
#include <random.h>
#include <serialize.h>
#include <utilstrencodings.h>

#include <stdarg.h>

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

#ifndef WIN32
// for posix_fallocate
#ifdef __linux__

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#define _POSIX_C_SOURCE 200112L

#endif // __linux__

#include <algorithm>
#include <fcntl.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/stat.h>

#else

#ifdef _MSC_VER
#pragma warning(disable:4786)
#pragma warning(disable:4804)
#pragma warning(disable:4805)
#pragma warning(disable:4717)
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501

#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <io.h> /* for _commit */
#include <shlobj.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_MALLOPT_ARENA_MAX
#include <malloc.h>
#endif

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/program_options/detail/config_file.hpp>
#include <boost/thread.hpp>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <thread>

#ifdef WIN32

fs::path GetPidFile();
void CreatePidFile(const fs::path &path, pid_t pid);
fs::path GetSpecialFolderPath(int nFolder, bool fCreate = true);

bool IsSwitchChar(char c)
{
  return c == '-' || c == '/';
}

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

bool TruncateFile(FILE *file, unsigned int length) {
  return _chsize(_fileno(file), length) == 0;
}

int RaiseFileDescriptorLimit(int nMinFD) {
  return 2048;
}

/**
 * this function tries to make a particular range of a file allocated (corresponding to disk space)
 * it is advisory, and the range specified in the arguments will never contain live data
 */
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length) {
  // Windows-specific version
  HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
  LARGE_INTEGER nFileSize;
  int64_t nEndPos = (int64_t)offset + length;
  nFileSize.u.LowPart = nEndPos & 0xFFFFFFFF;
  nFileSize.u.HighPart = nEndPos >> 32;
  SetFilePointerEx(hFile, nFileSize, 0, FILE_BEGIN);
  SetEndOfFile(hFile);
}

fs::path GetSpecialFolderPath(int nFolder, bool fCreate)
{
  char pszPath[MAX_PATH] = "";

  if(SHGetSpecialFolderPathA(nullptr, pszPath, nFolder, fCreate))
  {
      return fs::path(pszPath);
  }

  LogPrintf("SHGetSpecialFolderPathA() failed, could not obtain requested path.\n");
  return fs::path("");
}

bool SetupNetworking()
{
  // Initialize Windows Sockets
  WSADATA wsadata;
  int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
  if (ret != NO_ERROR || LOBYTE(wsadata.wVersion ) != 2 || HIBYTE(wsadata.wVersion) != 2)
      return false;
  return true;
}
#endif
