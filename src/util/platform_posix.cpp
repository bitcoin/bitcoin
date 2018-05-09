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

bool IsSwitchChar(char c)
{
  return c == '-';
}

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

bool TruncateFile(FILE *file, unsigned int length) {
  return ftruncate(fileno(file), length) == 0;
}

int RaiseFileDescriptorLimit(int nMinFD) {
  struct rlimit limitFD;
  if (getrlimit(RLIMIT_NOFILE, &limitFD) != -1) {
      if (limitFD.rlim_cur < (rlim_t)nMinFD) {
          limitFD.rlim_cur = nMinFD;
          if (limitFD.rlim_cur > limitFD.rlim_max)
              limitFD.rlim_cur = limitFD.rlim_max;
          setrlimit(RLIMIT_NOFILE, &limitFD);
          getrlimit(RLIMIT_NOFILE, &limitFD);
      }
      return limitFD.rlim_cur;
  }
  return nMinFD; // getrlimit failed, assume it's fine
}

/**
 * this function tries to make a particular range of a file allocated (corresponding to disk space)
 * it is advisory, and the range specified in the arguments will never contain live data
 */
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length) {
// TODO: We need to combine the MAC OSX, Linux, and Fallback
#if defined(MAC_OSX)
    // OSX specific version
    fstore_t fst;
    fst.fst_flags = F_ALLOCATECONTIG;
    fst.fst_posmode = F_PEOFPOSMODE;
    fst.fst_offset = 0;
    fst.fst_length = (off_t)offset + length;
    fst.fst_bytesalloc = 0;
    if (fcntl(fileno(file), F_PREALLOCATE, &fst) == -1) {
        fst.fst_flags = F_ALLOCATEALL;
        fcntl(fileno(file), F_PREALLOCATE, &fst);
    }
    ftruncate(fileno(file), fst.fst_length);
#elif defined(__linux__)
    // Version using posix_fallocate
    off_t nEndPos = (off_t)offset + length;
    posix_fallocate(fileno(file), 0, nEndPos);
#else
    // Fallback version
    // TODO: just write one byte per block
    static const char buf[65536] = {};
    if (fseek(file, offset, SEEK_SET)) {
        return;
    }
    while (length > 0) {
        unsigned int now = 65536;
        if (length < now)
            now = length;
        fwrite(buf, 1, now, file); // allowed to fail; this function is advisory anyway
        length -= now;
    }
#endif
}

bool SetupNetworking()
{
  return true;
}
