// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FS_H
#define BITCOIN_FS_H

#include <stdio.h>
#include <string>
#if defined WIN32 && defined __GLIBCXX__
#include <ext/stdio_filebuf.h>
#endif

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

/** Filesystem operations and types */
namespace fs {

using namespace boost::filesystem;

/**
 * Path class wrapper to prepare application code for transition from
 * boost::filesystem library to std::filesystem implementation. The main
 * purpose of the class is to define fs::path::u8string() and fs::u8path()
 * functions not present in boost. In the next git commit, it also blocks calls
 * to the fs::path(std::string) implicit constructor and the fs::path::string()
 * method, which worked well in the boost::filesystem implementation, but have
 * unsafe and unpredictable behavior on Windows in the std::filesystem
 * implementation (see implementation note in \ref PathToString for details).
 */
class path : public boost::filesystem::path
{
public:
    using boost::filesystem::path::path;
    path(boost::filesystem::path path) : boost::filesystem::path::path(std::move(path)) {}

    // Define UTF-8 string conversion method not present in boost::filesystem but present in std::filesystem.
    std::string u8string() const { return boost::filesystem::path::string(); }
};

// Define UTF-8 string conversion function not present in boost::filesystem but present in std::filesystem.
static inline path u8path(const std::string& string)
{
    return boost::filesystem::path(string);
}

/**
 * Convert path object to byte string. On POSIX, paths natively are byte
 * strings so this is trivial. On Windows, paths natively are Unicode, so an
 * encoding step is necessary.
 *
 * The inverse of \ref PathToString is \ref PathFromString. The strings
 * returned and parsed by these functions can be used to call POSIX APIs, and
 * for roundtrip conversion, logging, and debugging. But they are not
 * guaranteed to be valid UTF-8, and are generally meant to be used internally,
 * not externally. When communicating with external programs and libraries that
 * require UTF-8, fs::path::u8string() and fs::u8path() methods can be used.
 * For other applications, if support for non UTF-8 paths is required, or if
 * higher-level JSON or XML or URI or C-style escapes are preferred, it may be
 * also be appropriate to use different path encoding functions.
 *
 * Implementation note: On Windows, the std::filesystem::path(string)
 * constructor and std::filesystem::path::string() method are not safe to use
 * here, because these methods encode the path using C++'s narrow multibyte
 * encoding, which on Windows corresponds to the current "code page", which is
 * unpredictable and typically not able to represent all valid paths. So
 * std::filesystem::path::u8string() and std::filesystem::u8path() functions
 * are used instead on Windows. On POSIX, u8string/u8path functions are not
 * safe to use because paths are not always valid UTF-8, so plain string
 * methods which do not transform the path there are used.
 */
static inline std::string PathToString(const path& path)
{
#ifdef WIN32
    return path.u8string();
#else
    static_assert(std::is_same<path::string_type, std::string>::value, "PathToString not implemented on this platform");
    return path.boost::filesystem::path::string();
#endif
}

/**
 * Convert byte string to path object. Inverse of \ref PathToString.
 */
static inline path PathFromString(const std::string& string)
{
#ifdef WIN32
    return u8path(string);
#else
    return boost::filesystem::path(string);
#endif
}
} // namespace fs

/** Bridge operations to C stdio */
namespace fsbridge {
    FILE *fopen(const fs::path& p, const char *mode);

    /**
     * Helper function for joining two paths
     *
     * @param[in] base  Base path
     * @param[in] path  Path to combine with base
     * @returns path unchanged if it is an absolute path, otherwise returns base joined with path. Returns base unchanged if path is empty.
     * @pre  Base path must be absolute
     * @post Returned path will always be absolute
     */
    fs::path AbsPathJoin(const fs::path& base, const fs::path& path);

    class FileLock
    {
    public:
        FileLock() = delete;
        FileLock(const FileLock&) = delete;
        FileLock(FileLock&&) = delete;
        explicit FileLock(const fs::path& file);
        ~FileLock();
        bool TryLock();
        std::string GetReason() { return reason; }

    private:
        std::string reason;
#ifndef WIN32
        int fd = -1;
#else
        void* hFile = (void*)-1; // INVALID_HANDLE_VALUE
#endif
    };

    std::string get_filesystem_error_message(const fs::filesystem_error& e);

    // GNU libstdc++ specific workaround for opening UTF-8 paths on Windows.
    //
    // On Windows, it is only possible to reliably access multibyte file paths through
    // `wchar_t` APIs, not `char` APIs. But because the C++ standard doesn't
    // require ifstream/ofstream `wchar_t` constructors, and the GNU library doesn't
    // provide them (in contrast to the Microsoft C++ library, see
    // https://stackoverflow.com/questions/821873/how-to-open-an-stdfstream-ofstream-or-ifstream-with-a-unicode-filename/822032#822032),
    // Boost is forced to fall back to `char` constructors which may not work properly.
    //
    // Work around this issue by creating stream objects with `_wfopen` in
    // combination with `__gnu_cxx::stdio_filebuf`. This workaround can be removed
    // with an upgrade to C++17, where streams can be constructed directly from
    // `std::filesystem::path` objects.

#if defined WIN32 && defined __GLIBCXX__
    class ifstream : public std::istream
    {
    public:
        ifstream() = default;
        explicit ifstream(const fs::path& p, std::ios_base::openmode mode = std::ios_base::in) { open(p, mode); }
        ~ifstream() { close(); }
        void open(const fs::path& p, std::ios_base::openmode mode = std::ios_base::in);
        bool is_open() { return m_filebuf.is_open(); }
        void close();

    private:
        __gnu_cxx::stdio_filebuf<char> m_filebuf;
        FILE* m_file = nullptr;
    };
    class ofstream : public std::ostream
    {
    public:
        ofstream() = default;
        explicit ofstream(const fs::path& p, std::ios_base::openmode mode = std::ios_base::out) { open(p, mode); }
        ~ofstream() { close(); }
        void open(const fs::path& p, std::ios_base::openmode mode = std::ios_base::out);
        bool is_open() { return m_filebuf.is_open(); }
        void close();

    private:
        __gnu_cxx::stdio_filebuf<char> m_filebuf;
        FILE* m_file = nullptr;
    };
#else  // !(WIN32 && __GLIBCXX__)
    typedef fs::ifstream ifstream;
    typedef fs::ofstream ofstream;
#endif // WIN32 && __GLIBCXX__
};

#endif // BITCOIN_FS_H
