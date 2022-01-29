//---------------------------------------------------------------------------------------
//
// ghc::filesystem - A C++17-like filesystem implementation for C++11/C++14
//
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2018, Steffen Schümann <s.schuemann@pobox.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//---------------------------------------------------------------------------------------
//
// To dynamically select std::filesystem where available, you could use:
//
// #if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include) && __has_include(<filesystem>)
// #include <filesystem>
// namespace fs = std::filesystem;
// #else
// #include <ghc/filesystem.hpp>
// namespace fs = ghc::filesystem;
// #endif
//
//---------------------------------------------------------------------------------------
#ifndef GHC_FILESYSTEM_H
#define GHC_FILESYSTEM_H

#if defined(__APPLE__) && defined(__MACH__)
#define GHC_OS_MACOS
#elif defined(__linux__)
#define GHC_OS_LINUX
#elif defined(_WIN64)
#define GHC_OS_WINDOWS
#define GHC_OS_WIN64
#elif defined(_WIN32)
#define GHC_OS_WINDOWS
#define GHC_OS_WIN32
#else
#error "Operating system currently not supported!"
#endif

#if defined(GHC_FILESYSTEM_IMPLEMENTATION)
#define GHC_EXPAND_IMPL
#define GHC_INLINE
#ifdef GHC_OS_WINDOWS
#define GHC_FS_API
#define GHC_FS_API_CLASS
#else
#define GHC_FS_API __attribute__((visibility("default")))
#define GHC_FS_API_CLASS __attribute__((visibility("default")))
#endif
#elif defined(GHC_FILESYSTEM_FWD)
#define GHC_INLINE
#ifdef GHC_OS_WINDOWS
#define GHC_FS_API extern
#define GHC_FS_API_CLASS
#else
#define GHC_FS_API extern
#define GHC_FS_API_CLASS
#endif
#else
#define GHC_EXPAND_IMPL
#define GHC_INLINE inline
#define GHC_FS_API
#define GHC_FS_API_CLASS
#endif

#ifdef GHC_EXPAND_IMPL

#ifdef GHC_OS_WINDOWS
#include <windows.h>
// additional includes
#include <shellapi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wchar.h>
#include <winioctl.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <langinfo.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(__ANDROID__)
#define GHC_OS_ANDROID
#include <android/api-level.h>
#endif
#endif
#ifdef GHC_OS_MACOS
#include <Availability.h>
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#else  // GHC_EXPAND_IMPL
#include <chrono>
#include <fstream>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <system_error>
#endif  // GHC_EXPAND_IMPL

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Behaviour Switches (see README.md, should match the config in test/filesystem_test.cpp):
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LWG #2682 disables the since then invalid use of the copy option create_symlinks on directories
// configure LWG conformance ()
#define LWG_2682_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LWG #2395 makes crate_directory/create_directories not emit an error if there is a regular
// file with that name, it is superceded by P1164R1, so only activate if really needed
// #define LWG_2935_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LWG #2937 enforces that fs::equivalent emits an error, if !fs::exists(p1)||!exists(p2)
#define LWG_2937_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// ghc::filesystem version in decimal (major * 10000 + minor * 100 + patch)
#define GHC_FILESYSTEM_VERSION 10105L

namespace ghc {
namespace filesystem {

// temporary existing exception type for yet unimplemented parts
class GHC_FS_API_CLASS not_implemented_exception : public std::logic_error
{
public:
    not_implemented_exception()
        : std::logic_error("function not implemented yet.")
    {
    }
};

// 30.10.8 class path
class GHC_FS_API_CLASS path
{
public:
    using value_type = std::string::value_type;
    using string_type = std::basic_string<value_type>;
#ifdef GHC_OS_WINDOWS
    static constexpr value_type preferred_separator = '\\';
#else
    static constexpr value_type preferred_separator = '/';
#endif
    // 30.10.10.1 enumeration format
    /// The path format in wich the constructor argument is given.
    enum format {
        generic_format,  ///< The generic format, internally used by
                         ///< ghc::filesystem::path with slashes
        native_format,   ///< The format native to the current platform this code
                         ///< is build for
        auto_format,     ///< Try to auto-detect the format, fallback to native
    };

    template <class T>
    struct _is_basic_string : std::false_type
    {
    };
    template <class CharT, class Traits, class Alloc>
    struct _is_basic_string<std::basic_string<CharT, Traits, Alloc>> : std::true_type
    {
    };
#ifdef __cpp_lib_string_view
    template <class CharT>
    struct _is_basic_string<std::basic_string_view<CharT>> : std::true_type
    {
    };
#endif

    template <typename T1, typename T2 = void>
    using path_type = typename std::enable_if<!std::is_same<path, T1>::value, path>::type;
    template <typename T>
    using path_from_string = typename std::enable_if<_is_basic_string<T>::value || std::is_same<char const*, typename std::decay<T>::type>::value || std::is_same<char*, typename std::decay<T>::type>::value, path>::type;
    template <typename T>
    using path_type_EcharT = typename std::enable_if<std::is_same<T, char>::value || std::is_same<T, char16_t>::value || std::is_same<T, char32_t>::value || std::is_same<T, wchar_t>::value, path>::type;

    // 30.10.8.4.1 constructors and destructor
    path() noexcept;
    path(const path& p);
    path(path&& p) noexcept;
    path(string_type&& source, format fmt = auto_format);
    template <class Source, typename = path_from_string<Source>>
    path(const Source& source, format fmt = auto_format);
    template <class InputIterator>
    path(InputIterator first, InputIterator last, format fmt = auto_format);
    template <class Source, typename = path_from_string<Source>>
    path(const Source& source, const std::locale& loc, format fmt = auto_format);
    template <class InputIterator>
    path(InputIterator first, InputIterator last, const std::locale& loc, format fmt = auto_format);
    ~path();

    // 30.10.8.4.2 assignments
    path& operator=(const path& p);
    path& operator=(path&& p) noexcept;
    path& operator=(string_type&& source);
    path& assign(string_type&& source);
    template <class Source>
    path& operator=(const Source& source);
    template <class Source>
    path& assign(const Source& source);
    template <class InputIterator>
    path& assign(InputIterator first, InputIterator last);

    // 30.10.8.4.3 appends
    path& operator/=(const path& p);
    template <class Source>
    path& operator/=(const Source& source);
    template <class Source>
    path& append(const Source& source);
    template <class InputIterator>
    path& append(InputIterator first, InputIterator last);

    // 30.10.8.4.4 concatenation
    path& operator+=(const path& x);
    path& operator+=(const string_type& x);
#ifdef __cpp_lib_string_view
    path& operator+=(std::basic_string_view<value_type> x);
#endif
    path& operator+=(const value_type* x);
    path& operator+=(value_type x);
    template <class Source>
    path_type<Source>& operator+=(const Source& x);
    template <class EcharT>
    path_type_EcharT<EcharT>& operator+=(EcharT x);
    template <class Source>
    path& concat(const Source& x);
    template <class InputIterator>
    path& concat(InputIterator first, InputIterator last);

    // 30.10.8.4.5 modifiers
    void clear() noexcept;
    path& make_preferred();
    path& remove_filename();
    path& replace_filename(const path& replacement);
    path& replace_extension(const path& replacement = path());
    void swap(path& rhs) noexcept;

    // 30.10.8.4.6 native format observers
    const string_type& native() const;  // this implementation doesn't support noexcept for native()
    const value_type* c_str() const;    // this implementation doesn't support noexcept for c_str()
    operator string_type() const;
    template <class EcharT, class traits = std::char_traits<EcharT>, class Allocator = std::allocator<EcharT>>
    std::basic_string<EcharT, traits, Allocator> string(const Allocator& a = Allocator()) const;
    std::string string() const;
    std::wstring wstring() const;
    std::string u8string() const;
    std::u16string u16string() const;
    std::u32string u32string() const;

    // 30.10.8.4.7 generic format observers
    template <class EcharT, class traits = std::char_traits<EcharT>, class Allocator = std::allocator<EcharT>>
    std::basic_string<EcharT, traits, Allocator> generic_string(const Allocator& a = Allocator()) const;
    const std::string& generic_string() const;  // this is different from the standard, that returns by value
    std::wstring generic_wstring() const;
    std::string generic_u8string() const;
    std::u16string generic_u16string() const;
    std::u32string generic_u32string() const;

    // 30.10.8.4.8 compare
    int compare(const path& p) const noexcept;
    int compare(const string_type& s) const;
#ifdef __cpp_lib_string_view
    int compare(std::basic_string_view<value_type> s) const;
#endif
    int compare(const value_type* s) const;

    // 30.10.8.4.9 decomposition
    path root_name() const;
    path root_directory() const;
    path root_path() const;
    path relative_path() const;
    path parent_path() const;
    path filename() const;
    path stem() const;
    path extension() const;

    // 30.10.8.4.10 query
    bool empty() const noexcept;
    bool has_root_name() const;
    bool has_root_directory() const;
    bool has_root_path() const;
    bool has_relative_path() const;
    bool has_parent_path() const;
    bool has_filename() const;
    bool has_stem() const;
    bool has_extension() const;
    bool is_absolute() const;
    bool is_relative() const;

    // 30.10.8.4.11 generation
    path lexically_normal() const;
    path lexically_relative(const path& base) const;
    path lexically_proximate(const path& base) const;

    // 30.10.8.5 iterators
    class iterator;
    using const_iterator = iterator;
    iterator begin() const;
    iterator end() const;

private:
    friend class directory_iterator;
    void append_name(const char* name);
    static constexpr value_type generic_separator = '/';
    template <typename InputIterator>
    class input_iterator_range
    {
    public:
        typedef InputIterator iterator;
        typedef InputIterator const_iterator;
        typedef typename InputIterator::difference_type difference_type;

        input_iterator_range(const InputIterator& first, const InputIterator& last)
            : _first(first)
            , _last(last)
        {
        }

        InputIterator begin() const { return _first; }
        InputIterator end() const { return _last; }

    private:
        InputIterator _first;
        InputIterator _last;
    };
    friend void swap(path& lhs, path& rhs) noexcept;
    friend size_t hash_value(const path& p) noexcept;
    string_type _path;
#ifdef GHC_OS_WINDOWS
    mutable string_type _native_cache;
#endif
};

// 30.10.8.6 path non-member functions
GHC_FS_API void swap(path& lhs, path& rhs) noexcept;
GHC_FS_API size_t hash_value(const path& p) noexcept;
GHC_FS_API bool operator==(const path& lhs, const path& rhs) noexcept;
GHC_FS_API bool operator!=(const path& lhs, const path& rhs) noexcept;
GHC_FS_API bool operator<(const path& lhs, const path& rhs) noexcept;
GHC_FS_API bool operator<=(const path& lhs, const path& rhs) noexcept;
GHC_FS_API bool operator>(const path& lhs, const path& rhs) noexcept;
GHC_FS_API bool operator>=(const path& lhs, const path& rhs) noexcept;

GHC_FS_API path operator/(const path& lhs, const path& rhs);

// 30.10.8.6.1 path inserter and extractor
template <class charT, class traits>
std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& os, const path& p);
template <class charT, class traits>
std::basic_istream<charT, traits>& operator>>(std::basic_istream<charT, traits>& is, path& p);

// 30.10.8.6.2 path factory functions
template <class Source, typename = path::path_from_string<Source>>
path u8path(const Source& source);
template <class InputIterator>
path u8path(InputIterator first, InputIterator last);

// 30.10.9 class filesystem_error
class GHC_FS_API_CLASS filesystem_error : public std::system_error
{
public:
    filesystem_error(const std::string& what_arg, std::error_code ec);
    filesystem_error(const std::string& what_arg, const path& p1, std::error_code ec);
    filesystem_error(const std::string& what_arg, const path& p1, const path& p2, std::error_code ec);
    const path& path1() const noexcept;
    const path& path2() const noexcept;
    const char* what() const noexcept override;

private:
    std::string _what_arg;
    std::error_code _ec;
    path _p1, _p2;
};

class GHC_FS_API_CLASS path::iterator
{
public:
    using value_type = const path;
    using difference_type = std::ptrdiff_t;
    using pointer = const path*;
    using reference = const path&;
    using iterator_category = std::bidirectional_iterator_tag;

    iterator();
    iterator(const string_type::const_iterator& first, const string_type::const_iterator& last, const string_type::const_iterator& pos);
    iterator& operator++();
    iterator operator++(int);
    iterator& operator--();
    iterator operator--(int);
    bool operator==(const iterator& other) const;
    bool operator!=(const iterator& other) const;
    reference operator*() const;
    pointer operator->() const;

private:
    string_type::const_iterator increment(const std::string::const_iterator& pos) const;
    string_type::const_iterator decrement(const std::string::const_iterator& pos) const;
    void updateCurrent();
    string_type::const_iterator _first;
    string_type::const_iterator _last;
    string_type::const_iterator _root;
    string_type::const_iterator _iter;
    path _current;
};

struct space_info
{
    uintmax_t capacity;
    uintmax_t free;
    uintmax_t available;
};

// 30.10.10, enumerations
enum class file_type {
    none,
    not_found,
    regular,
    directory,
    symlink,
    block,
    character,
    fifo,
    socket,
    unknown,
};

enum class perms : uint16_t {
    none = 0,

    owner_read = 0400,
    owner_write = 0200,
    owner_exec = 0100,
    owner_all = 0700,

    group_read = 040,
    group_write = 020,
    group_exec = 010,
    group_all = 070,

    others_read = 04,
    others_write = 02,
    others_exec = 01,
    others_all = 07,

    all = 0777,
    set_uid = 04000,
    set_gid = 02000,
    sticky_bit = 01000,

    mask = 07777,
    unknown = 0xffff
};

enum class perm_options : uint16_t {
    replace = 3,
    add = 1,
    remove = 2,
    nofollow = 4,
};

enum class copy_options : uint16_t {
    none = 0,

    skip_existing = 1,
    overwrite_existing = 2,
    update_existing = 4,

    recursive = 8,

    copy_symlinks = 0x10,
    skip_symlinks = 0x20,

    directories_only = 0x40,
    create_symlinks = 0x80,
    create_hard_links = 0x100
};

enum class directory_options : uint16_t {
    none = 0,
    follow_directory_symlink = 1,
    skip_permission_denied = 2,
};

// 30.10.11 class file_status
class GHC_FS_API_CLASS file_status
{
public:
    // 30.10.11.1 constructors and destructor
    file_status() noexcept;
    explicit file_status(file_type ft, perms prms = perms::unknown) noexcept;
    file_status(const file_status&) noexcept;
    file_status(file_status&&) noexcept;
    ~file_status();
    // assignments:
    file_status& operator=(const file_status&) noexcept;
    file_status& operator=(file_status&&) noexcept;
    // 30.10.11.3 modifiers
    void type(file_type ft) noexcept;
    void permissions(perms prms) noexcept;
    // 30.10.11.2 observers
    file_type type() const noexcept;
    perms permissions() const noexcept;

private:
    file_type _type;
    perms _perms;
};

using file_time_type = std::chrono::time_point<std::chrono::system_clock>;

// 30.10.12 Class directory_entry
class GHC_FS_API_CLASS directory_entry
{
public:
    // 30.10.12.1 constructors and destructor
    directory_entry() noexcept = default;
    directory_entry(const directory_entry&) = default;
    directory_entry(directory_entry&&) noexcept = default;
    explicit directory_entry(const path& p);
    directory_entry(const path& p, std::error_code& ec);
    ~directory_entry();

    // assignments:
    directory_entry& operator=(const directory_entry&) = default;
    directory_entry& operator=(directory_entry&&) noexcept = default;

    // 30.10.12.2 modifiers
    void assign(const path& p);
    void assign(const path& p, std::error_code& ec);
    void replace_filename(const path& p);
    void replace_filename(const path& p, std::error_code& ec);
    void refresh();
    void refresh(std::error_code& ec) noexcept;

    // 30.10.12.3 observers
    const filesystem::path& path() const noexcept;
    operator const filesystem::path&() const noexcept;
    bool exists() const;
    bool exists(std::error_code& ec) const noexcept;
    bool is_block_file() const;
    bool is_block_file(std::error_code& ec) const noexcept;
    bool is_character_file() const;
    bool is_character_file(std::error_code& ec) const noexcept;
    bool is_directory() const;
    bool is_directory(std::error_code& ec) const noexcept;
    bool is_fifo() const;
    bool is_fifo(std::error_code& ec) const noexcept;
    bool is_other() const;
    bool is_other(std::error_code& ec) const noexcept;
    bool is_regular_file() const;
    bool is_regular_file(std::error_code& ec) const noexcept;
    bool is_socket() const;
    bool is_socket(std::error_code& ec) const noexcept;
    bool is_symlink() const;
    bool is_symlink(std::error_code& ec) const noexcept;
    uintmax_t file_size() const;
    uintmax_t file_size(std::error_code& ec) const noexcept;
    uintmax_t hard_link_count() const;
    uintmax_t hard_link_count(std::error_code& ec) const noexcept;
    file_time_type last_write_time() const;
    file_time_type last_write_time(std::error_code& ec) const noexcept;

    file_status status() const;
    file_status status(std::error_code& ec) const noexcept;

    file_status symlink_status() const;
    file_status symlink_status(std::error_code& ec) const noexcept;
    bool operator<(const directory_entry& rhs) const noexcept;
    bool operator==(const directory_entry& rhs) const noexcept;
    bool operator!=(const directory_entry& rhs) const noexcept;
    bool operator<=(const directory_entry& rhs) const noexcept;
    bool operator>(const directory_entry& rhs) const noexcept;
    bool operator>=(const directory_entry& rhs) const noexcept;

private:
    friend class directory_iterator;
    filesystem::path _path;
    file_status _status;
    file_status _symlink_status;
    uintmax_t _file_size;
#ifndef GHC_OS_WINDOWS
    uintmax_t _hard_link_count;
#endif
    time_t _last_write_time;
};

// 30.10.13 Class directory_iterator
class GHC_FS_API_CLASS directory_iterator
{
public:
    class GHC_FS_API_CLASS proxy
    {
    public:
        const directory_entry& operator*() const& noexcept { return _dir_entry; }
        directory_entry operator*() && noexcept { return std::move(_dir_entry); }

