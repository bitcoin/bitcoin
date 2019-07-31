#include <fs.h>

#ifndef WIN32
#include <fcntl.h>
#else
#define NOMINMAX
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
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, std::numeric_limits<DWORD>::max(), std::numeric_limits<DWORD>::max(), &overlapped)) {
        reason = GetErrorReason();
        return false;
    }
    return true;
}
#endif

std::string get_filesystem_error_message(const fs::filesystem_error& e)
{
#ifndef WIN32
    return e.what();
#else
    // Convert from Multi Byte to utf-16
    std::string mb_string(e.what());
    int size = MultiByteToWideChar(CP_ACP, 0, mb_string.c_str(), mb_string.size(), nullptr, 0);

    std::wstring utf16_string(size, L'\0');
    MultiByteToWideChar(CP_ACP, 0, mb_string.c_str(), mb_string.size(), &*utf16_string.begin(), size);
    // Convert from utf-16 to utf-8
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(utf16_string);
#endif
}

#ifdef WIN32
#ifdef __GLIBCXX__

// reference: https://github.com/gcc-mirror/gcc/blob/gcc-7_3_0-release/libstdc%2B%2B-v3/include/std/fstream#L270

static std::string openmodeToStr(std::ios_base::openmode mode)
{
    switch (mode & ~std::ios_base::ate) {
    case std::ios_base::out:
    case std::ios_base::out | std::ios_base::trunc:
        return "w";
    case std::ios_base::out | std::ios_base::app:
    case std::ios_base::app:
        return "a";
    case std::ios_base::in:
        return "r";
    case std::ios_base::in | std::ios_base::out:
        return "r+";
    case std::ios_base::in | std::ios_base::out | std::ios_base::trunc:
        return "w+";
    case std::ios_base::in | std::ios_base::out | std::ios_base::app:
    case std::ios_base::in | std::ios_base::app:
        return "a+";
    case std::ios_base::out | std::ios_base::binary:
    case std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
        return "wb";
    case std::ios_base::out | std::ios_base::app | std::ios_base::binary:
    case std::ios_base::app | std::ios_base::binary:
        return "ab";
    case std::ios_base::in | std::ios_base::binary:
        return "rb";
    case std::ios_base::in | std::ios_base::out | std::ios_base::binary:
        return "r+b";
    case std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
        return "w+b";
    case std::ios_base::in | std::ios_base::out | std::ios_base::app | std::ios_base::binary:
    case std::ios_base::in | std::ios_base::app | std::ios_base::binary:
        return "a+b";
    default:
        return std::string();
    }
}

void ifstream::open(const fs::path& p, std::ios_base::openmode mode)
{
    close();
    mode |= std::ios_base::in;
    m_file = fsbridge::fopen(p, openmodeToStr(mode).c_str());
    if (m_file == nullptr) {
        return;
    }
    m_filebuf = __gnu_cxx::stdio_filebuf<char>(m_file, mode);
    rdbuf(&m_filebuf);
    if (mode & std::ios_base::ate) {
        seekg(0, std::ios_base::end);
    }
}

void ifstream::close()
{
    if (m_file != nullptr) {
        m_filebuf.close();
        fclose(m_file);
    }
    m_file = nullptr;
}

void ofstream::open(const fs::path& p, std::ios_base::openmode mode)
{
    close();
    mode |= std::ios_base::out;
    m_file = fsbridge::fopen(p, openmodeToStr(mode).c_str());
    if (m_file == nullptr) {
        return;
    }
    m_filebuf = __gnu_cxx::stdio_filebuf<char>(m_file, mode);
    rdbuf(&m_filebuf);
    if (mode & std::ios_base::ate) {
        seekp(0, std::ios_base::end);
    }
}

void ofstream::close()
{
    if (m_file != nullptr) {
        m_filebuf.close();
        fclose(m_file);
    }
    m_file = nullptr;
}
#else // __GLIBCXX__

static_assert(sizeof(*fs::path().BOOST_FILESYSTEM_C_STR) == sizeof(wchar_t),
    "Warning: This build is using boost::filesystem ofstream and ifstream "
    "implementations which will fail to open paths containing multibyte "
    "characters. You should delete this static_assert to ignore this warning, "
    "or switch to a different C++ standard library like the Microsoft C++ "
    "Standard Library (where boost uses non-standard extensions to construct "
    "stream objects with wide filenames), or the GNU libstdc++ library (where "
    "a more complicated workaround has been implemented above).");

#endif // __GLIBCXX__
#endif // WIN32

} // fsbridge
