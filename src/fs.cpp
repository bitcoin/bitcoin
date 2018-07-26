#include <fs.h>

#ifndef WIN32
#include <fcntl.h>
#else
#include <codecvt>
#include <windows.h>
#endif

namespace fsbridge {

FILE *fopen(const fs::path& p, const char *mode)
{
#ifndef WIN32
    return ::fopen(p.string().c_str(), mode);
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> utf8_cvt;
    return ::_wfopen(p.wstring().c_str(), utf8_cvt.from_bytes(mode).c_str());
#endif
}

#ifndef WIN32

static std::string GetErrorReason() {
    return std::strerror(errno);
}

FileLock::FileLock(const fs::path& file)
{
    fd = open(file.string().c_str(), O_RDWR);
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
    wchar_t* err;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<WCHAR*>(&err), 0, nullptr);
    std::wstring err_str(err);
    LocalFree(err);
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(err_str);
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
    _OVERLAPPED overlapped = {0};
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 0, 0, &overlapped)) {
        reason = GetErrorReason();
        return false;
    }
    return true;
}
#endif

} // fsbridge