    private:
        explicit proxy(const directory_entry& dir_entry)
            : _dir_entry(dir_entry)
        {
        }
        friend class directory_iterator;
        friend class recursive_directory_iterator;
        directory_entry _dir_entry;
    };
    using iterator_category = std::input_iterator_tag;
    using value_type = directory_entry;
    using difference_type = std::ptrdiff_t;
    using pointer = const directory_entry*;
    using reference = const directory_entry&;

    // 30.10.13.1 member functions
    directory_iterator() noexcept;
    explicit directory_iterator(const path& p);
    directory_iterator(const path& p, directory_options options);
    directory_iterator(const path& p, std::error_code& ec) noexcept;
    directory_iterator(const path& p, directory_options options, std::error_code& ec) noexcept;
    directory_iterator(const directory_iterator& rhs);
    directory_iterator(directory_iterator&& rhs) noexcept;
    ~directory_iterator();
    directory_iterator& operator=(const directory_iterator& rhs);
    directory_iterator& operator=(directory_iterator&& rhs) noexcept;
    const directory_entry& operator*() const;
    const directory_entry* operator->() const;
    directory_iterator& operator++();
    directory_iterator& increment(std::error_code& ec) noexcept;

    // other members as required by 27.2.3, input iterators
    proxy operator++(int)
    {
        proxy proxy{**this};
        ++*this;
        return proxy;
    }
    bool operator==(const directory_iterator& rhs) const;
    bool operator!=(const directory_iterator& rhs) const;
    void swap(directory_iterator& rhs);

private:
    friend class recursive_directory_iterator;
    class impl;
    std::shared_ptr<impl> _impl;
};

// 30.10.13.2 directory_iterator non-member functions
GHC_FS_API directory_iterator begin(directory_iterator iter) noexcept;
GHC_FS_API directory_iterator end(const directory_iterator&) noexcept;

// 30.10.14 class recursive_directory_iterator
class GHC_FS_API_CLASS recursive_directory_iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = directory_entry;
    using difference_type = std::ptrdiff_t;
    using pointer = const directory_entry*;
    using reference = const directory_entry&;

    // 30.10.14.1 constructors and destructor
    recursive_directory_iterator() noexcept;
    explicit recursive_directory_iterator(const path& p);
    recursive_directory_iterator(const path& p, directory_options options);
    recursive_directory_iterator(const path& p, directory_options options, std::error_code& ec) noexcept;
    recursive_directory_iterator(const path& p, std::error_code& ec) noexcept;
    recursive_directory_iterator(const recursive_directory_iterator& rhs);
    recursive_directory_iterator(recursive_directory_iterator&& rhs) noexcept;
    ~recursive_directory_iterator();

    // 30.10.14.1 observers
    directory_options options() const;
    int depth() const;
    bool recursion_pending() const;

    const directory_entry& operator*() const;
    const directory_entry* operator->() const;

    // 30.10.14.1 modifiers recursive_directory_iterator&
    recursive_directory_iterator& operator=(const recursive_directory_iterator& rhs);
    recursive_directory_iterator& operator=(recursive_directory_iterator&& rhs) noexcept;
    recursive_directory_iterator& operator++();
    recursive_directory_iterator& increment(std::error_code& ec) noexcept;

    void pop();
    void pop(std::error_code& ec);
    void disable_recursion_pending();

    // other members as required by 27.2.3, input iterators
    directory_iterator::proxy operator++(int)
    {
        directory_iterator::proxy proxy{**this};
        ++*this;
        return proxy;
    }
    bool operator==(const recursive_directory_iterator& rhs) const;
    bool operator!=(const recursive_directory_iterator& rhs) const;
    void swap(recursive_directory_iterator& rhs);

private:
    struct recursive_directory_iterator_impl
    {
        directory_options _options;
        bool _recursion_pending;
        std::stack<directory_iterator> _dir_iter_stack;
        recursive_directory_iterator_impl(directory_options options, bool recursion_pending)
            : _options(options)
            , _recursion_pending(recursion_pending)
        {
        }
    };
    std::shared_ptr<recursive_directory_iterator_impl> _impl;
};

// 30.10.14.2 directory_iterator non-member functions
GHC_FS_API recursive_directory_iterator begin(recursive_directory_iterator iter) noexcept;
GHC_FS_API recursive_directory_iterator end(const recursive_directory_iterator&) noexcept;

// 30.10.15 filesystem operations
GHC_FS_API path absolute(const path& p);
GHC_FS_API path absolute(const path& p, std::error_code& ec);

GHC_FS_API path canonical(const path& p);
GHC_FS_API path canonical(const path& p, std::error_code& ec);

GHC_FS_API void copy(const path& from, const path& to);
GHC_FS_API void copy(const path& from, const path& to, std::error_code& ec) noexcept;
GHC_FS_API void copy(const path& from, const path& to, copy_options options);
GHC_FS_API void copy(const path& from, const path& to, copy_options options, std::error_code& ec) noexcept;

GHC_FS_API bool copy_file(const path& from, const path& to);
GHC_FS_API bool copy_file(const path& from, const path& to, std::error_code& ec) noexcept;
GHC_FS_API bool copy_file(const path& from, const path& to, copy_options option);
GHC_FS_API bool copy_file(const path& from, const path& to, copy_options option, std::error_code& ec) noexcept;

GHC_FS_API void copy_symlink(const path& existing_symlink, const path& new_symlink);
GHC_FS_API void copy_symlink(const path& existing_symlink, const path& new_symlink, std::error_code& ec) noexcept;

GHC_FS_API bool create_directories(const path& p);
GHC_FS_API bool create_directories(const path& p, std::error_code& ec) noexcept;

GHC_FS_API bool create_directory(const path& p);
GHC_FS_API bool create_directory(const path& p, std::error_code& ec) noexcept;

GHC_FS_API bool create_directory(const path& p, const path& attributes);
GHC_FS_API bool create_directory(const path& p, const path& attributes, std::error_code& ec) noexcept;

GHC_FS_API void create_directory_symlink(const path& to, const path& new_symlink);
GHC_FS_API void create_directory_symlink(const path& to, const path& new_symlink, std::error_code& ec) noexcept;

GHC_FS_API void create_hard_link(const path& to, const path& new_hard_link);
GHC_FS_API void create_hard_link(const path& to, const path& new_hard_link, std::error_code& ec) noexcept;

GHC_FS_API void create_symlink(const path& to, const path& new_symlink);
GHC_FS_API void create_symlink(const path& to, const path& new_symlink, std::error_code& ec) noexcept;

GHC_FS_API path current_path();
GHC_FS_API path current_path(std::error_code& ec);
GHC_FS_API void current_path(const path& p);
GHC_FS_API void current_path(const path& p, std::error_code& ec) noexcept;

GHC_FS_API bool exists(file_status s) noexcept;
GHC_FS_API bool exists(const path& p);
GHC_FS_API bool exists(const path& p, std::error_code& ec) noexcept;

GHC_FS_API bool equivalent(const path& p1, const path& p2);
GHC_FS_API bool equivalent(const path& p1, const path& p2, std::error_code& ec) noexcept;

GHC_FS_API uintmax_t file_size(const path& p);
GHC_FS_API uintmax_t file_size(const path& p, std::error_code& ec) noexcept;

GHC_FS_API uintmax_t hard_link_count(const path& p);
GHC_FS_API uintmax_t hard_link_count(const path& p, std::error_code& ec) noexcept;

GHC_FS_API bool is_block_file(file_status s) noexcept;
GHC_FS_API bool is_block_file(const path& p);
GHC_FS_API bool is_block_file(const path& p, std::error_code& ec) noexcept;
GHC_FS_API bool is_character_file(file_status s) noexcept;
GHC_FS_API bool is_character_file(const path& p);
GHC_FS_API bool is_character_file(const path& p, std::error_code& ec) noexcept;
GHC_FS_API bool is_directory(file_status s) noexcept;
GHC_FS_API bool is_directory(const path& p);
GHC_FS_API bool is_directory(const path& p, std::error_code& ec) noexcept;
GHC_FS_API bool is_empty(const path& p);
GHC_FS_API bool is_empty(const path& p, std::error_code& ec) noexcept;
GHC_FS_API bool is_fifo(file_status s) noexcept;
GHC_FS_API bool is_fifo(const path& p);
GHC_FS_API bool is_fifo(const path& p, std::error_code& ec) noexcept;
GHC_FS_API bool is_other(file_status s) noexcept;
GHC_FS_API bool is_other(const path& p);
GHC_FS_API bool is_other(const path& p, std::error_code& ec) noexcept;
GHC_FS_API bool is_regular_file(file_status s) noexcept;
GHC_FS_API bool is_regular_file(const path& p);
GHC_FS_API bool is_regular_file(const path& p, std::error_code& ec) noexcept;
GHC_FS_API bool is_socket(file_status s) noexcept;
GHC_FS_API bool is_socket(const path& p);
GHC_FS_API bool is_socket(const path& p, std::error_code& ec) noexcept;
GHC_FS_API bool is_symlink(file_status s) noexcept;
GHC_FS_API bool is_symlink(const path& p);
GHC_FS_API bool is_symlink(const path& p, std::error_code& ec) noexcept;

GHC_FS_API file_time_type last_write_time(const path& p);
GHC_FS_API file_time_type last_write_time(const path& p, std::error_code& ec) noexcept;
GHC_FS_API void last_write_time(const path& p, file_time_type new_time);
GHC_FS_API void last_write_time(const path& p, file_time_type new_time, std::error_code& ec) noexcept;

GHC_FS_API void permissions(const path& p, perms prms, perm_options opts = perm_options::replace);
GHC_FS_API void permissions(const path& p, perms prms, std::error_code& ec) noexcept;
GHC_FS_API void permissions(const path& p, perms prms, perm_options opts, std::error_code& ec);

GHC_FS_API path proximate(const path& p, std::error_code& ec);
GHC_FS_API path proximate(const path& p, const path& base = current_path());
GHC_FS_API path proximate(const path& p, const path& base, std::error_code& ec);

GHC_FS_API path read_symlink(const path& p);
GHC_FS_API path read_symlink(const path& p, std::error_code& ec);

GHC_FS_API path relative(const path& p, std::error_code& ec);
GHC_FS_API path relative(const path& p, const path& base = current_path());
GHC_FS_API path relative(const path& p, const path& base, std::error_code& ec);

GHC_FS_API bool remove(const path& p);
GHC_FS_API bool remove(const path& p, std::error_code& ec) noexcept;

GHC_FS_API uintmax_t remove_all(const path& p);
GHC_FS_API uintmax_t remove_all(const path& p, std::error_code& ec) noexcept;

GHC_FS_API void rename(const path& from, const path& to);
GHC_FS_API void rename(const path& from, const path& to, std::error_code& ec) noexcept;

GHC_FS_API void resize_file(const path& p, uintmax_t size);
GHC_FS_API void resize_file(const path& p, uintmax_t size, std::error_code& ec) noexcept;

GHC_FS_API space_info space(const path& p);
GHC_FS_API space_info space(const path& p, std::error_code& ec) noexcept;

GHC_FS_API file_status status(const path& p);
GHC_FS_API file_status status(const path& p, std::error_code& ec) noexcept;

GHC_FS_API bool status_known(file_status s) noexcept;

GHC_FS_API file_status symlink_status(const path& p);
GHC_FS_API file_status symlink_status(const path& p, std::error_code& ec) noexcept;

GHC_FS_API path temp_directory_path();
GHC_FS_API path temp_directory_path(std::error_code& ec) noexcept;

GHC_FS_API path weakly_canonical(const path& p);
GHC_FS_API path weakly_canonical(const path& p, std::error_code& ec) noexcept;

// Non-C++17 add-on std::fstream wrappers with path
template <class charT, class traits = std::char_traits<charT>>
class basic_filebuf : public std::basic_filebuf<charT, traits>
{
public:
    basic_filebuf() {}
    ~basic_filebuf() override {}
    basic_filebuf(const basic_filebuf&) = delete;
    const basic_filebuf& operator=(const basic_filebuf&) = delete;
    basic_filebuf<charT, traits>* open(const path& p, std::ios_base::openmode mode)
    {
#if defined(GHC_OS_WINDOWS) && defined(_MSC_VER)
        return std::basic_filebuf<charT, traits>::open(p.wstring().c_str(), mode) ? this : 0;
#else
        return std::basic_filebuf<charT, traits>::open(p.c_str(), mode) ? this : 0;
#endif
    }
};

template <class charT, class traits = std::char_traits<charT>>
class basic_ifstream : public std::basic_ifstream<charT, traits>
{
public:
    basic_ifstream() {}
#if defined(GHC_OS_WINDOWS) && defined(_MSC_VER)
    explicit basic_ifstream(const path& p, std::ios_base::openmode mode = std::ios_base::in)
        : std::basic_ifstream<charT, traits>(p.wstring().c_str(), mode)
    {
    }
    void open(const path& p, std::ios_base::openmode mode = std::ios_base::in) { std::basic_ifstream<charT, traits>::open(p.wstring().c_str(), mode); }
#else
    explicit basic_ifstream(const path& p, std::ios_base::openmode mode = std::ios_base::in)
        : std::basic_ifstream<charT, traits>(p.c_str(), mode)
    {
    }
    void open(const path& p, std::ios_base::openmode mode = std::ios_base::in) { std::basic_ifstream<charT, traits>::open(p.c_str(), mode); }
#endif
    basic_ifstream(const basic_ifstream&) = delete;
    const basic_ifstream& operator=(const basic_ifstream&) = delete;
    ~basic_ifstream() override {}
};

template <class charT, class traits = std::char_traits<charT>>
class basic_ofstream : public std::basic_ofstream<charT, traits>
{
public:
    basic_ofstream() {}
#if defined(GHC_OS_WINDOWS) && defined(_MSC_VER)
    explicit basic_ofstream(const path& p, std::ios_base::openmode mode = std::ios_base::out)
        : std::basic_ofstream<charT, traits>(p.wstring().c_str(), mode)
    {
    }
    void open(const path& p, std::ios_base::openmode mode = std::ios_base::out) { std::basic_ofstream<charT, traits>::open(p.wstring().c_str(), mode); }
#else
    explicit basic_ofstream(const path& p, std::ios_base::openmode mode = std::ios_base::out)
        : std::basic_ofstream<charT, traits>(p.c_str(), mode)
    {
    }
    void open(const path& p, std::ios_base::openmode mode = std::ios_base::out) { std::basic_ofstream<charT, traits>::open(p.c_str(), mode); }
#endif
    basic_ofstream(const basic_ofstream&) = delete;
    const basic_ofstream& operator=(const basic_ofstream&) = delete;
    ~basic_ofstream() override {}
};

template <class charT, class traits = std::char_traits<charT>>
class basic_fstream : public std::basic_fstream<charT, traits>
{
public:
    basic_fstream() {}
#if defined(GHC_OS_WINDOWS) && defined(_MSC_VER)
    explicit basic_fstream(const path& p, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
        : std::basic_fstream<charT, traits>(p.wstring().c_str(), mode)
    {
    }
    void open(const path& p, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) { std::basic_fstream<charT, traits>::open(p.wstring().c_str(), mode); }
#else
    explicit basic_fstream(const path& p, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
        : std::basic_fstream<charT, traits>(p.c_str(), mode)
    {
    }
    void open(const path& p, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) { std::basic_fstream<charT, traits>::open(p.c_str(), mode); }
#endif
    basic_fstream(const basic_fstream&) = delete;
    const basic_fstream& operator=(const basic_fstream&) = delete;
    ~basic_fstream() override {}
};

typedef basic_filebuf<char> filebuf;
typedef basic_filebuf<wchar_t> wfilebuf;
typedef basic_ifstream<char> ifstream;
typedef basic_ifstream<wchar_t> wifstream;
typedef basic_ofstream<char> ofstream;
typedef basic_ofstream<wchar_t> wofstream;
typedef basic_fstream<char> fstream;
typedef basic_fstream<wchar_t> wfstream;

class GHC_FS_API_CLASS u8arguments
{
public:
    u8arguments(int& argc, char**& argv);
    ~u8arguments()
    {
        _refargc = _argc;
        _refargv = _argv;
    }

    bool valid() const { return _isvalid; }

private:
    int _argc;
    char** _argv;
    int& _refargc;
    char**& _refargv;
    bool _isvalid;
#ifdef GHC_OS_WINDOWS
    std::vector<std::string> _args;
    std::vector<char*> _argp;
#endif
};

//-------------------------------------------------------------------------------------------------
//  Implementation
//-------------------------------------------------------------------------------------------------

namespace detail {
GHC_FS_API void postprocess_path_with_format(path::string_type& p, path::format fmt);
enum utf8_states_t { S_STRT = 0, S_RJCT = 8 };
GHC_FS_API void appendUTF8(std::string& str, uint32_t unicode);
GHC_FS_API bool is_surrogate(uint32_t c);
GHC_FS_API bool is_high_surrogate(uint32_t c);
GHC_FS_API bool is_low_surrogate(uint32_t c);
GHC_FS_API unsigned consumeUtf8Fragment(const unsigned state, const uint8_t fragment, uint32_t& codepoint);
enum class portable_error {
    none = 0,
    exists,
    not_found,
    not_supported,
    not_implemented,
    invalid_argument,
    is_a_directory,
};
GHC_FS_API std::error_code make_error_code(portable_error err);
}  // namespace detail

namespace detail {

#ifdef GHC_EXPAND_IMPL

GHC_INLINE std::error_code make_error_code(portable_error err)
{
#ifdef GHC_OS_WINDOWS
    switch (err) {
        case portable_error::none:
            return std::error_code();
        case portable_error::exists:
            return std::error_code(ERROR_ALREADY_EXISTS, std::system_category());
        case portable_error::not_found:
            return std::error_code(ERROR_PATH_NOT_FOUND, std::system_category());
        case portable_error::not_supported:
            return std::error_code(ERROR_NOT_SUPPORTED, std::system_category());
        case portable_error::not_implemented:
            return std::error_code(ERROR_CALL_NOT_IMPLEMENTED, std::system_category());
        case portable_error::invalid_argument:
            return std::error_code(ERROR_INVALID_PARAMETER, std::system_category());
        case portable_error::is_a_directory:
#ifdef ERROR_DIRECTORY_NOT_SUPPORTED
            return std::error_code(ERROR_DIRECTORY_NOT_SUPPORTED, std::system_category()); 
#else
            return std::error_code(ERROR_NOT_SUPPORTED, std::system_category());
#endif
    }
#else
    switch (err) {
        case portable_error::none:
            return std::error_code();
        case portable_error::exists:
            return std::error_code(EEXIST, std::system_category());
        case portable_error::not_found:
            return std::error_code(ENOENT, std::system_category());
        case portable_error::not_supported:
            return std::error_code(ENOTSUP, std::system_category());
        case portable_error::not_implemented:
            return std::error_code(ENOSYS, std::system_category());
        case portable_error::invalid_argument:
            return std::error_code(EINVAL, std::system_category());
        case portable_error::is_a_directory:
            return std::error_code(EISDIR, std::system_category());
    }
#endif
    return std::error_code();
}

#endif  // GHC_EXPAND_IMPL

template <typename Enum>
using EnableBitmask = typename std::enable_if<std::is_same<Enum, perms>::value || std::is_same<Enum, perm_options>::value || std::is_same<Enum, copy_options>::value || std::is_same<Enum, directory_options>::value, Enum>::type;
}  // namespace detail

template <typename Enum>
detail::EnableBitmask<Enum> operator&(Enum X, Enum Y)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(X) & static_cast<underlying>(Y));
}

