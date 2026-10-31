// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/fs.h>

#include <util/check.h>
#include <util/syserror.h>

#include <cerrno>
#include <string>

#if !defined(__linux__) && !defined(WIN32)
#include <cstring>
#endif

#ifndef WIN32
#include <fcntl.h>
#include <unistd.h>
#else
#include <windows.h>

#include <limits>
#endif

namespace fsbridge {

FILE *fopen(const fs::path& p, const char *mode)
{
#if defined(__linux__)
    const std::string safe_mode{std::string(mode) + "e"};
    return ::fopen(p.c_str(), safe_mode.c_str());
#elif defined(WIN32)
    const std::string safe_mode{std::string(mode) + "N"};
    return ::fopen(p.utf8string().c_str(), safe_mode.c_str());
#else
    const bool read_write{std::strchr(mode, '+') != nullptr};
    int flags{O_CLOEXEC};

    switch (mode[0]) {
        case 'r':
            flags |= read_write ? O_RDWR : O_RDONLY;
            break;
        case 'w':
            flags |= (read_write ? O_RDWR :  O_WRONLY) | O_CREAT | O_TRUNC;
            if(std::strchr(mode, 'x')) flags |= O_EXCL;
            break;
        case 'a':
            flags |= (read_write ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
            break;
        default:
            errno = EINVAL;
            return nullptr;
    }

    const int fd{::open(p.c_str(), flags, 0666)};
    if (fd == -1) return nullptr;

    // Match fopen's initial position for append.
    if (mode[0] == 'a') {
        (void)::lseek(fd, 0, SEEK_END);
    }

    FILE* file{::fdopen(fd, mode)};
    if (!file) {
        const int saved_errno{errno};
        ::close(fd);
        errno = saved_errno;
    }

    return file;
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
