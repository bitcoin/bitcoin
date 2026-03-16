// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/fs.h>
#include <util/syserror.h>

#ifndef WIN32
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/utsname.h>
#include <unistd.h>
#else
#include <limits>
#include <windows.h>
#endif

#include <cassert>
#include <cerrno>
#include <string>

namespace fsbridge {

FILE *fopen(const fs::path& p, const char *mode)
{
#ifndef WIN32
    return ::fopen(p.c_str(), mode);
#else
    return ::fopen(p.utf8string().c_str(), mode);
#endif
}

fs::path AbsPathJoin(const fs::path& base, const fs::path& path)
{
    assert(base.is_absolute());
    return path.empty() ? base : fs::path(base / path);
}

#ifndef WIN32

static std::string GetErrorReason()
{
    return SysErrorString(errno);
}

FileLock::FileLock(const fs::path& file)
{
    fd = open(file.c_str(), O_RDWR);
    if (fd == -1) {
        reason = GetErrorReason();
    }
}

FileLock::~FileLock()
{
    if (fd != -1) {
        close(fd);
    }
}

bool FileLock::TryLock()
{
    if (fd == -1) {
        return false;
    }

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl(fd, F_SETLK, &lock) == -1) {
        reason = GetErrorReason();
        return false;
    }

    return true;
}
#else

static std::string GetErrorReason() {
    return Win32ErrorString(GetLastError());
}

FileLock::FileLock(const fs::path& file)
{
    hFile = CreateFileW(file.wstring().c_str(),  GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        reason = GetErrorReason();
    }
}

FileLock::~FileLock()
{
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}

bool FileLock::TryLock()
{
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    _OVERLAPPED overlapped = {};
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, std::numeric_limits<DWORD>::max(), std::numeric_limits<DWORD>::max(), &overlapped)) {
        reason = GetErrorReason();
        return false;
    }
    return true;
}
#endif

} // namespace fsbridge