template <typename Enum>
detail::EnableBitmask<Enum> operator|(Enum X, Enum Y)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(X) | static_cast<underlying>(Y));
}

template <typename Enum>
detail::EnableBitmask<Enum> operator^(Enum X, Enum Y)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<underlying>(X) ^ static_cast<underlying>(Y));
}

template <typename Enum>
detail::EnableBitmask<Enum> operator~(Enum X)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(~static_cast<underlying>(X));
}

template <typename Enum>
detail::EnableBitmask<Enum>& operator&=(Enum& X, Enum Y)
{
    X = X & Y;
    return X;
}

template <typename Enum>
detail::EnableBitmask<Enum>& operator|=(Enum& X, Enum Y)
{
    X = X | Y;
    return X;
}

template <typename Enum>
detail::EnableBitmask<Enum>& operator^=(Enum& X, Enum Y)
{
    X = X ^ Y;
    return X;
}

#ifdef GHC_EXPAND_IMPL

namespace detail {

GHC_INLINE bool in_range(uint32_t c, uint32_t lo, uint32_t hi)
{
    return ((uint32_t)(c - lo) < (hi - lo + 1));
}

GHC_INLINE bool is_surrogate(uint32_t c)
{
    return in_range(c, 0xd800, 0xdfff);
}

GHC_INLINE bool is_high_surrogate(uint32_t c)
{
    return (c & 0xfffffc00) == 0xd800;
}

GHC_INLINE bool is_low_surrogate(uint32_t c)
{
    return (c & 0xfffffc00) == 0xdc00;
}

GHC_INLINE void appendUTF8(std::string& str, uint32_t unicode)
{
    if (unicode <= 0x7f) {
        str.push_back(static_cast<char>(unicode));
    }
    else if (unicode >= 0x80 && unicode <= 0x7ff) {
        str.push_back(static_cast<char>((unicode >> 6) + 192));
        str.push_back(static_cast<char>((unicode & 0x3f) + 128));
    }
    else if ((unicode >= 0x800 && unicode <= 0xd7ff) || (unicode >= 0xe000 && unicode <= 0xffff)) {
        str.push_back(static_cast<char>((unicode >> 12) + 224));
        str.push_back(static_cast<char>(((unicode & 0xfff) >> 6) + 128));
        str.push_back(static_cast<char>((unicode & 0x3f) + 128));
    }
    else {
        str.push_back(static_cast<char>((unicode >> 18) + 240));
        str.push_back(static_cast<char>(((unicode & 0x3ffff) >> 12) + 128));
        str.push_back(static_cast<char>(((unicode & 0xfff) >> 6) + 128));
        str.push_back(static_cast<char>((unicode & 0x3f) + 128));
    }
}

// Thanks to Bjoern Hoehrmann (https://bjoern.hoehrmann.de/utf-8/decoder/dfa/)
// and Taylor R Campbell for the ideas to this DFA approach of UTF-8 decoding;
// Generating debugging and shrinking my own DFA from scratch was a day of fun!
GHC_INLINE unsigned consumeUtf8Fragment(const unsigned state, const uint8_t fragment, uint32_t& codepoint)
{
    static const uint32_t utf8_state_info[] = {
        0x11111111u, 0x11111111u, 0x77777777u, 0x77777777u, 0x88888888u, 0x88888888u, 0x88888888u, 0x88888888u, 0x22222299u, 0x22222222u, 0x22222222u, 0x22222222u, 0x3333333au, 0x33433333u,
        0x9995666bu, 0x99999999u, 0x88888880u, 0x22818108u, 0x88888881u, 0x88888882u, 0x88888884u, 0x88888887u, 0x88888886u, 0x82218108u, 0x82281108u, 0x88888888u, 0x88888883u, 0x88888885u,
    };
    uint8_t category = fragment < 128 ? 0 : (utf8_state_info[(fragment >> 3) & 0xf] >> ((fragment & 7) << 2)) & 0xf;
    codepoint = (state ? (codepoint << 6) | (fragment & 0x3f) : (0xff >> category) & fragment);
    return state == S_RJCT ? static_cast<unsigned>(S_RJCT) : static_cast<unsigned>((utf8_state_info[category + 16] >> (state << 2)) & 0xf);
}

}  // namespace detail

#endif

namespace detail {

template <class StringType>
inline StringType fromUtf8(const std::string& utf8String, const typename StringType::allocator_type& alloc = typename StringType::allocator_type())
{
    if (sizeof(typename StringType::value_type) == 1) {
        return StringType(utf8String.begin(), utf8String.end());
    }
    StringType result(alloc);
    result.reserve(utf8String.length());
    std::string::const_iterator iter = utf8String.begin();
    unsigned utf8_state = S_STRT;
    std::uint32_t codepoint = 0;
    while (iter < utf8String.end()) {
        if ((utf8_state = consumeUtf8Fragment(utf8_state, (uint8_t)*iter++, codepoint)) == S_STRT) {
            if (sizeof(typename StringType::value_type) == 4) {
                result += codepoint;
            }
            else {
                if (codepoint <= 0xffff) {
                    result += (typename StringType::value_type)codepoint;
                }
                else {
                    codepoint -= 0x10000;
                    result += (typename StringType::value_type)((codepoint >> 10) + 0xd800);
                    result += (typename StringType::value_type)((codepoint & 0x3ff) + 0xdc00);
                }
            }
            codepoint = 0;
        }
        else if (utf8_state == S_RJCT) {
            result += (typename StringType::value_type)0xfffd;
            utf8_state = S_STRT;
            codepoint = 0;
        }
    }
    if (utf8_state) {
        result += (typename StringType::value_type)0xfffd;
    }
    return result;
}

template <typename charT, typename traits, typename Alloc>
inline std::string toUtf8(const std::basic_string<charT, traits, Alloc>& unicodeString)
{
    using StringType = std::basic_string<charT, traits, Alloc>;
    if (sizeof(typename StringType::value_type) == 1) {
        return std::string(unicodeString.begin(), unicodeString.end());
    }
    std::string result;
    result.reserve(unicodeString.length());
    if (sizeof(typename StringType::value_type) == 2) {
        for (typename StringType::const_iterator iter = unicodeString.begin(); iter != unicodeString.end(); ++iter) {
            char32_t c = *iter;
            if (is_surrogate(c)) {
                ++iter;
                if (iter != unicodeString.end() && is_high_surrogate(c) && is_low_surrogate(*iter)) {
                    appendUTF8(result, (char32_t(c) << 10) + *iter - 0x35fdc00);
                }
                else {
                    appendUTF8(result, 0xfffd);
                }
            }
            else {
                appendUTF8(result, c);
            }
        }
    }
    else {
        for (char32_t c : unicodeString) {
            appendUTF8(result, c);
        }
    }
    return result;
}

template <typename SourceType>
inline std::string toUtf8(const SourceType* unicodeString)
{
    return toUtf8(std::basic_string<SourceType, std::char_traits<SourceType>>(unicodeString));
}

}  // namespace detail

#ifdef GHC_EXPAND_IMPL

namespace detail {

GHC_INLINE bool startsWith(const std::string& what, const std::string& with)
{
    return with.length() <= what.length() && equal(with.begin(), with.end(), what.begin());
}

GHC_INLINE void postprocess_path_with_format(path::string_type& p, path::format fmt)
{
    switch (fmt) {
#ifndef GHC_OS_WINDOWS
        case path::auto_format:
        case path::native_format:
#endif
        case path::generic_format:
            // nothing to do
            break;
#ifdef GHC_OS_WINDOWS
        case path::auto_format:
        case path::native_format:
#endif
            if (startsWith(p, std::string("\\\\?\\"))) {
                // remove Windows long filename marker
                p.erase(0, 4);
                if (startsWith(p, std::string("UNC\\"))) {
                    p.erase(0, 2);
                    p[0] = '\\';
                }
            }
            for (auto& c : p) {
                if (c == '\\') {
                    c = '/';
                }
            }
            break;
    }
    if (p.length() > 2 && p[0] == '/' && p[1] == '/' && p[2] != '/') {
        std::string::iterator new_end = std::unique(p.begin() + 2, p.end(), [](path::value_type lhs, path::value_type rhs) { return lhs == rhs && lhs == '/'; });
        p.erase(new_end, p.end());
    }
    else {
        std::string::iterator new_end = std::unique(p.begin(), p.end(), [](path::value_type lhs, path::value_type rhs) { return lhs == rhs && lhs == '/'; });
        p.erase(new_end, p.end());
    }
}

}  // namespace detail

#endif  // GHC_EXPAND_IMPL

template <class Source, typename>
inline path::path(const Source& source, format fmt)
    : _path(source)
{
    detail::postprocess_path_with_format(_path, fmt);
}
template <>
inline path::path(const std::wstring& source, format fmt)
{
    _path = detail::toUtf8(source);
    detail::postprocess_path_with_format(_path, fmt);
}
template <>
inline path::path(const std::u16string& source, format fmt)
{
    _path = detail::toUtf8(source);
    detail::postprocess_path_with_format(_path, fmt);
}
template <>
inline path::path(const std::u32string& source, format fmt)
{
    _path = detail::toUtf8(source);
    detail::postprocess_path_with_format(_path, fmt);
}

template <class Source, typename>
inline path u8path(const Source& source)
{
    return path(source);
}
template <class InputIterator>
inline path u8path(InputIterator first, InputIterator last)
{
    return path(first, last);
}

template <class InputIterator>
inline path::path(InputIterator first, InputIterator last, format fmt)
    : path(std::basic_string<typename std::iterator_traits<InputIterator>::value_type>(first, last), fmt)
{
    // delegated
}

#ifdef GHC_EXPAND_IMPL

namespace detail {

GHC_INLINE bool equals_simple_insensitive(const char* str1, const char* str2)
{
    std::string a(str1);
    std::string b(str2);
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
/*#ifdef GHC_OS_WINDOWS
#  ifdef __GNUC__
    while (::tolower((unsigned char)*str1) == ::tolower((unsigned char)*str2++)) {
        if (*str1++ == 0)
            return true;
    }
    return false;
#  else
    return 0 == ::_stricmp(str1, str2);
#  endif
#else
    return 0 == strcasecmp(str1, str2);
#endif*/
}

template <typename ErrorNumber>
GHC_INLINE std::string systemErrorText(ErrorNumber code = 0)
{
#if defined(GHC_OS_WINDOWS)
    LPVOID msgBuf;
    DWORD dw = code ? code : ::GetLastError();
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msgBuf, 0, NULL);
    std::string msg = toUtf8(std::wstring((LPWSTR)msgBuf));
    LocalFree(msgBuf);
    return msg;
#elif defined(GHC_OS_MACOS) || ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !defined(_GNU_SOURCE)) || (defined(GHC_OS_ANDROID) && __ANDROID_API__ < 23)
    char buffer[512];
    int rc = strerror_r(code ? code : errno, buffer, sizeof(buffer));
    return rc == 0 ? (const char*)buffer : "Error in strerror_r!";
#else
    char buffer[512];
    char* msg = strerror_r(code ? code : errno, buffer, sizeof(buffer));
    return msg ? msg : buffer;
#endif
}

#ifdef GHC_OS_WINDOWS
using CreateSymbolicLinkW_fp = BOOLEAN(WINAPI*)(LPCWSTR, LPCWSTR, DWORD);
using CreateHardLinkW_fp = BOOLEAN(WINAPI*)(LPCWSTR, LPCWSTR, LPSECURITY_ATTRIBUTES);

GHC_INLINE void create_symlink(const path& target_name, const path& new_symlink, bool to_directory, std::error_code& ec)
{
    std::error_code tec;
    auto fs = status(target_name, tec);
    if ((fs.type() == file_type::directory && !to_directory) || (fs.type() == file_type::regular && to_directory)) {
        ec = detail::make_error_code(detail::portable_error::not_supported);
        return;
    }
    static CreateSymbolicLinkW_fp api_call = reinterpret_cast<CreateSymbolicLinkW_fp>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateSymbolicLinkW"));
    if (api_call) {
        if (api_call(detail::fromUtf8<std::wstring>(new_symlink.u8string()).c_str(), detail::fromUtf8<std::wstring>(target_name.u8string()).c_str(), to_directory ? 1 : 0) == 0) {
            auto result = ::GetLastError();
            if (result == ERROR_PRIVILEGE_NOT_HELD && api_call(detail::fromUtf8<std::wstring>(new_symlink.u8string()).c_str(), detail::fromUtf8<std::wstring>(target_name.u8string()).c_str(), to_directory ? 3 : 2) != 0) {
                return;
            }
            ec = std::error_code(result, std::system_category());
        }
    }
    else {
        ec = std::error_code(ERROR_NOT_SUPPORTED, std::system_category());
    }
}

GHC_INLINE void create_hardlink(const path& target_name, const path& new_hardlink, std::error_code& ec)
{
    static CreateHardLinkW_fp api_call = reinterpret_cast<CreateHardLinkW_fp>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "CreateHardLinkW"));
    if (api_call) {
        if (api_call(detail::fromUtf8<std::wstring>(new_hardlink.u8string()).c_str(), detail::fromUtf8<std::wstring>(target_name.u8string()).c_str(), NULL) == 0) {
            ec = std::error_code(::GetLastError(), std::system_category());
        }
    }
    else {
        ec = std::error_code(ERROR_NOT_SUPPORTED, std::system_category());
    }
}
#else
GHC_INLINE void create_symlink(const path& target_name, const path& new_symlink, bool, std::error_code& ec)
{
    if (::symlink(target_name.c_str(), new_symlink.c_str()) != 0) {
        ec = std::error_code(errno, std::system_category());
    }
}

GHC_INLINE void create_hardlink(const path& target_name, const path& new_hardlink, std::error_code& ec)
{
    if (::link(target_name.c_str(), new_hardlink.c_str()) != 0) {
        ec = std::error_code(errno, std::system_category());
    }
}
#endif

template <typename T>
GHC_INLINE file_status file_status_from_st_mode(T mode)
{
#ifdef GHC_OS_WINDOWS
    file_type ft = file_type::unknown;
    if ((mode & _S_IFDIR) == _S_IFDIR) {
        ft = file_type::directory;
    }
    else if ((mode & _S_IFREG) == _S_IFREG) {
        ft = file_type::regular;
    }
    else if ((mode & _S_IFCHR) == _S_IFCHR) {
        ft = file_type::character;
    }
    perms prms = static_cast<perms>(mode & 0xfff);
    return file_status(ft, prms);
#else
    file_type ft = file_type::unknown;
    if (S_ISDIR(mode)) {
        ft = file_type::directory;
    }
    else if (S_ISREG(mode)) {
        ft = file_type::regular;
    }
    else if (S_ISCHR(mode)) {
        ft = file_type::character;
    }
    else if (S_ISBLK(mode)) {
        ft = file_type::block;
    }
    else if (S_ISFIFO(mode)) {
        ft = file_type::fifo;
    }
    else if (S_ISLNK(mode)) {
        ft = file_type::symlink;
    }
    else if (S_ISSOCK(mode)) {
        ft = file_type::socket;
    }
    perms prms = static_cast<perms>(mode & 0xfff);
    return file_status(ft, prms);
#endif
}

GHC_INLINE path resolveSymlink(const path& p, std::error_code& ec)
{
#ifdef GHC_OS_WINDOWS
#ifndef REPARSE_DATA_BUFFER_HEADER_SIZE
    typedef struct _REPARSE_DATA_BUFFER
    {
        ULONG ReparseTag;
        USHORT ReparseDataLength;
        USHORT Reserved;
        union
        {
            struct
            {
                USHORT SubstituteNameOffset;
                USHORT SubstituteNameLength;
                USHORT PrintNameOffset;
                USHORT PrintNameLength;
                ULONG Flags;
                WCHAR PathBuffer[1];
            } SymbolicLinkReparseBuffer;
            struct
            {
                USHORT SubstituteNameOffset;
                USHORT SubstituteNameLength;
                USHORT PrintNameOffset;
                USHORT PrintNameLength;
                WCHAR PathBuffer[1];
            } MountPointReparseBuffer;
            struct
            {
                UCHAR DataBuffer[1];
            } GenericReparseBuffer;
        } DUMMYUNIONNAME;
    } REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE (16 * 1024)
#endif
#endif

    std::shared_ptr<void> file(CreateFileW(p.wstring().c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, 0), CloseHandle);
    if (file.get() == INVALID_HANDLE_VALUE) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return path();
    }

    char buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE] = {0};
    REPARSE_DATA_BUFFER& reparseData = *(REPARSE_DATA_BUFFER*)buffer;
    ULONG bufferUsed;
    path result;
    if (DeviceIoControl(file.get(), FSCTL_GET_REPARSE_POINT, 0, 0, &reparseData, sizeof(buffer), &bufferUsed, 0)) {
        if (IsReparseTagMicrosoft(reparseData.ReparseTag)) {
            switch (reparseData.ReparseTag) {
                case IO_REPARSE_TAG_SYMLINK:
                    result = std::wstring(&reparseData.SymbolicLinkReparseBuffer.PathBuffer[reparseData.SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(WCHAR)], reparseData.SymbolicLinkReparseBuffer.PrintNameLength / sizeof(WCHAR));
                    break;
                case IO_REPARSE_TAG_MOUNT_POINT:
                    result = std::wstring(&reparseData.MountPointReparseBuffer.PathBuffer[reparseData.MountPointReparseBuffer.PrintNameOffset / sizeof(WCHAR)], reparseData.MountPointReparseBuffer.PrintNameLength / sizeof(WCHAR));
                    break;
                default:
                    break;
            }
        }
    }
    else {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
    return result;
#else
    size_t bufferSize = 256;
    while (true) {
        std::vector<char> buffer(bufferSize, (char)0);
        auto rc = ::readlink(p.c_str(), buffer.data(), buffer.size());
        if (rc < 0) {
            ec = std::error_code(errno, std::system_category());
            return path();
        }
        else if (rc < static_cast<int>(bufferSize)) {
            return path(std::string(buffer.data(), rc));
        }
        bufferSize *= 2;
    }
    return path();
#endif
}

