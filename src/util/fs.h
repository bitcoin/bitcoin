// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_FS_H
#define BITCOIN_UTIL_FS_H

#include <tinyformat.h>

#include <cstdio>
#include <filesystem> // IWYU pragma: export
#include <functional>
#include <iomanip>
#include <ios>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

/** Filesystem operations and types */
namespace fs {

using namespace std::filesystem;

/**
 * Path class wrapper to block calls to the fs::path(std::string) implicit
 * constructor and the fs::path::string() method, which have unsafe and
 * unpredictable behavior on Windows (see implementation note in
 * \ref PathToString for details)
 */
class path : public std::filesystem::path
{
public:
    using std::filesystem::path::path;

    // Convenience method for accessing standard path type without needing a cast.
    std::filesystem::path& std_path() { return *this; }
    const std::filesystem::path& std_path() const { return *this; }

    // Allow path objects arguments for compatibility.
    path(std::filesystem::path path) : std::filesystem::path::path(std::move(path)) {}
    path& operator=(std::filesystem::path path) { std::filesystem::path::operator=(std::move(path)); return *this; }
    path& operator/=(const std::filesystem::path& path) { std::filesystem::path::operator/=(path); return *this; }

    // Allow literal string arguments, which are safe as long as the literals are ASCII.
    path(const char* c) : std::filesystem::path(c) {}
    path& operator=(const char* c) { std::filesystem::path::operator=(c); return *this; }
    path& operator/=(const char* c) { std::filesystem::path::operator/=(c); return *this; }
    path& append(const char* c) { std::filesystem::path::append(c); return *this; }

    // Disallow std::string arguments to avoid locale-dependent decoding on windows.
    path(std::string) = delete;
    path& operator=(std::string) = delete;
    path& operator/=(std::string) = delete;
    path& append(std::string) = delete;

    // Disallow std::string conversion method to avoid locale-dependent encoding on windows.
    std::string string() const = delete;

    // Disallow implicit string conversion to ensure code is portable.
    // `string_type` may be `string` or `wstring` depending on the platform, so
    // using this conversion could result in code that compiles on unix but
    // fails to compile on windows, or vice versa.
    operator string_type() const = delete;

    /**
     * Return a UTF-8 representation of the path as a std::string, for
     * compatibility with code using std::string. For code using the newer
     * std::u8string type, it is more efficient to call the inherited
     * std::filesystem::path::u8string method instead.
     */
    std::string utf8string() const
    {
        const std::u8string& utf8_str{std::filesystem::path::u8string()};
        return std::string{utf8_str.begin(), utf8_str.end()};
    }
};

static inline path u8path(std::string_view utf8_str)
{
    return std::filesystem::path(std::u8string{utf8_str.begin(), utf8_str.end()});
}

// Disallow implicit std::string conversion for absolute to avoid
// locale-dependent encoding on windows.
static inline path absolute(const path& p)
{
    return std::filesystem::absolute(p);
}

// Disallow implicit std::string conversion for exists to avoid
// locale-dependent encoding on windows.
static inline bool exists(const path& p)
{
    return std::filesystem::exists(p);
}

// Allow explicit quoted stream I/O.
static inline auto quoted(const std::string& s)
{
    return std::quoted(s, '"', '&');
}

// Allow safe path append operations.
static inline path operator/(path p1, const path& p2)
{
    p1 /= p2;
    return p1;
}
static inline path operator/(path p1, const char* p2)
{
    p1 /= p2;
    return p1;
}
static inline path operator+(path p1, const char* p2)
{
    p1 += p2;
    return p1;
}
static inline path operator+(path p1, path::value_type p2)
{
    p1 += p2;
    return p1;
}

// Disallow unsafe path append operations.
template<typename T> static inline path operator/(path p1, T p2) = delete;
template<typename T> static inline path operator+(path p1, T p2) = delete;

// Disallow implicit std::string conversion for copy_file
// to avoid locale-dependent encoding on Windows.
static inline bool copy_file(const path& from, const path& to, copy_options options)
{
    return std::filesystem::copy_file(from, to, options);
}

/**
 * Convert path object to a byte string. On POSIX, paths natively are byte
 * strings, so this is trivial. On Windows, paths natively are Unicode, so an
 * encoding step is necessary. The inverse of \ref PathToString is \ref
 * PathFromString. The strings returned and parsed by these functions can be
 * used to call POSIX APIs, and for roundtrip conversion, logging, and
 * debugging.
 *
 * Because \ref PathToString and \ref PathFromString functions don't specify an
 * encoding, they are meant to be used internally, not externally. They are not
 * appropriate to use in applications requiring UTF-8, where
 * fs::path::u8string() / fs::path::utf8string() and fs::u8path() methods should be used instead. Other
 * applications could require still different encodings. For example, JSON, XML,
 * or URI applications might prefer to use higher-level escapes (\uXXXX or
 * &XXXX; or %XX) instead of multibyte encoding. Rust, Python, Java applications
 * may require encoding paths with their respective UTF-8 derivatives WTF-8,
 * PEP-383, and CESU-8 (see https://en.wikipedia.org/wiki/UTF-8#Derivatives).
 */
static inline std::string PathToString(const path& path)
{
    // Implementation note: On Windows, the std::filesystem::path(string)
    // constructor and std::filesystem::path::string() method are not safe to
    // use here, because these methods encode the path using C++'s narrow
    // multibyte encoding, which on Windows corresponds to the current "code
    // page", which is unpredictable and typically not able to represent all
    // valid paths. So fs::path::utf8string() and
    // fs::u8path() functions are used instead on Windows. On
    // POSIX, u8string/utf8string/u8path functions are not safe to use because paths are
    // not always valid UTF-8, so plain string methods which do not transform
    // the path there are used.
#ifdef WIN32
    return path.utf8string();
#else
    static_assert(std::is_same_v<path::string_type, std::string>, "PathToString not implemented on this platform");
    return path.std::filesystem::path::string();
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
    return std::filesystem::path(string);
#endif
}

/**
 * Create directory (and if necessary its parents), unless the leaf directory
 * already exists or is a symlink to an existing directory.
 * This is a temporary workaround for an issue in libstdc++ that has been fixed
 * upstream [PR101510].
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101510
 */
static inline bool create_directories(const std::filesystem::path& p)
{
    if (std::filesystem::is_symlink(p) && std::filesystem::is_directory(p)) {
        return false;
    }
    return std::filesystem::create_directories(p);
}

/**
 * This variant is not used. Delete it to prevent it from accidentally working
 * around the workaround. If it is needed, add a workaround in the same pattern
 * as above.
 */
bool create_directories(const std::filesystem::path& p, std::error_code& ec) = delete;

} // namespace fs

/** Bridge operations to C stdio */
namespace fsbridge {
    using FopenFn = std::function<FILE*(const fs::path&, const char*)>;
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
};

// Disallow path operator<< formatting in tinyformat to avoid locale-dependent
// encoding on windows.
namespace tinyformat {
template<> inline void formatValue(std::ostream&, const char*, const char*, int, const std::filesystem::path&) = delete;
template<> inline void formatValue(std::ostream&, const char*, const char*, int, const fs::path&) = delete;
} // namespace tinyformat

#endif // BITCOIN_UTIL_FS_H