#ifdef GHC_OS_WINDOWS
GHC_INLINE time_t timeFromFILETIME(const FILETIME& ft)
{
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return ull.QuadPart / 10000000ULL - 11644473600ULL;
}

GHC_INLINE void timeToFILETIME(time_t t, FILETIME& ft)
{
    LONGLONG ll;
    ll = Int32x32To64(t, 10000000) + 116444736000000000;
    ft.dwLowDateTime = (DWORD)ll;
    ft.dwHighDateTime = ll >> 32;
}

template <typename INFO>
GHC_INLINE uintmax_t hard_links_from_INFO(const INFO* info)
{
    return static_cast<uintmax_t>(-1);
}

template <>
GHC_INLINE uintmax_t hard_links_from_INFO<BY_HANDLE_FILE_INFORMATION>(const BY_HANDLE_FILE_INFORMATION* info)
{
    return info->nNumberOfLinks;
}

template <typename INFO>
GHC_INLINE file_status status_from_INFO(const path& p, const INFO* info, std::error_code& ec, uintmax_t* sz = nullptr, time_t* lwt = nullptr) noexcept
{
    file_type ft = file_type::unknown;
    if ((info->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        ft = file_type::symlink;
    }
    else {
        if ((info->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            ft = file_type::directory;
        }
        else {
            ft = file_type::regular;
        }
    }
    perms prms = perms::owner_read | perms::group_read | perms::others_read;
    if (!(info->dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
        prms = prms | perms::owner_write | perms::group_write | perms::others_write;
    }
    std::string ext = p.extension().generic_string();
    if (equals_simple_insensitive(ext.c_str(), ".exe") || equals_simple_insensitive(ext.c_str(), ".cmd") || equals_simple_insensitive(ext.c_str(), ".bat") || equals_simple_insensitive(ext.c_str(), ".com")) {
        prms = prms | perms::owner_exec | perms::group_exec | perms::others_exec;
    }
    if (sz) {
        *sz = static_cast<uintmax_t>(info->nFileSizeHigh) << (sizeof(info->nFileSizeHigh) * 8) | info->nFileSizeLow;
    }
    if (lwt) {
        *lwt = detail::timeFromFILETIME(info->ftLastWriteTime);
    }
    return file_status(ft, prms);
}

#endif

GHC_INLINE bool is_not_found_error(std::error_code& ec)
{
#ifdef GHC_OS_WINDOWS
    return ec.value() == ERROR_FILE_NOT_FOUND || ec.value() == ERROR_PATH_NOT_FOUND || ec.value() == ERROR_INVALID_NAME;
#else
    return ec.value() == ENOENT || ec.value() == ENOTDIR;
#endif
}

GHC_INLINE file_status symlink_status_ex(const path& p, std::error_code& ec, uintmax_t* sz = nullptr, uintmax_t* nhl = nullptr, time_t* lwt = nullptr) noexcept
{
#ifdef GHC_OS_WINDOWS
    file_status fs;
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExW(detail::fromUtf8<std::wstring>(p.u8string()).c_str(), GetFileExInfoStandard, &attr)) {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
    else {
        ec.clear();
        fs = detail::status_from_INFO(p, &attr, ec, sz, lwt);
        if (nhl) {
            *nhl = 0;
        }
        if (attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            fs.type(file_type::symlink);
        }
    }
    if (detail::is_not_found_error(ec)) {
        return file_status(file_type::not_found);
    }
    return ec ? file_status(file_type::none) : fs;
#else
    (void)sz;
    (void)nhl;
    (void)lwt;
    struct ::stat fs;
    auto result = ::lstat(p.c_str(), &fs);
    if (result == 0) {
        ec.clear();
        file_status f_s = detail::file_status_from_st_mode(fs.st_mode);
        return f_s;
    }
    ec = std::error_code(errno, std::system_category());
    if (detail::is_not_found_error(ec)) {
        return file_status(file_type::not_found, perms::unknown);
    }
    return file_status(file_type::none);
#endif
}

GHC_INLINE file_status status_ex(const path& p, std::error_code& ec, file_status* sls = nullptr, uintmax_t* sz = nullptr, uintmax_t* nhl = nullptr, time_t* lwt = nullptr, int recurse_count = 0) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    if (recurse_count > 16) {
        ec = std::error_code(0x2A9 /*ERROR_STOPPED_ON_SYMLINK*/, std::system_category());
        return file_status(file_type::unknown);
    }
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!::GetFileAttributesExW(p.wstring().c_str(), GetFileExInfoStandard, &attr)) {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
    else if (attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        path target = resolveSymlink(p, ec);
        file_status result;
        if (!ec && !target.empty()) {
            if (sls) {
                *sls = status_from_INFO(p, &attr, ec);
            }
            return detail::status_ex(target, ec, nullptr, sz, nhl, lwt, recurse_count + 1);
        }
        return file_status(file_type::unknown);
    }
    if (ec) {
        if (detail::is_not_found_error(ec)) {
            return file_status(file_type::not_found);
        }
        return file_status(file_type::none);
    }
    if (nhl) {
        *nhl = 0;
    }
    return detail::status_from_INFO(p, &attr, ec, sz, lwt);
#else
    (void)recurse_count;
    struct ::stat st;
    auto result = ::lstat(p.c_str(), &st);
    if (result == 0) {
        ec.clear();
        file_status fs = detail::file_status_from_st_mode(st.st_mode);
        if (fs.type() == file_type::symlink) {
            result = ::stat(p.c_str(), &st);
            if (result == 0) {
                if (sls) {
                    *sls = fs;
                }
                fs = detail::file_status_from_st_mode(st.st_mode);
            }
        }
        if (sz) {
            *sz = st.st_size;
        }
        if (nhl) {
            *nhl = st.st_nlink;
        }
        if (lwt) {
            *lwt = st.st_mtime;
        }
        return fs;
    }
    else {
        ec = std::error_code(errno, std::system_category());
        if (detail::is_not_found_error(ec)) {
            return file_status(file_type::not_found, perms::unknown);
        }
        return file_status(file_type::none);
    }
#endif
}

}  // namespace detail

GHC_INLINE u8arguments::u8arguments(int& argc, char**& argv)
    : _argc(argc)
    , _argv(argv)
    , _refargc(argc)
    , _refargv(argv)
    , _isvalid(false)
{
#ifdef GHC_OS_WINDOWS
    LPWSTR* p;
    p = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    _args.reserve(argc);
    _argp.reserve(argc);
    for (size_t i = 0; i < static_cast<size_t>(argc); ++i) {
        _args.push_back(detail::toUtf8(std::wstring(p[i])));
        _argp.push_back((char*)_args[i].data());
    }
    argv = _argp.data();
    ::LocalFree(p);
    _isvalid = true;
#else
    std::setlocale(LC_ALL, "");
#if defined(__ANDROID__) && __ANDROID_API__ < 26
    _isvalid = true;
#else
    if (detail::equals_simple_insensitive(::nl_langinfo(CODESET), "UTF-8")) {
        _isvalid = true;
    }
#endif
#endif
}

//-----------------------------------------------------------------------------
// 30.10.8.4.1 constructors and destructor

GHC_INLINE path::path() noexcept {}

GHC_INLINE path::path(const path& p)
    : _path(p._path)
{
}

GHC_INLINE path::path(path&& p) noexcept
    : _path(std::move(p._path))
{
}

GHC_INLINE path::path(string_type&& source, format fmt)
    : _path(std::move(source))
{
    detail::postprocess_path_with_format(_path, fmt);
}

#endif  // GHC_EXPAND_IMPL

template <class Source, typename>
inline path::path(const Source& source, const std::locale& loc, format fmt)
    : path(source, fmt)
{
    std::string locName = loc.name();
    if (!(locName.length() >= 5 && (locName.substr(locName.length() - 5) == "UTF-8" || locName.substr(locName.length() - 5) == "utf-8"))) {
        throw filesystem_error("This implementation only supports UTF-8 locales!", path(_path), detail::make_error_code(detail::portable_error::not_supported));
    }
}

template <class InputIterator>
inline path::path(InputIterator first, InputIterator last, const std::locale& loc, format fmt)
    : path(std::basic_string<typename std::iterator_traits<InputIterator>::value_type>(first, last), fmt)
{
    std::string locName = loc.name();
    if (!(locName.length() >= 5 && (locName.substr(locName.length() - 5) == "UTF-8" || locName.substr(locName.length() - 5) == "utf-8"))) {
        throw filesystem_error("This implementation only supports UTF-8 locales!", path(_path), detail::make_error_code(detail::portable_error::not_supported));
    }
}

#ifdef GHC_EXPAND_IMPL

GHC_INLINE path::~path() {}

//-----------------------------------------------------------------------------
// 30.10.8.4.2 assignments

GHC_INLINE path& path::operator=(const path& p)
{
    _path = p._path;
    return *this;
}

GHC_INLINE path& path::operator=(path&& p) noexcept
{
    _path = std::move(p._path);
    return *this;
}

GHC_INLINE path& path::operator=(path::string_type&& source)
{
    return assign(source);
}

GHC_INLINE path& path::assign(path::string_type&& source)
{
    _path = std::move(source);
    detail::postprocess_path_with_format(_path, native_format);
    return *this;
}

#endif  // GHC_EXPAND_IMPL

template <class Source>
inline path& path::operator=(const Source& source)
{
    return assign(source);
}

template <class Source>
inline path& path::assign(const Source& source)
{
    _path.assign(detail::toUtf8(source));
    detail::postprocess_path_with_format(_path, native_format);
    return *this;
}

template <>
inline path& path::assign<path>(const path& source)
{
    _path = source._path;
    return *this;
}

template <class InputIterator>
inline path& path::assign(InputIterator first, InputIterator last)
{
    _path.assign(first, last);
    detail::postprocess_path_with_format(_path, native_format);
    return *this;
}

#ifdef GHC_EXPAND_IMPL

//-----------------------------------------------------------------------------
// 30.10.8.4.3 appends

GHC_INLINE path& path::operator/=(const path& p)
{
    if (p.empty()) {
        // was: if ((!has_root_directory() && is_absolute()) || has_filename())
        if (!_path.empty() && _path[_path.length() - 1] != '/' && _path[_path.length() - 1] != ':') {
            _path += '/';
        }
        return *this;
    }
    if ((p.is_absolute() && (_path != root_name() || p._path != "/")) || (p.has_root_name() && p.root_name() != root_name())) {
        assign(p);
        return *this;
    }
    if (p.has_root_directory()) {
        assign(root_name());
    }
    else if ((!has_root_directory() && is_absolute()) || has_filename()) {
        _path += '/';
    }
    auto iter = p.begin();
    bool first = true;
    if (p.has_root_name()) {
        ++iter;
    }
    while (iter != p.end()) {
        if (!first && !(!_path.empty() && _path[_path.length() - 1] == '/')) {
            _path += '/';
        }
        first = false;
        _path += (*iter++).generic_string();
    }
    return *this;
}

GHC_INLINE void path::append_name(const char* name)
{
    if (_path.empty()) {
        this->operator/=(path(name));
    }
    else {
        if (_path.back() != path::generic_separator) {
            _path.push_back(path::generic_separator);
        }
        _path += name;
    }
}

#endif  // GHC_EXPAND_IMPL

template <class Source>
inline path& path::operator/=(const Source& source)
{
    return append(source);
}

template <class Source>
inline path& path::append(const Source& source)
{
    return this->operator/=(path(detail::toUtf8(source)));
}

template <>
inline path& path::append<path>(const path& p)
{
    return this->operator/=(p);
}

template <class InputIterator>
inline path& path::append(InputIterator first, InputIterator last)
{
    std::basic_string<typename std::iterator_traits<InputIterator>::value_type> part(first, last);
    return append(part);
}

#ifdef GHC_EXPAND_IMPL

//-----------------------------------------------------------------------------
// 30.10.8.4.4 concatenation

GHC_INLINE path& path::operator+=(const path& x)
{
    return concat(x._path);
}

GHC_INLINE path& path::operator+=(const string_type& x)
{
    return concat(x);
}

#ifdef __cpp_lib_string_view
GHC_INLINE path& path::operator+=(std::basic_string_view<value_type> x)
{
    return concat(x);
}
#endif

GHC_INLINE path& path::operator+=(const value_type* x)
{
    return concat(string_type(x));
}

GHC_INLINE path& path::operator+=(value_type x)
{
#ifdef GHC_OS_WINDOWS
    if (x == '\\') {
        x = generic_separator;
    }
#endif
    if (_path.empty() || _path.back() != generic_separator) {
        _path += x;
    }
    return *this;
}

#endif  // GHC_EXPAND_IMPL

template <class Source>
inline path::path_type<Source>& path::operator+=(const Source& x)
{
    return concat(x);
}

template <class EcharT>
inline path::path_type_EcharT<EcharT>& path::operator+=(EcharT x)
{
    std::basic_string<EcharT> part(x);
    concat(detail::toUtf8(part));
    return *this;
}

template <class Source>
inline path& path::concat(const Source& x)
{
    path p(x);
    detail::postprocess_path_with_format(p._path, native_format);
    _path += p._path;
    return *this;
}
template <class InputIterator>
inline path& path::concat(InputIterator first, InputIterator last)
{
    _path.append(first, last);
    detail::postprocess_path_with_format(_path, native_format);
    return *this;
}

#ifdef GHC_EXPAND_IMPL

//-----------------------------------------------------------------------------
// 30.10.8.4.5 modifiers
GHC_INLINE void path::clear() noexcept
{
    _path.clear();
}

GHC_INLINE path& path::make_preferred()
{
    // as this filesystem implementation only uses generic_format
    // internally, this must be a no-op
    return *this;
}

GHC_INLINE path& path::remove_filename()
{
    if (has_filename()) {
        _path.erase(_path.size() - filename()._path.size());
    }
    return *this;
}

GHC_INLINE path& path::replace_filename(const path& replacement)
{
    remove_filename();
    return append(replacement);
}

GHC_INLINE path& path::replace_extension(const path& replacement)
{
    if (has_extension()) {
        _path.erase(_path.size() - extension()._path.size());
    }
    if (!replacement.empty() && replacement._path[0] != '.') {
        _path += '.';
    }
    return concat(replacement);
}

GHC_INLINE void path::swap(path& rhs) noexcept
{
    _path.swap(rhs._path);
}

//-----------------------------------------------------------------------------
// 30.10.8.4.6, native format observers
GHC_INLINE const path::string_type& path::native() const
{
#ifdef GHC_OS_WINDOWS
    if (is_absolute() && _path.length() > MAX_PATH - 10) {
        // expand long Windows filenames with marker
        if (has_root_name() && _path[0] == '/') {
            _native_cache = "\\\\?\\UNC" + _path.substr(1);
        }
        else {
            _native_cache = "\\\\?\\" + _path;
        }
    }
    else {
        _native_cache = _path;
    }
    /*if (has_root_name() && root_name()._path[0] == '/') {
        return _path;
    }*/
    for (auto& c : _native_cache) {
        if (c == '/') {
            c = '\\';
        }
    }
    return _native_cache;
#else
    return _path;
#endif
}

GHC_INLINE const path::value_type* path::c_str() const
{
    return native().c_str();
}

GHC_INLINE path::operator path::string_type() const
{
    return native();
}

#endif  // GHC_EXPAND_IMPL

template <class EcharT, class traits, class Allocator>
inline std::basic_string<EcharT, traits, Allocator> path::string(const Allocator& a) const
{
    return detail::fromUtf8<std::basic_string<EcharT, traits, Allocator>>(native(), a);
}

#ifdef GHC_EXPAND_IMPL

GHC_INLINE std::string path::string() const
{
    return native();
}

GHC_INLINE std::wstring path::wstring() const
{
    return detail::fromUtf8<std::wstring>(native());
}

GHC_INLINE std::string path::u8string() const
{
    return native();
}

GHC_INLINE std::u16string path::u16string() const
{
    return detail::fromUtf8<std::u16string>(native());
}

GHC_INLINE std::u32string path::u32string() const
{
    return detail::fromUtf8<std::u32string>(native());
}

#endif  // GHC_EXPAND_IMPL

//-----------------------------------------------------------------------------
// 30.10.8.4.7, generic format observers
template <class EcharT, class traits, class Allocator>
inline std::basic_string<EcharT, traits, Allocator> path::generic_string(const Allocator& a) const
{
    return detail::fromUtf8<std::basic_string<EcharT, traits, Allocator>>(_path, a);
}

#ifdef GHC_EXPAND_IMPL

GHC_INLINE const std::string& path::generic_string() const
{
    return _path;
}

GHC_INLINE std::wstring path::generic_wstring() const
{
    return detail::fromUtf8<std::wstring>(_path);
}

GHC_INLINE std::string path::generic_u8string() const
{
    return _path;
}

GHC_INLINE std::u16string path::generic_u16string() const
{
    return detail::fromUtf8<std::u16string>(_path);
}

GHC_INLINE std::u32string path::generic_u32string() const
{
    return detail::fromUtf8<std::u32string>(_path);
}

//-----------------------------------------------------------------------------
// 30.10.8.4.8, compare
GHC_INLINE int path::compare(const path& p) const noexcept
{
    return native().compare(p.native());
}

GHC_INLINE int path::compare(const string_type& s) const
{
    return native().compare(path(s).native());
}

#ifdef __cpp_lib_string_view
GHC_INLINE int path::compare(std::basic_string_view<value_type> s) const
{
    return native().compare(path(s).native());
}
#endif

GHC_INLINE int path::compare(const value_type* s) const
{
    return native().compare(path(s).native());
}

//-----------------------------------------------------------------------------
// 30.10.8.4.9, decomposition
GHC_INLINE path path::root_name() const
{
#ifdef GHC_OS_WINDOWS
    if (_path.length() >= 2 && std::toupper(static_cast<unsigned char>(_path[0])) >= 'A' && std::toupper(static_cast<unsigned char>(_path[0])) <= 'Z' && _path[1] == ':') {
        return path(_path.substr(0, 2));
    }
#endif
    if (_path.length() > 2 && _path[0] == '/' && _path[1] == '/' && _path[2] != '/' && std::isprint(_path[2])) {
        string_type::size_type pos = _path.find_first_of("/\\", 3);
        if (pos == string_type::npos) {
            return path(_path);
        }
        else {
            return path(_path.substr(0, pos));
        }
    }
    return path();
}

GHC_INLINE path path::root_directory() const
{
    path root = root_name();
    if (_path.length() > root._path.length() && _path[root._path.length()] == '/') {
        return path("/");
    }
    return path();
}

GHC_INLINE path path::root_path() const
{
    return root_name().generic_string() + root_directory().generic_string();
}

GHC_INLINE path path::relative_path() const
{
    std::string root = root_path()._path;
    return path(_path.substr((std::min)(root.length(), _path.length())), generic_format);
}

GHC_INLINE path path::parent_path() const
{
    if (has_relative_path()) {
        if (empty() || begin() == --end()) {
            return path();
        }
        else {
            path pp;
            for (const string_type s : input_iterator_range<iterator>(begin(), --end())) {
                if (s == "/") {
                    // don't use append to join a path-
                    pp += s;
                }
                else {
                    pp /= s;
                }
            }
            return pp;
        }
    }
    else {
        return *this;
    }
}

GHC_INLINE path path::filename() const
{
    return relative_path().empty() ? path() : path(*--end());
}

GHC_INLINE path path::stem() const
{
    string_type fn = filename();
    if (fn != "." && fn != "..") {
        string_type::size_type n = fn.rfind(".");
        if (n != string_type::npos && n != 0) {
            return fn.substr(0, n);
        }
    }
    return fn;
}

GHC_INLINE path path::extension() const
{
    string_type fn = filename();
    string_type::size_type pos = fn.find_last_of('.');
    if (pos == std::string::npos || pos == 0) {
        return "";
    }
    return fn.substr(pos);
}

//-----------------------------------------------------------------------------
// 30.10.8.4.10, query
GHC_INLINE bool path::empty() const noexcept
{
    return _path.empty();
}

GHC_INLINE bool path::has_root_name() const
{
    return !root_name().empty();
}

GHC_INLINE bool path::has_root_directory() const
{
    return !root_directory().empty();
}

GHC_INLINE bool path::has_root_path() const
{
    return !root_path().empty();
}

GHC_INLINE bool path::has_relative_path() const
{
    return !relative_path().empty();
}

GHC_INLINE bool path::has_parent_path() const
{
    return !parent_path().empty();
}

GHC_INLINE bool path::has_filename() const
{
    return !filename().empty();
}

GHC_INLINE bool path::has_stem() const
{
    return !stem().empty();
}

GHC_INLINE bool path::has_extension() const
{
    return !extension().empty();
}

GHC_INLINE bool path::is_absolute() const
{
#ifdef GHC_OS_WINDOWS
    return has_root_name() && has_root_directory();
#else
    return has_root_directory();
#endif
}

GHC_INLINE bool path::is_relative() const
{
    return !is_absolute();
}

//-----------------------------------------------------------------------------
// 30.10.8.4.11, generation
GHC_INLINE path path::lexically_normal() const
{
    path dest;
    for (const string_type s : *this) {
        if (s == ".") {
            dest /= "";
            continue;
        }
        else if (s == ".." && !dest.empty()) {
            auto root = root_path();
            if (dest == root) {
                continue;
            }
            else if (*(--dest.end()) != "..") {
                if (dest._path.back() == generic_separator) {
                    dest._path.pop_back();
                }
                dest.remove_filename();
                continue;
            }
        }
        dest /= s;
    }
    if (dest.empty()) {
        dest = ".";
    }
    return dest;
}

GHC_INLINE path path::lexically_relative(const path& base) const
{
    if (root_name() != base.root_name() || is_absolute() != base.is_absolute() || (!has_root_directory() && base.has_root_directory())) {
        return path();
    }
    const_iterator a = begin(), b = base.begin();
    while (a != end() && b != base.end() && *a == *b) {
        ++a;
        ++b;
    }
    if (a == end() && b == base.end()) {
        return path(".");
    }
    int count = 0;
    for (const auto& element : input_iterator_range<const_iterator>(b, base.end())) {
        if (element != "." && element != "..") {
            ++count;
        }
        else if (element == "..") {
            --count;
        }
    }
    if (count < 0) {
        return path();
    }
    path result;
    for (int i = 0; i < count; ++i) {
        result /= "..";
    }
    for (const auto& element : input_iterator_range<const_iterator>(a, end())) {
        result /= element;
    }
    return result;
}

GHC_INLINE path path::lexically_proximate(const path& base) const
{
    path result = lexically_relative(base);
    return result.empty() ? *this : result;
}

//-----------------------------------------------------------------------------
// 30.10.8.5, iterators
GHC_INLINE path::iterator::iterator() {}

GHC_INLINE path::iterator::iterator(const path::string_type::const_iterator& first, const path::string_type::const_iterator& last, const path::string_type::const_iterator& pos)
    : _first(first)
    , _last(last)
    , _iter(pos)
{
    updateCurrent();
    // find the position of a potential root directory slash
#ifdef GHC_OS_WINDOWS
    if (_last - _first >= 3 && std::toupper(static_cast<unsigned char>(*first)) >= 'A' && std::toupper(static_cast<unsigned char>(*first)) <= 'Z' && *(first + 1) == ':' && *(first + 2) == '/') {
        _root = _first + 2;
    }
    else
#endif
        if (_first != _last && *_first == '/') {
        if (_last - _first >= 2 && *(_first + 1) == '/' && !(_last - _first >= 3 && *(_first + 2) == '/')) {
            _root = increment(_first);
        }
        else {
            _root = _first;
        }
    }
    else {
        _root = _last;
    }
}

GHC_INLINE path::string_type::const_iterator path::iterator::increment(const path::string_type::const_iterator& pos) const
{
    std::string::const_iterator i = pos;
    bool fromStart = i == _first;
    if (i != _last) {
        // we can only sit on a slash if it is a network name or a root
        if (*i++ == '/') {
            if (i != _last && *i == '/') {
                if (fromStart && !(i + 1 != _last && *(i + 1) == '/')) {
                    // leadind double slashes detected, treat this and the
                    // following until a slash as one unit
                    i = std::find(++i, _last, '/');
                }
                else {
                    // skip redundant slashes
                    while (i != _last && *i == '/') {
                        ++i;
                    }
                }
            }
        }
        else {
            if (fromStart && i != _last && *i == ':') {
                ++i;
            }
            else {
                i = std::find(i, _last, '/');
            }
        }
    }
    return i;
}

GHC_INLINE path::string_type::const_iterator path::iterator::decrement(const path::string_type::const_iterator& pos) const
{
    std::string::const_iterator i = pos;
    if (i != _first) {
        --i;
        // if this is now the root slash or the trailing slash, we are done,
        // else check for network name
        if (i != _root && (pos != _last || *i != '/')) {
#ifdef GHC_OS_WINDOWS
            static const std::string seps = "/:";
            i = std::find_first_of(std::reverse_iterator<std::string::const_iterator>(i), std::reverse_iterator<std::string::const_iterator>(_first), seps.begin(), seps.end()).base();
            if (i > _first && *i == ':') {
                i++;
            }
#else
            i = std::find(std::reverse_iterator<std::string::const_iterator>(i), std::reverse_iterator<std::string::const_iterator>(_first), '/').base();
#endif
            // Now we have to check if this is a network name
            if (i - _first == 2 && *_first == '/' && *(_first + 1) == '/') {
                i -= 2;
            }
        }
    }
    return i;
}

GHC_INLINE void path::iterator::updateCurrent()
{
    if (_iter != _first && _iter != _last && (*_iter == '/' && _iter != _root) && (_iter + 1 == _last)) {
        _current = "";
    }
    else {
        _current.assign(_iter, increment(_iter));
        if (_current.generic_string().size() > 1 && _current.generic_string()[0] == '/' && _current.generic_string()[_current.generic_string().size() - 1] == '/') {
            // shrink successive slashes to one
            _current = "/";
        }
    }
}

GHC_INLINE path::iterator& path::iterator::operator++()
{
    _iter = increment(_iter);
    while (_iter != _last &&     // we didn't reach the end
           _iter != _root &&     // this is not a root position
           *_iter == '/' &&      // we are on a slash
           (_iter + 1) != _last  // the slash is not the last char
    ) {
        ++_iter;
    }
    updateCurrent();
    return *this;
}

GHC_INLINE path::iterator path::iterator::operator++(int)
{
    path::iterator i{*this};
    ++(*this);
    return i;
}

GHC_INLINE path::iterator& path::iterator::operator--()
{
    _iter = decrement(_iter);
    updateCurrent();
    return *this;
}

GHC_INLINE path::iterator path::iterator::operator--(int)
{
    path::iterator i{*this};
    --(*this);
    return i;
}

GHC_INLINE bool path::iterator::operator==(const path::iterator& other) const
{
    return _iter == other._iter;
}

GHC_INLINE bool path::iterator::operator!=(const path::iterator& other) const
{
    return _iter != other._iter;
}

GHC_INLINE path::iterator::reference path::iterator::operator*() const
{
    return _current;
}

GHC_INLINE path::iterator::pointer path::iterator::operator->() const
{
    return &_current;
}

GHC_INLINE path::iterator path::begin() const
{
    return iterator(_path.begin(), _path.end(), _path.begin());
}

GHC_INLINE path::iterator path::end() const
{
    return iterator(_path.begin(), _path.end(), _path.end());
}

//-----------------------------------------------------------------------------
// 30.10.8.6, path non-member functions
GHC_INLINE void swap(path& lhs, path& rhs) noexcept
{
    swap(lhs._path, rhs._path);
}

GHC_INLINE size_t hash_value(const path& p) noexcept
{
    return std::hash<std::string>()(p.generic_string());
}

GHC_INLINE bool operator==(const path& lhs, const path& rhs) noexcept
{
    return lhs.generic_string() == rhs.generic_string();
}

GHC_INLINE bool operator!=(const path& lhs, const path& rhs) noexcept
{
    return lhs.generic_string() != rhs.generic_string();
}

GHC_INLINE bool operator<(const path& lhs, const path& rhs) noexcept
{
    return lhs.generic_string() < rhs.generic_string();
}

GHC_INLINE bool operator<=(const path& lhs, const path& rhs) noexcept
{
    return lhs.generic_string() <= rhs.generic_string();
}

GHC_INLINE bool operator>(const path& lhs, const path& rhs) noexcept
{
    return lhs.generic_string() > rhs.generic_string();
}

GHC_INLINE bool operator>=(const path& lhs, const path& rhs) noexcept
{
    return lhs.generic_string() >= rhs.generic_string();
}

GHC_INLINE path operator/(const path& lhs, const path& rhs)
{
    path result(lhs);
    result /= rhs;
    return result;
}

#endif  // GHC_EXPAND_IMPL

//-----------------------------------------------------------------------------
// 30.10.8.6.1 path inserter and extractor
template <class charT, class traits>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& os, const path& p)
{
    os << "\"";
    auto ps = p.string<charT, traits>();
    for (auto c : ps) {
        if (c == '"' || c == '\\') {
            os << '\\';
        }
        os << c;
    }
    os << "\"";
    return os;
}

template <class charT, class traits>
inline std::basic_istream<charT, traits>& operator>>(std::basic_istream<charT, traits>& is, path& p)
{
    std::basic_string<charT, traits> tmp;
    auto c = is.get();
    if (c == '"') {
        auto sf = is.flags();
        is >> std::noskipws;
        while (is) {
            c = is.get();
            if (is) {
                if (c == '\\') {
                    c = is.get();
                    if (is) {
                        tmp += static_cast<charT>(c);
                    }
                }
                else if (c == '"') {
                    break;
                }
                else {
                    tmp += static_cast<charT>(c);
                }
            }
        }
        if ((sf & std::ios_base::skipws) == std::ios_base::skipws) {
            is >> std::skipws;
        }
        p = path(tmp);
    }
    else {
        is >> tmp;
        p = path(static_cast<charT>(c) + tmp);
    }
    return is;
}

#ifdef GHC_EXPAND_IMPL

//-----------------------------------------------------------------------------
// 30.10.9 Class filesystem_error
GHC_INLINE filesystem_error::filesystem_error(const std::string& what_arg, std::error_code ec)
    : std::system_error(ec, what_arg)
    , _what_arg(what_arg)
    , _ec(ec)
{
}

GHC_INLINE filesystem_error::filesystem_error(const std::string& what_arg, const path& p1, std::error_code ec)
    : std::system_error(ec, what_arg)
    , _what_arg(what_arg)
    , _ec(ec)
    , _p1(p1)
{
    if (!_p1.empty()) {
        _what_arg += ": '" + _p1.u8string() + "'";
    }
}

GHC_INLINE filesystem_error::filesystem_error(const std::string& what_arg, const path& p1, const path& p2, std::error_code ec)
    : std::system_error(ec, what_arg)
    , _what_arg(what_arg)
    , _ec(ec)
    , _p1(p1)
    , _p2(p2)
{
    if (!_p1.empty()) {
        _what_arg += ": '" + _p1.u8string() + "'";
    }
    if (!_p2.empty()) {
        _what_arg += ", '" + _p2.u8string() + "'";
    }
}

GHC_INLINE const path& filesystem_error::path1() const noexcept
{
    return _p1;
}

GHC_INLINE const path& filesystem_error::path2() const noexcept
{
    return _p2;
}

GHC_INLINE const char* filesystem_error::what() const noexcept
{
    return _what_arg.c_str();
}

//-----------------------------------------------------------------------------
// 30.10.15, filesystem operations
GHC_INLINE path absolute(const path& p)
{
    std::error_code ec;
    path result = absolute(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE path absolute(const path& p, std::error_code& ec)
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    if (p.empty()) {
        return absolute(current_path(ec), ec) / "";
    }
    ULONG size = ::GetFullPathNameW(p.wstring().c_str(), 0, 0, 0);
    if (size) {
        std::vector<wchar_t> buf(size, 0);
        ULONG s2 = GetFullPathNameW(p.wstring().c_str(), size, buf.data(), nullptr);
        if (s2 && s2 < size) {
            path result = path(std::wstring(buf.data(), s2));
            if (p.filename() == ".") {
                result /= ".";
            }
            return result;
        }
    }
    ec = std::error_code(::GetLastError(), std::system_category());
    return path();
#else
    path base = current_path(ec);
    if (!ec) {
        if (p.empty()) {
            return base / p;
        }
        if (p.has_root_name()) {
            if (p.has_root_directory()) {
                return p;
            }
            else {
                return p.root_name() / base.root_directory() / base.relative_path() / p.relative_path();
            }
        }
        else {
            if (p.has_root_directory()) {
                return base.root_name() / p;
            }
            else {
                return base / p;
            }
        }
    }
    ec = std::error_code(errno, std::system_category());
    return path();
#endif
}

GHC_INLINE path canonical(const path& p)
{
    std::error_code ec;
    auto result = canonical(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE path canonical(const path& p, std::error_code& ec)
{
    if (p.empty()) {
        ec = detail::make_error_code(detail::portable_error::not_found);
        return path();
    }
    path work = p.is_absolute() ? p : absolute(p, ec);
    path root = work.root_path();
    path result;

    auto fs = status(work, ec);
    if (ec) {
        return path();
    }
    if (fs.type() == file_type::not_found) {
        ec = detail::make_error_code(detail::portable_error::not_found);
        return path();
    }
    bool redo;
    do {
        redo = false;
        result.clear();
        for (auto pe : work) {
            if (pe.empty() || pe == ".") {
                continue;
            }
            else if (pe == "..") {
                result = result.parent_path();
                continue;
            }
            else if ((result / pe).string().length() <= root.string().length()) {
                result /= pe;
                continue;
            }
            auto sls = symlink_status(result / pe, ec);
            if (ec) {
                return path();
            }
            if (is_symlink(sls)) {
                redo = true;
                auto target = read_symlink(result / pe, ec);
                if (ec) {
                    return path();
                }
                if (target.is_absolute()) {
                    result = target;
                    continue;
                }
                else {
                    result /= target;
                    continue;
                }
            }
            else {
                result /= pe;
            }
        }
        work = result;
    } while (redo);
    ec.clear();
    return result;
}

GHC_INLINE void copy(const path& from, const path& to)
{
    copy(from, to, copy_options::none);
}

GHC_INLINE void copy(const path& from, const path& to, std::error_code& ec) noexcept
{
    copy(from, to, copy_options::none, ec);
}

GHC_INLINE void copy(const path& from, const path& to, copy_options options)
{
    std::error_code ec;
    copy(from, to, options, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), from, to, ec);
    }
}

GHC_INLINE void copy(const path& from, const path& to, copy_options options, std::error_code& ec) noexcept
{
    std::error_code tec;
    file_status fs_from, fs_to;
    ec.clear();
    if ((options & (copy_options::skip_symlinks | copy_options::copy_symlinks | copy_options::create_symlinks)) != copy_options::none) {
        fs_from = symlink_status(from, ec);
    }
    else {
        fs_from = status(from, ec);
    }
    if (!exists(fs_from)) {
        if (!ec) {
            ec = detail::make_error_code(detail::portable_error::not_found);
        }
        return;
    }
    if ((options & (copy_options::skip_symlinks | copy_options::create_symlinks)) != copy_options::none) {
        fs_to = symlink_status(to, tec);
    }
    else {
        fs_to = status(to, tec);
    }
    if (is_other(fs_from) || is_other(fs_to) || (is_directory(fs_from) && is_regular_file(fs_to)) || (exists(fs_to) && equivalent(from, to, ec))) {
        ec = detail::make_error_code(detail::portable_error::invalid_argument);
    }
    else if (is_symlink(fs_from)) {
        if ((options & copy_options::skip_symlinks) == copy_options::none) {
            if (!exists(fs_to) && (options & copy_options::copy_symlinks) != copy_options::none) {
                copy_symlink(from, to, ec);
            }
            else {
                ec = detail::make_error_code(detail::portable_error::invalid_argument);
            }
        }
    }
    else if (is_regular_file(fs_from)) {
        if ((options & copy_options::directories_only) == copy_options::none) {
            if ((options & copy_options::create_symlinks) != copy_options::none) {
                create_symlink(from.is_absolute() ? from : canonical(from, ec), to, ec);
            }
            else if ((options & copy_options::create_hard_links) != copy_options::none) {
                create_hard_link(from, to, ec);
            }
            else if (is_directory(fs_to)) {
                copy_file(from, to / from.filename(), options, ec);
            }
            else {
                copy_file(from, to, ec);
            }
        }
    }
#ifdef LWG_2682_BEHAVIOUR
    else if (is_directory(fs_from) && (options & copy_options::create_symlinks) != copy_options::none) {
        ec = detail::make_error_code(detail::portable_error::is_a_directory);
    }
#endif
    else if (is_directory(fs_from) && (options == copy_options::none || (options & copy_options::recursive) != copy_options::none)) {
        if (!exists(fs_to)) {
            create_directory(to, from, ec);
            if (ec) {
                return;
            }
        }
        for (auto iter = directory_iterator(from, ec); iter != directory_iterator(); iter.increment(ec)) {
            if (!ec) {
                copy(iter->path(), to / iter->path().filename(), options | static_cast<copy_options>(0x8000), ec);
            }
            if (ec) {
                return;
            }
        }
    }
    return;
}

GHC_INLINE bool copy_file(const path& from, const path& to)
{
    return copy_file(from, to, copy_options::none);
}

GHC_INLINE bool copy_file(const path& from, const path& to, std::error_code& ec) noexcept
{
    return copy_file(from, to, copy_options::none, ec);
}

GHC_INLINE bool copy_file(const path& from, const path& to, copy_options option)
{
    std::error_code ec;
    auto result = copy_file(from, to, option, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), from, to, ec);
    }
    return result;
}

GHC_INLINE bool copy_file(const path& from, const path& to, copy_options options, std::error_code& ec) noexcept
{
    std::error_code tecf, tect;
    auto sf = status(from, tecf);
    auto st = status(to, tect);
    bool overwrite = false;
    ec.clear();
    if (!is_regular_file(sf)) {
        ec = tecf;
        return false;
    }
    if (exists(st) && (!is_regular_file(st) || equivalent(from, to, ec) || (options & (copy_options::skip_existing | copy_options::overwrite_existing | copy_options::update_existing)) == copy_options::none)) {
        ec = tect ? tect : detail::make_error_code(detail::portable_error::exists);
        return false;
    }
    if (exists(st)) {
        if ((options & copy_options::update_existing) == copy_options::update_existing) {
            auto from_time = last_write_time(from, ec);
            if (ec) {
                ec = std::error_code(errno, std::system_category());
                return false;
            }
            auto to_time = last_write_time(to, ec);
            if (ec) {
                ec = std::error_code(errno, std::system_category());
                return false;
            }
            if (from_time <= to_time) {
                return false;
            }
        }
        overwrite = true;
    }
#ifdef GHC_OS_WINDOWS
    if (!::CopyFileW(detail::fromUtf8<std::wstring>(from.u8string()).c_str(), detail::fromUtf8<std::wstring>(to.u8string()).c_str(), !overwrite)) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return false;
    }
    return true;
#else
    std::vector<char> buffer(16384, '\0');
    int in = -1, out = -1;
    if ((in = ::open(from.c_str(), O_RDONLY)) < 0) {
        ec = std::error_code(errno, std::system_category());
        return false;
    }
    std::shared_ptr<void> guard_out(nullptr, [out](void*) { ::close(out); });
    int mode = O_CREAT | O_WRONLY | O_TRUNC;
    if (!overwrite) {
        mode |= O_EXCL;
    }
    if ((out = ::open(to.c_str(), mode, static_cast<int>(sf.permissions() & perms::all))) < 0) {
        ec = std::error_code(errno, std::system_category());
        return false;
    }
    std::shared_ptr<void> guard_in(nullptr, [in](void*) { ::close(in); });
    ssize_t br, bw;
    while ((br = ::read(in, buffer.data(), buffer.size())) > 0) {
        int offset = 0;
        do {
            if ((bw = ::write(out, buffer.data() + offset, br)) > 0) {
                br -= bw;
                offset += bw;
            }
            else if (bw < 0) {
                ec = std::error_code(errno, std::system_category());
                return false;
            }
        } while (br);
    }
    return true;
#endif
}

GHC_INLINE void copy_symlink(const path& existing_symlink, const path& new_symlink)
{
    std::error_code ec;
    copy_symlink(existing_symlink, new_symlink, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), existing_symlink, new_symlink, ec);
    }
}

GHC_INLINE void copy_symlink(const path& existing_symlink, const path& new_symlink, std::error_code& ec) noexcept
{
    ec.clear();
    auto to = read_symlink(existing_symlink, ec);
    if (!ec) {
        if (exists(to, ec) && is_directory(to, ec)) {
            create_directory_symlink(to, new_symlink, ec);
        }
        else {
            create_symlink(to, new_symlink, ec);
        }
    }
}

GHC_INLINE bool create_directories(const path& p)
{
    std::error_code ec;
    auto result = create_directories(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE bool create_directories(const path& p, std::error_code& ec) noexcept
{
    path current;
    ec.clear();
    for (const std::string part : p) {
        current /= part;
        if (current != p.root_name() && current != p.root_path()) {
            std::error_code tec;
            auto fs = status(current, tec);
            if (tec && fs.type() != file_type::not_found) {
                ec = tec;
                return false;
            }
            if (!exists(fs)) {
                create_directory(current, ec);
                if (ec) {
                    return false;
                }
            }
#ifndef LWG_2935_BEHAVIOUR
            else if (!is_directory(fs)) {
                ec = detail::make_error_code(detail::portable_error::exists);
                return false;
            }
#endif
        }
    }
    return true;
}

GHC_INLINE bool create_directory(const path& p)
{
    std::error_code ec;
    auto result = create_directory(p, path(), ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE bool create_directory(const path& p, std::error_code& ec) noexcept
{
    return create_directory(p, path(), ec);
}

GHC_INLINE bool create_directory(const path& p, const path& attributes)
{
    std::error_code ec;
    auto result = create_directory(p, attributes, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE bool create_directory(const path& p, const path& attributes, std::error_code& ec) noexcept
{
    std::error_code tec;
    ec.clear();
    auto fs = status(p, tec);
#ifdef LWG_2935_BEHAVIOUR
    if (status_known(fs) && exists(fs)) {
        return false;
    }
#else
    if (status_known(fs) && exists(fs) && is_directory(fs)) {
        return false;
    }
#endif
#ifdef GHC_OS_WINDOWS
    if (!attributes.empty()) {
        if (!::CreateDirectoryExW(detail::fromUtf8<std::wstring>(attributes.u8string()).c_str(), detail::fromUtf8<std::wstring>(p.u8string()).c_str(), NULL)) {
            ec = std::error_code(::GetLastError(), std::system_category());
            return false;
        }
    }
    else if (!::CreateDirectoryW(detail::fromUtf8<std::wstring>(p.u8string()).c_str(), NULL)) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return false;
    }
#else
    ::mode_t attribs = static_cast<mode_t>(perms::all);
    if (!attributes.empty()) {
        struct ::stat fileStat;
        if (::stat(attributes.c_str(), &fileStat) != 0) {
            ec = std::error_code(errno, std::system_category());
            return false;
        }
        attribs = fileStat.st_mode;
    }
    if (::mkdir(p.c_str(), attribs) != 0) {
        ec = std::error_code(errno, std::system_category());
        return false;
    }
#endif
    return true;
}

GHC_INLINE void create_directory_symlink(const path& to, const path& new_symlink)
{
    std::error_code ec;
    create_directory_symlink(to, new_symlink, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), to, new_symlink, ec);
    }
}

GHC_INLINE void create_directory_symlink(const path& to, const path& new_symlink, std::error_code& ec) noexcept
{
    detail::create_symlink(to, new_symlink, true, ec);
}

GHC_INLINE void create_hard_link(const path& to, const path& new_hard_link)
{
    std::error_code ec;
    create_hard_link(to, new_hard_link, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), to, new_hard_link, ec);
    }
}

GHC_INLINE void create_hard_link(const path& to, const path& new_hard_link, std::error_code& ec) noexcept
{
    detail::create_hardlink(to, new_hard_link, ec);
}

GHC_INLINE void create_symlink(const path& to, const path& new_symlink)
{
    std::error_code ec;
    create_symlink(to, new_symlink, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), to, new_symlink, ec);
    }
}

GHC_INLINE void create_symlink(const path& to, const path& new_symlink, std::error_code& ec) noexcept
{
    detail::create_symlink(to, new_symlink, false, ec);
}

GHC_INLINE path current_path()
{
    std::error_code ec;
    auto result = current_path(ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), ec);
    }
    return result;
}

GHC_INLINE path current_path(std::error_code& ec)
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    DWORD pathlen = ::GetCurrentDirectoryW(0, 0);
    std::unique_ptr<wchar_t[]> buffer(new wchar_t[pathlen + 1]);
    if (::GetCurrentDirectoryW(pathlen, buffer.get()) == 0) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return path();
    }
    return path(std::wstring(buffer.get()), path::native_format);
#else
    size_t pathlen = static_cast<size_t>(std::max(int(::pathconf(".", _PC_PATH_MAX)), int(PATH_MAX)));
    std::unique_ptr<char[]> buffer(new char[pathlen + 1]);
    if (::getcwd(buffer.get(), pathlen) == NULL) {
        ec = std::error_code(errno, std::system_category());
        return path();
    }
    return path(buffer.get());
#endif
}

GHC_INLINE void current_path(const path& p)
{
    std::error_code ec;
    current_path(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
}

GHC_INLINE void current_path(const path& p, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    if (!::SetCurrentDirectoryW(detail::fromUtf8<std::wstring>(p.u8string()).c_str())) {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
#else
    if (::chdir(p.string().c_str()) == -1) {
        ec = std::error_code(errno, std::system_category());
    }
#endif
}

GHC_INLINE bool exists(file_status s) noexcept
{
    return status_known(s) && s.type() != file_type::not_found;
}

GHC_INLINE bool exists(const path& p)
{
    return exists(status(p));
}

GHC_INLINE bool exists(const path& p, std::error_code& ec) noexcept
{
    file_status s = status(p, ec);
    if (status_known(s)) {
        ec.clear();
    }
    return exists(s);
}

GHC_INLINE bool equivalent(const path& p1, const path& p2)
{
    std::error_code ec;
    bool result = equivalent(p1, p2, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p1, p2, ec);
    }
    return result;
}

GHC_INLINE bool equivalent(const path& p1, const path& p2, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    std::shared_ptr<void> file1(::CreateFileW(p1.wstring().c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0), CloseHandle);
    auto e1 = ::GetLastError();
    std::shared_ptr<void> file2(::CreateFileW(p2.wstring().c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0), CloseHandle);
    if (file1.get() == INVALID_HANDLE_VALUE || file2.get() == INVALID_HANDLE_VALUE) {
#ifdef LWG_2937_BEHAVIOUR
        ec = std::error_code(e1 ? e1 : ::GetLastError(), std::system_category());
#else
        if (file1 == file2) {
            ec = std::error_code(e1 ? e1 : ::GetLastError(), std::system_category());
        }
#endif
        return false;
    }
    BY_HANDLE_FILE_INFORMATION inf1, inf2;
    if (!::GetFileInformationByHandle(file1.get(), &inf1)) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return false;
    }
    if (!::GetFileInformationByHandle(file2.get(), &inf2)) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return false;
    }
    return inf1.ftLastWriteTime.dwLowDateTime == inf2.ftLastWriteTime.dwLowDateTime && inf1.ftLastWriteTime.dwHighDateTime == inf2.ftLastWriteTime.dwHighDateTime && inf1.nFileIndexHigh == inf2.nFileIndexHigh && inf1.nFileIndexLow == inf2.nFileIndexLow &&
           inf1.nFileSizeHigh == inf2.nFileSizeHigh && inf1.nFileSizeLow == inf2.nFileSizeLow && inf1.dwVolumeSerialNumber == inf2.dwVolumeSerialNumber;
#else
    struct ::stat s1, s2;
    auto rc1 = ::stat(p1.c_str(), &s1);
    auto e1 = errno;
    auto rc2 = ::stat(p2.c_str(), &s2);
    if (rc1 || rc2) {
#ifdef LWG_2937_BEHAVIOUR
        ec = std::error_code(e1 ? e1 : errno, std::system_category());
#else
        if (rc1 && rc2) {
            ec = std::error_code(e1 ? e1 : errno, std::system_category());
        }
#endif
        return false;
    }
    return s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino && s1.st_size == s2.st_size && s1.st_mtime == s2.st_mtime;
#endif
}

GHC_INLINE uintmax_t file_size(const path& p)
{
    std::error_code ec;
    auto result = file_size(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE uintmax_t file_size(const path& p, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExW(detail::fromUtf8<std::wstring>(p.u8string()).c_str(), GetFileExInfoStandard, &attr)) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return static_cast<uintmax_t>(-1);
    }
    return static_cast<uintmax_t>(attr.nFileSizeHigh) << (sizeof(attr.nFileSizeHigh) * 8) | attr.nFileSizeLow;
#else
    struct ::stat fileStat;
    if (::stat(p.c_str(), &fileStat) == -1) {
        ec = std::error_code(errno, std::system_category());
        return static_cast<uintmax_t>(-1);
    }
    return static_cast<uintmax_t>(fileStat.st_size);
#endif
}

GHC_INLINE uintmax_t hard_link_count(const path& p)
{
    std::error_code ec;
    auto result = hard_link_count(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE uintmax_t hard_link_count(const path& p, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    uintmax_t result = static_cast<uintmax_t>(-1);
    std::shared_ptr<void> file(::CreateFileW(p.wstring().c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0), CloseHandle);
    BY_HANDLE_FILE_INFORMATION inf;
    if (file.get() == INVALID_HANDLE_VALUE) {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
    else {
        if (!::GetFileInformationByHandle(file.get(), &inf)) {
            ec = std::error_code(::GetLastError(), std::system_category());
        }
        else {
            result = inf.nNumberOfLinks;
        }
    }
    return result;
#else
    uintmax_t result = 0;
    file_status fs = detail::status_ex(p, ec, nullptr, nullptr, &result, nullptr);
    if (fs.type() == file_type::not_found) {
        ec = detail::make_error_code(detail::portable_error::not_found);
    }
    return ec ? static_cast<uintmax_t>(-1) : result;
#endif
}

GHC_INLINE bool is_block_file(file_status s) noexcept
{
    return s.type() == file_type::block;
}

GHC_INLINE bool is_block_file(const path& p)
{
    return is_block_file(status(p));
}

GHC_INLINE bool is_block_file(const path& p, std::error_code& ec) noexcept
{
    return is_block_file(status(p, ec));
}

GHC_INLINE bool is_character_file(file_status s) noexcept
{
    return s.type() == file_type::character;
}

GHC_INLINE bool is_character_file(const path& p)
{
    return is_character_file(status(p));
}

GHC_INLINE bool is_character_file(const path& p, std::error_code& ec) noexcept
{
    return is_character_file(status(p, ec));
}

GHC_INLINE bool is_directory(file_status s) noexcept
{
    return s.type() == file_type::directory;
}

GHC_INLINE bool is_directory(const path& p)
{
    return is_directory(status(p));
}

GHC_INLINE bool is_directory(const path& p, std::error_code& ec) noexcept
{
    return is_directory(status(p, ec));
}

GHC_INLINE bool is_empty(const path& p)
{
    if (is_directory(p)) {
        return directory_iterator(p) == directory_iterator();
    }
    else {
        return file_size(p) == 0;
    }
}

GHC_INLINE bool is_empty(const path& p, std::error_code& ec) noexcept
{
    auto fs = status(p, ec);
    if (ec) {
        return false;
    }
    if (is_directory(fs)) {
        directory_iterator iter(p, ec);
        if (ec) {
            return false;
        }
        return iter == directory_iterator();
    }
    else {
        auto sz = file_size(p, ec);
        if (ec) {
            return false;
        }
        return sz == 0;
    }
}

GHC_INLINE bool is_fifo(file_status s) noexcept
{
    return s.type() == file_type::fifo;
}

GHC_INLINE bool is_fifo(const path& p)
{
    return is_fifo(status(p));
}

GHC_INLINE bool is_fifo(const path& p, std::error_code& ec) noexcept
{
    return is_fifo(status(p, ec));
}

GHC_INLINE bool is_other(file_status s) noexcept
{
    return exists(s) && !is_regular_file(s) && !is_directory(s) && !is_symlink(s);
}

GHC_INLINE bool is_other(const path& p)
{
    return is_other(status(p));
}

GHC_INLINE bool is_other(const path& p, std::error_code& ec) noexcept
{
    return is_other(status(p, ec));
}

GHC_INLINE bool is_regular_file(file_status s) noexcept
{
    return s.type() == file_type::regular;
}

GHC_INLINE bool is_regular_file(const path& p)
{
    return is_regular_file(status(p));
}

GHC_INLINE bool is_regular_file(const path& p, std::error_code& ec) noexcept
{
    return is_regular_file(status(p, ec));
}

GHC_INLINE bool is_socket(file_status s) noexcept
{
    return s.type() == file_type::socket;
}

GHC_INLINE bool is_socket(const path& p)
{
    return is_socket(status(p));
}

GHC_INLINE bool is_socket(const path& p, std::error_code& ec) noexcept
{
    return is_socket(status(p, ec));
}

GHC_INLINE bool is_symlink(file_status s) noexcept
{
    return s.type() == file_type::symlink;
}

GHC_INLINE bool is_symlink(const path& p)
{
    return is_symlink(symlink_status(p));
}

GHC_INLINE bool is_symlink(const path& p, std::error_code& ec) noexcept
{
    return is_symlink(symlink_status(p, ec));
}

GHC_INLINE file_time_type last_write_time(const path& p)
{
    std::error_code ec;
    auto result = last_write_time(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE file_time_type last_write_time(const path& p, std::error_code& ec) noexcept
{
    time_t result = 0;
    ec.clear();
    file_status fs = detail::status_ex(p, ec, nullptr, nullptr, nullptr, &result);
    return ec ? (file_time_type::min)() : std::chrono::system_clock::from_time_t(result);
}

GHC_INLINE void last_write_time(const path& p, file_time_type new_time)
{
    std::error_code ec;
    last_write_time(p, new_time, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
}

GHC_INLINE void last_write_time(const path& p, file_time_type new_time, std::error_code& ec) noexcept
{
    ec.clear();
    auto d = new_time.time_since_epoch();
#ifdef GHC_OS_WINDOWS
    std::shared_ptr<void> file(::CreateFileW(p.wstring().c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL), ::CloseHandle);
    FILETIME ft;
    auto tt = std::chrono::duration_cast<std::chrono::microseconds>(d).count() * 10 + 116444736000000000;
    ft.dwLowDateTime = (unsigned long)tt;
    ft.dwHighDateTime = tt >> 32;
    if (!::SetFileTime(file.get(), 0, 0, &ft)) {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
#elif defined(GHC_OS_MACOS)
#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 101300
    struct ::stat fs;
    if (::stat(p.c_str(), &fs) == 0) {
        struct ::timeval tv[2];
        tv[0].tv_sec = fs.st_atimespec.tv_sec;
        tv[0].tv_usec = static_cast<int>(fs.st_atimespec.tv_nsec / 1000);
        tv[1].tv_sec = std::chrono::duration_cast<std::chrono::seconds>(d).count();
        tv[1].tv_usec = static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(d).count() % 1000000);
        if (::utimes(p.c_str(), tv) == 0) {
            return;
        }
    }
    ec = std::error_code(errno, std::system_category());
    return;
#else
    struct ::timespec times[2];
    times[0].tv_sec = 0;
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = std::chrono::duration_cast<std::chrono::seconds>(d).count();
    times[1].tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count() % 1000000000;
    if (::utimensat(AT_FDCWD, p.c_str(), times, AT_SYMLINK_NOFOLLOW) != 0) {
        ec = std::error_code(errno, std::system_category());
    }
    return;
#endif
#endif
#else
    struct ::timespec times[2];
    times[0].tv_sec = 0;
    times[0].tv_nsec = UTIME_OMIT;
    times[1].tv_sec = std::chrono::duration_cast<std::chrono::seconds>(d).count();
    times[1].tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count() % 1000000000;
    if (::utimensat(AT_FDCWD, p.c_str(), times, AT_SYMLINK_NOFOLLOW) != 0) {
        ec = std::error_code(errno, std::system_category());
    }
    return;
#endif
}

GHC_INLINE void permissions(const path& p, perms prms, perm_options opts)
{
    std::error_code ec;
    permissions(p, prms, opts, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
}

GHC_INLINE void permissions(const path& p, perms prms, std::error_code& ec) noexcept
{
    permissions(p, prms, perm_options::replace, ec);
}

GHC_INLINE void permissions(const path& p, perms prms, perm_options opts, std::error_code& ec)
{
    if (static_cast<int>(opts & (perm_options::replace | perm_options::add | perm_options::remove)) == 0) {
        ec = detail::make_error_code(detail::portable_error::invalid_argument);
        return;
    }
    auto fs = symlink_status(p, ec);
    if ((opts & perm_options::replace) != perm_options::replace) {
        if ((opts & perm_options::add) == perm_options::add) {
            prms = fs.permissions() | prms;
        }
        else {
            prms = fs.permissions() & ~prms;
        }
    }
#ifdef GHC_OS_WINDOWS
#  ifdef __GNUC__
    auto oldAttr = GetFileAttributesW(p.wstring().c_str());
    if (oldAttr != INVALID_FILE_ATTRIBUTES) {
        DWORD newAttr = ((prms & perms::owner_write) == perms::owner_write) ? oldAttr & ~FILE_ATTRIBUTE_READONLY : oldAttr | FILE_ATTRIBUTE_READONLY;
        if (oldAttr == newAttr || SetFileAttributesW(p.wstring().c_str(), newAttr)) {
            return;
        }
    }
    ec = std::error_code(::GetLastError(), std::system_category());
#  else
    int mode = 0;
    if ((prms & perms::owner_read) == perms::owner_read) {
        mode |= _S_IREAD;
    }
    if ((prms & perms::owner_write) == perms::owner_write) {
        mode |= _S_IWRITE;
    }
    if (::_wchmod(p.wstring().c_str(), mode) != 0) {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
#  endif
#else
    if ((opts & perm_options::nofollow) != perm_options::nofollow) {
        if (::chmod(p.c_str(), static_cast<mode_t>(prms)) != 0) {
            ec = std::error_code(errno, std::system_category());
        }
    }
#endif
}

GHC_INLINE path proximate(const path& p, std::error_code& ec)
{
    return proximate(p, current_path(), ec);
}

GHC_INLINE path proximate(const path& p, const path& base)
{
    return weakly_canonical(p).lexically_proximate(weakly_canonical(base));
}

GHC_INLINE path proximate(const path& p, const path& base, std::error_code& ec)
{
    return weakly_canonical(p, ec).lexically_proximate(weakly_canonical(base, ec));
}

GHC_INLINE path read_symlink(const path& p)
{
    std::error_code ec;
    auto result = read_symlink(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE path read_symlink(const path& p, std::error_code& ec)
{
    file_status fs = symlink_status(p, ec);
    if (fs.type() != file_type::symlink) {
        ec = detail::make_error_code(detail::portable_error::invalid_argument);
        return path();
    }
    auto result = detail::resolveSymlink(p, ec);
    return ec ? path() : result;
}

GHC_INLINE path relative(const path& p, std::error_code& ec)
{
    return relative(p, current_path(ec), ec);
}

GHC_INLINE path relative(const path& p, const path& base)
{
    return weakly_canonical(p).lexically_relative(weakly_canonical(base));
}

GHC_INLINE path relative(const path& p, const path& base, std::error_code& ec)
{
    return weakly_canonical(p, ec).lexically_relative(weakly_canonical(base, ec));
}

GHC_INLINE bool remove(const path& p)
{
    std::error_code ec;
    auto result = remove(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE bool remove(const path& p, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    std::wstring np = detail::fromUtf8<std::wstring>(p.u8string());
    DWORD attr = GetFileAttributesW(np.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        auto error = ::GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            return false;
        }
        ec = std::error_code(error, std::system_category());
    }
    if (!ec) {
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            if (!RemoveDirectoryW(np.c_str())) {
                ec = std::error_code(::GetLastError(), std::system_category());
            }
        }
        else {
            if (!DeleteFileW(np.c_str())) {
                ec = std::error_code(::GetLastError(), std::system_category());
            }
        }
    }
#else
    if (::remove(p.c_str()) == -1) {
        auto error = errno;
        if (error == ENOENT) {
            return false;
        }
        ec = std::error_code(errno, std::system_category());
    }
#endif
    return ec ? false : true;
}

GHC_INLINE uintmax_t remove_all(const path& p)
{
    std::error_code ec;
    auto result = remove_all(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE uintmax_t remove_all(const path& p, std::error_code& ec) noexcept
{
    ec.clear();
    uintmax_t count = 0;
    if (p == "/") {
        ec = detail::make_error_code(detail::portable_error::not_supported);
        return static_cast<uintmax_t>(-1);
    }
    std::error_code tec;
    auto fs = status(p, tec);
    if (exists(fs) && is_directory(fs)) {
        for (auto iter = directory_iterator(p, ec); iter != directory_iterator(); iter.increment(ec)) {
            if (ec) {
                break;
            }
            if (!iter->is_symlink() && iter->is_directory()) {
                count += remove_all(iter->path(), ec);
                if (ec) {
                    return static_cast<uintmax_t>(-1);
                }
            }
            else {
                remove(iter->path(), ec);
                if (ec) {
                    return static_cast<uintmax_t>(-1);
                }
                ++count;
            }
        }
    }
    if (!ec) {
        if (remove(p, ec)) {
            ++count;
        }
    }
    if (ec) {
        return static_cast<uintmax_t>(-1);
    }
    return count;
}

GHC_INLINE void rename(const path& from, const path& to)
{
    std::error_code ec;
    rename(from, to, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), from, to, ec);
    }
}

GHC_INLINE void rename(const path& from, const path& to, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    if (from != to) {
        if (!MoveFileW(detail::fromUtf8<std::wstring>(from.u8string()).c_str(), detail::fromUtf8<std::wstring>(to.u8string()).c_str())) {
            ec = std::error_code(::GetLastError(), std::system_category());
        }
    }
#else
    if (from != to) {
        if (::rename(from.c_str(), to.c_str()) != 0) {
            ec = std::error_code(errno, std::system_category());
        }
    }
#endif
}

GHC_INLINE void resize_file(const path& p, uintmax_t size)
{
    std::error_code ec;
    resize_file(p, size, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
}

GHC_INLINE void resize_file(const path& p, uintmax_t size, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    LARGE_INTEGER lisize;
    lisize.QuadPart = size;
    std::shared_ptr<void> file(CreateFileW(detail::fromUtf8<std::wstring>(p.u8string()).c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL), CloseHandle);
    if (file.get() == INVALID_HANDLE_VALUE) {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
    else if (SetFilePointerEx(file.get(), lisize, NULL, FILE_BEGIN) == 0 || SetEndOfFile(file.get()) == 0) {
        ec = std::error_code(::GetLastError(), std::system_category());
    }
#else
    if (::truncate(p.c_str(), size) != 0) {
        ec = std::error_code(errno, std::system_category());
    }
#endif
}

GHC_INLINE space_info space(const path& p)
{
    std::error_code ec;
    auto result = space(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE space_info space(const path& p, std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    ULARGE_INTEGER freeBytesAvailableToCaller = {0};
    ULARGE_INTEGER totalNumberOfBytes = {0};
    ULARGE_INTEGER totalNumberOfFreeBytes = {0};
    if (!GetDiskFreeSpaceExW(detail::fromUtf8<std::wstring>(p.u8string()).c_str(), &freeBytesAvailableToCaller, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return {static_cast<uintmax_t>(-1), static_cast<uintmax_t>(-1), static_cast<uintmax_t>(-1)};
    }
    return {static_cast<uintmax_t>(totalNumberOfBytes.QuadPart), static_cast<uintmax_t>(totalNumberOfFreeBytes.QuadPart), static_cast<uintmax_t>(freeBytesAvailableToCaller.QuadPart)};
#elif !defined(__ANDROID__) || __ANDROID_API__ >= 19
    struct ::statvfs sfs;
    if (::statvfs(p.c_str(), &sfs) != 0) {
        ec = std::error_code(errno, std::system_category());
        return {static_cast<uintmax_t>(-1), static_cast<uintmax_t>(-1), static_cast<uintmax_t>(-1)};
    }
    return {static_cast<uintmax_t>(sfs.f_blocks * sfs.f_frsize), static_cast<uintmax_t>(sfs.f_bfree * sfs.f_frsize), static_cast<uintmax_t>(sfs.f_bavail * sfs.f_frsize)};
#else
    ec = detail::make_error_code(detail::portable_error::not_supported);
    return {static_cast<uintmax_t>(-1), static_cast<uintmax_t>(-1), static_cast<uintmax_t>(-1)};
#endif
}

GHC_INLINE file_status status(const path& p)
{
    std::error_code ec;
    auto result = status(p, ec);
    if (result.type() == file_type::none) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE file_status status(const path& p, std::error_code& ec) noexcept
{
    return detail::status_ex(p, ec);
}

GHC_INLINE bool status_known(file_status s) noexcept
{
    return s.type() != file_type::none;
}

GHC_INLINE file_status symlink_status(const path& p)
{
    std::error_code ec;
    auto result = symlink_status(p, ec);
    if (result.type() == file_type::none) {
        throw filesystem_error(detail::systemErrorText(ec.value()), ec);
    }
    return result;
}

GHC_INLINE file_status symlink_status(const path& p, std::error_code& ec) noexcept
{
    return detail::symlink_status_ex(p, ec);
}

GHC_INLINE path temp_directory_path()
{
    std::error_code ec;
    path result = temp_directory_path(ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), ec);
    }
    return result;
}

GHC_INLINE path temp_directory_path(std::error_code& ec) noexcept
{
    ec.clear();
#ifdef GHC_OS_WINDOWS
    wchar_t buffer[512];
    int rc = GetTempPathW(511, buffer);
    if (!rc || rc > 511) {
        ec = std::error_code(::GetLastError(), std::system_category());
        return path();
    }
    return path(std::wstring(buffer));
#else
    static const char* temp_vars[] = {"TMPDIR", "TMP", "TEMP", "TEMPDIR", nullptr};
    const char* temp_path = nullptr;
    for (auto temp_name = temp_vars; *temp_name != nullptr; ++temp_name) {
        temp_path = std::getenv(*temp_name);
        if (temp_path) {
            return path(temp_path);
        }
    }
    return path("/tmp");
#endif
}

GHC_INLINE path weakly_canonical(const path& p)
{
    std::error_code ec;
    auto result = weakly_canonical(p, ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), p, ec);
    }
    return result;
}

GHC_INLINE path weakly_canonical(const path& p, std::error_code& ec) noexcept
{
    path result;
    ec.clear();
    bool scan = true;
    for (auto pe : p) {
        if (scan) {
            std::error_code tec;
            if (exists(result / pe, tec)) {
                result /= pe;
            }
            else {
                if (ec) {
                    return path();
                }
                scan = false;
                if (!result.empty()) {
                    result = canonical(result, ec) / pe;
                    if (ec) {
                        break;
                    }
                }
                else {
                    result /= pe;
                }
            }
        }
        else {
            result /= pe;
        }
    }
    if (scan) {
        if (!result.empty()) {
            result = canonical(result, ec);
        }
    }
    return ec ? path() : result.lexically_normal();
}

//-----------------------------------------------------------------------------
// 30.10.11 class file_status
// 30.10.11.1 constructors and destructor
GHC_INLINE file_status::file_status() noexcept
    : file_status(file_type::none)
{
}

GHC_INLINE file_status::file_status(file_type ft, perms prms) noexcept
    : _type(ft)
    , _perms(prms)
{
}

GHC_INLINE file_status::file_status(const file_status& other) noexcept
    : _type(other._type)
    , _perms(other._perms)
{
}

GHC_INLINE file_status::file_status(file_status&& other) noexcept
    : _type(other._type)
    , _perms(other._perms)
{
}

GHC_INLINE file_status::~file_status() {}

// assignments:
GHC_INLINE file_status& file_status::operator=(const file_status& rhs) noexcept
{
    _type = rhs._type;
    _perms = rhs._perms;
    return *this;
}

GHC_INLINE file_status& file_status::operator=(file_status&& rhs) noexcept
{
    _type = rhs._type;
    _perms = rhs._perms;
    return *this;
}

// 30.10.11.3 modifiers
GHC_INLINE void file_status::type(file_type ft) noexcept
{
    _type = ft;
}

GHC_INLINE void file_status::permissions(perms prms) noexcept
{
    _perms = prms;
}

// 30.10.11.2 observers
GHC_INLINE file_type file_status::type() const noexcept
{
    return _type;
}

GHC_INLINE perms file_status::permissions() const noexcept
{
    return _perms;
}

//-----------------------------------------------------------------------------
// 30.10.12 class directory_entry
// 30.10.12.1 constructors and destructor
// directory_entry::directory_entry() noexcept = default;
// directory_entry::directory_entry(const directory_entry&) = default;
// directory_entry::directory_entry(directory_entry&&) noexcept = default;
GHC_INLINE directory_entry::directory_entry(const filesystem::path& p)
    : _path(p)
    , _file_size(0)
#ifndef GHC_OS_WINDOWS
    , _hard_link_count(0)
#endif
    , _last_write_time(0)
{
    refresh();
}

GHC_INLINE directory_entry::directory_entry(const filesystem::path& p, std::error_code& ec)
    : _path(p)
    , _file_size(0)
#ifndef GHC_OS_WINDOWS
    , _hard_link_count(0)
#endif
    , _last_write_time(0)
{
    refresh(ec);
}

GHC_INLINE directory_entry::~directory_entry() {}

// assignments:
// directory_entry& directory_entry::operator=(const directory_entry&) = default;
// directory_entry& directory_entry::operator=(directory_entry&&) noexcept = default;

// 30.10.12.2 directory_entry modifiers
GHC_INLINE void directory_entry::assign(const filesystem::path& p)
{
    _path = p;
    refresh();
}

GHC_INLINE void directory_entry::assign(const filesystem::path& p, std::error_code& ec)
{
    _path = p;
    refresh(ec);
}

GHC_INLINE void directory_entry::replace_filename(const filesystem::path& p)
{
    _path.replace_filename(p);
    refresh();
}

GHC_INLINE void directory_entry::replace_filename(const filesystem::path& p, std::error_code& ec)
{
    _path.replace_filename(p);
    refresh(ec);
}

GHC_INLINE void directory_entry::refresh()
{
    std::error_code ec;
    refresh(ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), _path, ec);
    }
}

GHC_INLINE void directory_entry::refresh(std::error_code& ec) noexcept
{
#ifdef GHC_OS_WINDOWS
    _status = detail::status_ex(_path, ec, &_symlink_status, &_file_size, nullptr, &_last_write_time);
#else
    _status = detail::status_ex(_path, ec, &_symlink_status, &_file_size, &_hard_link_count, &_last_write_time);
#endif
}

// 30.10.12.3 directory_entry observers
GHC_INLINE const filesystem::path& directory_entry::path() const noexcept
{
    return _path;
}

GHC_INLINE directory_entry::operator const filesystem::path&() const noexcept
{
    return _path;
}

GHC_INLINE bool directory_entry::exists() const
{
    return filesystem::exists(status());
}

GHC_INLINE bool directory_entry::exists(std::error_code& ec) const noexcept
{
    return filesystem::exists(status(ec));
}

GHC_INLINE bool directory_entry::is_block_file() const
{
    return filesystem::is_block_file(status());
}
GHC_INLINE bool directory_entry::is_block_file(std::error_code& ec) const noexcept
{
    return filesystem::is_block_file(status(ec));
}

GHC_INLINE bool directory_entry::is_character_file() const
{
    return filesystem::is_character_file(status());
}

GHC_INLINE bool directory_entry::is_character_file(std::error_code& ec) const noexcept
{
    return filesystem::is_character_file(status(ec));
}

GHC_INLINE bool directory_entry::is_directory() const
{
    return filesystem::is_directory(status());
}

GHC_INLINE bool directory_entry::is_directory(std::error_code& ec) const noexcept
{
    return filesystem::is_directory(status(ec));
}

GHC_INLINE bool directory_entry::is_fifo() const
{
    return filesystem::is_fifo(status());
}

GHC_INLINE bool directory_entry::is_fifo(std::error_code& ec) const noexcept
{
    return filesystem::is_fifo(status(ec));
}

GHC_INLINE bool directory_entry::is_other() const
{
    return filesystem::is_other(status());
}

GHC_INLINE bool directory_entry::is_other(std::error_code& ec) const noexcept
{
    return filesystem::is_other(status(ec));
}

GHC_INLINE bool directory_entry::is_regular_file() const
{
    return filesystem::is_regular_file(status());
}

GHC_INLINE bool directory_entry::is_regular_file(std::error_code& ec) const noexcept
{
    return filesystem::is_regular_file(status(ec));
}

GHC_INLINE bool directory_entry::is_socket() const
{
    return filesystem::is_socket(status());
}

GHC_INLINE bool directory_entry::is_socket(std::error_code& ec) const noexcept
{
    return filesystem::is_socket(status(ec));
}

GHC_INLINE bool directory_entry::is_symlink() const
{
    return filesystem::is_symlink(symlink_status());
}

GHC_INLINE bool directory_entry::is_symlink(std::error_code& ec) const noexcept
{
    return filesystem::is_symlink(symlink_status(ec));
}

GHC_INLINE uintmax_t directory_entry::file_size() const
{
    if (_status.type() != file_type::none) {
        return _file_size;
    }
    return filesystem::file_size(path());
}

GHC_INLINE uintmax_t directory_entry::file_size(std::error_code& ec) const noexcept
{
    if (_status.type() != file_type::none) {
        return _file_size;
    }
    return filesystem::file_size(path(), ec);
}

GHC_INLINE uintmax_t directory_entry::hard_link_count() const
{
#ifndef GHC_OS_WINDOWS
    if (_status.type() != file_type::none) {
        return _hard_link_count;
    }
#endif
    return filesystem::hard_link_count(path());
}

GHC_INLINE uintmax_t directory_entry::hard_link_count(std::error_code& ec) const noexcept
{
#ifndef GHC_OS_WINDOWS
    if (_status.type() != file_type::none) {
        return _hard_link_count;
    }
#endif
    return filesystem::hard_link_count(path(), ec);
}

GHC_INLINE file_time_type directory_entry::last_write_time() const
{
    if (_status.type() != file_type::none) {
        return std::chrono::system_clock::from_time_t(_last_write_time);
    }
    return filesystem::last_write_time(path());
}

GHC_INLINE file_time_type directory_entry::last_write_time(std::error_code& ec) const noexcept
{
    if (_status.type() != file_type::none) {
        return std::chrono::system_clock::from_time_t(_last_write_time);
    }
    return filesystem::last_write_time(path(), ec);
}

GHC_INLINE file_status directory_entry::status() const
{
    if (_status.type() != file_type::none) {
        return _status;
    }
    return filesystem::status(path());
}

GHC_INLINE file_status directory_entry::status(std::error_code& ec) const noexcept
{
    if (_status.type() != file_type::none) {
        return _status;
    }
    return filesystem::status(path(), ec);
}

GHC_INLINE file_status directory_entry::symlink_status() const
{
    if (_symlink_status.type() != file_type::none) {
        return _symlink_status;
    }
    return filesystem::symlink_status(path());
}

GHC_INLINE file_status directory_entry::symlink_status(std::error_code& ec) const noexcept
{
    if (_symlink_status.type() != file_type::none) {
        return _symlink_status;
    }
    return filesystem::symlink_status(path(), ec);
}

GHC_INLINE bool directory_entry::operator<(const directory_entry& rhs) const noexcept
{
    return _path < rhs._path;
}

GHC_INLINE bool directory_entry::operator==(const directory_entry& rhs) const noexcept
{
    return _path == rhs._path;
}

GHC_INLINE bool directory_entry::operator!=(const directory_entry& rhs) const noexcept
{
    return _path != rhs._path;
}

GHC_INLINE bool directory_entry::operator<=(const directory_entry& rhs) const noexcept
{
    return _path <= rhs._path;
}

GHC_INLINE bool directory_entry::operator>(const directory_entry& rhs) const noexcept
{
    return _path > rhs._path;
}

GHC_INLINE bool directory_entry::operator>=(const directory_entry& rhs) const noexcept
{
    return _path >= rhs._path;
}

//-----------------------------------------------------------------------------
// 30.10.13 class directory_iterator

#ifdef GHC_OS_WINDOWS
class directory_iterator::impl
{
public:
    impl(const path& p, directory_options options)
        : _base(p)
        , _options(options)
        , _dirHandle(_base.empty() ? INVALID_HANDLE_VALUE : FindFirstFileW(detail::fromUtf8<std::wstring>((_base / "*").u8string()).c_str(), &_findData))
        , _current(_dirHandle != INVALID_HANDLE_VALUE ? _base / std::wstring(_findData.cFileName) : filesystem::path())
    {
        if (_dirHandle == INVALID_HANDLE_VALUE && !p.empty()) {
            auto error = ::GetLastError();
            _base = filesystem::path();
            if (error != ERROR_ACCESS_DENIED || (options & directory_options::skip_permission_denied) == directory_options::none) {
                _ec = std::error_code(::GetLastError(), std::system_category());
            }
        }
        else {
            if (std::wstring(_findData.cFileName) == L"." || std::wstring(_findData.cFileName) == L"..") {
                increment(_ec);
            }
            else {
                copyToDirEntry(_ec);
            }
        }
    }
    impl(const impl& other) = delete;
    ~impl()
    {
        if (_dirHandle != INVALID_HANDLE_VALUE) {
            FindClose(_dirHandle);
            _dirHandle = INVALID_HANDLE_VALUE;
        }
    }
    void increment(std::error_code& ec)
    {
        if (_dirHandle != INVALID_HANDLE_VALUE) {
            do {
                if (FindNextFileW(_dirHandle, &_findData)) {
                    _current = _base;
                    _current.append_name(detail::toUtf8(_findData.cFileName).c_str());
                    copyToDirEntry(ec);
                }
                else {
                    FindClose(_dirHandle);
                    _dirHandle = INVALID_HANDLE_VALUE;
                    _current = filesystem::path();
                    break;
                }
            } while (std::wstring(_findData.cFileName) == L"." || std::wstring(_findData.cFileName) == L"..");
        }
        else {
            ec = _ec;
        }
    }
    void copyToDirEntry(std::error_code& ec)
    {
        _dir_entry._path = _current;
        if (_findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            _dir_entry._status = detail::status_ex(_current, ec, &_dir_entry._symlink_status, &_dir_entry._file_size, nullptr, &_dir_entry._last_write_time);
        }
        else {
            _dir_entry._status = detail::status_from_INFO(_current, &_findData, ec, &_dir_entry._file_size, &_dir_entry._last_write_time);
            _dir_entry._symlink_status = _dir_entry._status;
        }
        if (ec) {
            if (_dir_entry._status.type() != file_type::none && _dir_entry._symlink_status.type() != file_type::none) {
                ec.clear();
            }
            else {
                _dir_entry._file_size = static_cast<uintmax_t>(-1);
                _dir_entry._last_write_time = 0;
            }
        }
    }
    path _base;
    directory_options _options;
    WIN32_FIND_DATAW _findData;
    HANDLE _dirHandle;
    path _current;
    directory_entry _dir_entry;
    std::error_code _ec;
};
#else
// POSIX implementation
class directory_iterator::impl
{
public:
    impl(const path& path, directory_options options)
        : _base(path)
        , _options(options)
        , _dir(nullptr)
        , _entry(nullptr)
    {
        if (!path.empty()) {
            _dir = ::opendir(path.native().c_str());
        }
        if (!path.empty()) {
            if (!_dir) {
                auto error = errno;
                _base = filesystem::path();
                if (error != EACCES || (options & directory_options::skip_permission_denied) == directory_options::none) {
                    _ec = std::error_code(errno, std::system_category());
                }
            }
            else {
                increment(_ec);
            }
        }
    }
    impl(const impl& other) = delete;
    ~impl()
    {
        if (_dir) {
            ::closedir(_dir);
        }
    }
    void increment(std::error_code& ec)
    {
        if (_dir) {
            do {
                errno = 0;
                _entry = readdir(_dir);
                if (_entry) {
                    _current = _base;
                    _current.append_name(_entry->d_name);
                    _dir_entry = directory_entry(_current, ec);
                }
                else {
                    ::closedir(_dir);
                    _dir = nullptr;
                    _current = path();
                    if (errno) {
                        ec = std::error_code(errno, std::system_category());
                    }
                    break;
                }
            } while (std::strcmp(_entry->d_name, ".") == 0 || std::strcmp(_entry->d_name, "..") == 0);
        }
    }
    path _base;
    directory_options _options;
    path _current;
    DIR* _dir;
    struct ::dirent* _entry;
    directory_entry _dir_entry;
    std::error_code _ec;
};
#endif

// 30.10.13.1 member functions
GHC_INLINE directory_iterator::directory_iterator() noexcept
    : _impl(new impl(path(), directory_options::none))
{
}

GHC_INLINE directory_iterator::directory_iterator(const path& p)
    : _impl(new impl(p, directory_options::none))
{
    if (_impl->_ec) {
        throw filesystem_error(detail::systemErrorText(_impl->_ec.value()), p, _impl->_ec);
    }
    _impl->_ec.clear();
}

GHC_INLINE directory_iterator::directory_iterator(const path& p, directory_options options)
    : _impl(new impl(p, options))
{
    if (_impl->_ec) {
        throw filesystem_error(detail::systemErrorText(_impl->_ec.value()), p, _impl->_ec);
    }
}

GHC_INLINE directory_iterator::directory_iterator(const path& p, std::error_code& ec) noexcept
    : _impl(new impl(p, directory_options::none))
{
    if (_impl->_ec) {
        ec = _impl->_ec;
    }
}

GHC_INLINE directory_iterator::directory_iterator(const path& p, directory_options options, std::error_code& ec) noexcept
    : _impl(new impl(p, options))
{
    if (_impl->_ec) {
        ec = _impl->_ec;
    }
}

GHC_INLINE directory_iterator::directory_iterator(const directory_iterator& rhs)
    : _impl(rhs._impl)
{
}

GHC_INLINE directory_iterator::directory_iterator(directory_iterator&& rhs) noexcept
    : _impl(std::move(rhs._impl))
{
}

GHC_INLINE directory_iterator::~directory_iterator() {}

GHC_INLINE directory_iterator& directory_iterator::operator=(const directory_iterator& rhs)
{
    _impl = rhs._impl;
    return *this;
}

GHC_INLINE directory_iterator& directory_iterator::operator=(directory_iterator&& rhs) noexcept
{
    _impl = std::move(rhs._impl);
    return *this;
}

GHC_INLINE const directory_entry& directory_iterator::operator*() const
{
    return _impl->_dir_entry;
}

GHC_INLINE const directory_entry* directory_iterator::operator->() const
{
    return &_impl->_dir_entry;
}

GHC_INLINE directory_iterator& directory_iterator::operator++()
{
    std::error_code ec;
    _impl->increment(ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), _impl->_current, ec);
    }
    return *this;
}

GHC_INLINE directory_iterator& directory_iterator::increment(std::error_code& ec) noexcept
{
    _impl->increment(ec);
    return *this;
}

GHC_INLINE bool directory_iterator::operator==(const directory_iterator& rhs) const
{
    return _impl->_current == rhs._impl->_current;
}

GHC_INLINE bool directory_iterator::operator!=(const directory_iterator& rhs) const
{
    return _impl->_current != rhs._impl->_current;
}

GHC_INLINE void directory_iterator::swap(directory_iterator& rhs)
{
    std::swap(_impl, rhs._impl);
}

// 30.10.13.2 directory_iterator non-member functions

GHC_INLINE directory_iterator begin(directory_iterator iter) noexcept
{
    return iter;
}

GHC_INLINE directory_iterator end(const directory_iterator&) noexcept
{
    return directory_iterator();
}

//-----------------------------------------------------------------------------
// 30.10.14 class recursive_directory_iterator

GHC_INLINE recursive_directory_iterator::recursive_directory_iterator() noexcept
    : _impl(new recursive_directory_iterator_impl(directory_options::none, true))
{
    _impl->_dir_iter_stack.push(directory_iterator());
}

GHC_INLINE recursive_directory_iterator::recursive_directory_iterator(const path& p)
    : _impl(new recursive_directory_iterator_impl(directory_options::none, true))
{
    _impl->_dir_iter_stack.push(directory_iterator(p));
}

GHC_INLINE recursive_directory_iterator::recursive_directory_iterator(const path& p, directory_options options)
    : _impl(new recursive_directory_iterator_impl(options, true))
{
    _impl->_dir_iter_stack.push(directory_iterator(p, options));
}

GHC_INLINE recursive_directory_iterator::recursive_directory_iterator(const path& p, directory_options options, std::error_code& ec) noexcept
    : _impl(new recursive_directory_iterator_impl(options, true))
{
    _impl->_dir_iter_stack.push(directory_iterator(p, options, ec));
}

GHC_INLINE recursive_directory_iterator::recursive_directory_iterator(const path& p, std::error_code& ec) noexcept
    : _impl(new recursive_directory_iterator_impl(directory_options::none, true))
{
    _impl->_dir_iter_stack.push(directory_iterator(p, ec));
}

GHC_INLINE recursive_directory_iterator::recursive_directory_iterator(const recursive_directory_iterator& rhs)
    : _impl(rhs._impl)
{
}

GHC_INLINE recursive_directory_iterator::recursive_directory_iterator(recursive_directory_iterator&& rhs) noexcept
    : _impl(std::move(rhs._impl))
{
}

GHC_INLINE recursive_directory_iterator::~recursive_directory_iterator() {}

// 30.10.14.1 observers
GHC_INLINE directory_options recursive_directory_iterator::options() const
{
    return _impl->_options;
}

GHC_INLINE int recursive_directory_iterator::depth() const
{
    return static_cast<int>(_impl->_dir_iter_stack.size() - 1);
}

GHC_INLINE bool recursive_directory_iterator::recursion_pending() const
{
    return _impl->_recursion_pending;
}

GHC_INLINE const directory_entry& recursive_directory_iterator::operator*() const
{
    return *(_impl->_dir_iter_stack.top());
}

GHC_INLINE const directory_entry* recursive_directory_iterator::operator->() const
{
    return &(*(_impl->_dir_iter_stack.top()));
}

// 30.10.14.1 modifiers recursive_directory_iterator&
GHC_INLINE recursive_directory_iterator& recursive_directory_iterator::operator=(const recursive_directory_iterator& rhs)
{
    _impl = rhs._impl;
    return *this;
}

GHC_INLINE recursive_directory_iterator& recursive_directory_iterator::operator=(recursive_directory_iterator&& rhs) noexcept
{
    _impl = std::move(rhs._impl);
    return *this;
}

GHC_INLINE recursive_directory_iterator& recursive_directory_iterator::operator++()
{
    std::error_code ec;
    increment(ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), _impl->_dir_iter_stack.empty() ? path() : _impl->_dir_iter_stack.top()->path(), ec);
    }
    return *this;
}

GHC_INLINE recursive_directory_iterator& recursive_directory_iterator::increment(std::error_code& ec) noexcept
{
    if (recursion_pending() && is_directory((*this)->status()) && (!is_symlink((*this)->symlink_status()) || (options() & directory_options::follow_directory_symlink) != directory_options::none)) {
        _impl->_dir_iter_stack.push(directory_iterator((*this)->path(), _impl->_options, ec));
    }
    else {
        _impl->_dir_iter_stack.top().increment(ec);
    }
    if (!ec) {
        while (depth() && _impl->_dir_iter_stack.top() == directory_iterator()) {
            _impl->_dir_iter_stack.pop();
            _impl->_dir_iter_stack.top().increment(ec);
        }
    }
    else if (!_impl->_dir_iter_stack.empty()) {
        _impl->_dir_iter_stack.pop();
    }
    _impl->_recursion_pending = true;
    return *this;
}

GHC_INLINE void recursive_directory_iterator::pop()
{
    std::error_code ec;
    pop(ec);
    if (ec) {
        throw filesystem_error(detail::systemErrorText(ec.value()), _impl->_dir_iter_stack.empty() ? path() : _impl->_dir_iter_stack.top()->path(), ec);
    }
}

GHC_INLINE void recursive_directory_iterator::pop(std::error_code& ec)
{
    if (depth() == 0) {
        *this = recursive_directory_iterator();
    }
    else {
        do {
            _impl->_dir_iter_stack.pop();
            _impl->_dir_iter_stack.top().increment(ec);
        } while (depth() && _impl->_dir_iter_stack.top() == directory_iterator());
    }
}

GHC_INLINE void recursive_directory_iterator::disable_recursion_pending()
{
    _impl->_recursion_pending = false;
}

// other members as required by 27.2.3, input iterators
GHC_INLINE bool recursive_directory_iterator::operator==(const recursive_directory_iterator& rhs) const
{
    return _impl->_dir_iter_stack.top() == rhs._impl->_dir_iter_stack.top();
}

GHC_INLINE bool recursive_directory_iterator::operator!=(const recursive_directory_iterator& rhs) const
{
    return _impl->_dir_iter_stack.top() != rhs._impl->_dir_iter_stack.top();
}

GHC_INLINE void recursive_directory_iterator::swap(recursive_directory_iterator& rhs)
{
    std::swap(_impl, rhs._impl);
}

// 30.10.14.2 directory_iterator non-member functions
GHC_INLINE recursive_directory_iterator begin(recursive_directory_iterator iter) noexcept
{
    return iter;
}

GHC_INLINE recursive_directory_iterator end(const recursive_directory_iterator&) noexcept
{
    return recursive_directory_iterator();
}

#endif  // GHC_EXPAND_IMPL

}  // namespace filesystem
}  // namespace ghc

#endif  // GHC_FILESYSTEM_H
