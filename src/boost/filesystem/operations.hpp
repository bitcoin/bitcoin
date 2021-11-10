//  boost/filesystem/operations.hpp  ---------------------------------------------------//

//  Copyright Beman Dawes 2002-2009
//  Copyright Jan Langer 2002
//  Copyright Dietmar Kuehl 2001
//  Copyright Vladimir Prus 2002
//  Copyright Andrey Semashev 2020-2021

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#ifndef BOOST_FILESYSTEM_OPERATIONS_HPP
#define BOOST_FILESYSTEM_OPERATIONS_HPP

#include <boost/filesystem/config.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/file_status.hpp>

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
// These includes are left for backward compatibility and should be included directly by users, as needed
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/directory.hpp>
#endif

#include <boost/detail/bitmask.hpp>
#include <boost/core/scoped_enum.hpp>
#include <boost/system/error_code.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <ctime>

#include <boost/filesystem/detail/header.hpp> // must be the last #include

//--------------------------------------------------------------------------------------//

namespace boost {
namespace filesystem {

struct space_info
{
    // all values are byte counts
    boost::uintmax_t capacity;
    boost::uintmax_t free;      // <= capacity
    boost::uintmax_t available; // <= free
};

BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(copy_options, unsigned int)
{
    none = 0u, // Default. For copy_file: error if the target file exists. For copy: do not recurse, follow symlinks, copy file contents.

    // copy_file options:
    skip_existing = 1u,           // Don't overwrite the existing target file, don't report an error
    overwrite_existing = 1u << 1, // Overwrite existing file
    update_existing = 1u << 2,    // Overwrite existing file if its last write time is older than the replacement file
    synchronize_data = 1u << 3,   // Flush all buffered data written to the target file to permanent storage
    synchronize = 1u << 4,        // Flush all buffered data and attributes written to the target file to permanent storage

    // copy options:
    recursive = 1u << 8,          // Recurse into sub-directories
    copy_symlinks = 1u << 9,      // Copy symlinks as symlinks instead of copying the referenced file
    skip_symlinks = 1u << 10,     // Don't copy symlinks
    directories_only = 1u << 11,  // Only copy directory structure, do not copy non-directory files
    create_symlinks = 1u << 12,   // Create symlinks instead of copying files
    create_hard_links = 1u << 13, // Create hard links instead of copying files
    _detail_recursing = 1u << 14  // Internal use only, do not use
}
BOOST_SCOPED_ENUM_DECLARE_END(copy_options)

BOOST_BITMASK(BOOST_SCOPED_ENUM_NATIVE(copy_options))

#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
BOOST_SCOPED_ENUM_DECLARE_BEGIN(copy_option)
{
    none = static_cast< unsigned int >(copy_options::none),
    fail_if_exists = none,
    overwrite_if_exists = static_cast< unsigned int >(copy_options::overwrite_existing)
}
BOOST_SCOPED_ENUM_DECLARE_END(copy_option)
#endif

//--------------------------------------------------------------------------------------//
//                             implementation details                                   //
//--------------------------------------------------------------------------------------//

namespace detail {

BOOST_FILESYSTEM_DECL
path absolute(path const& p, path const& base, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
file_status status(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
file_status symlink_status(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
bool is_empty(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path initial_path(system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path canonical(path const& p, path const& base, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void copy(path const& from, path const& to, unsigned int options, system::error_code* ec = 0);
#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
BOOST_FILESYSTEM_DECL
void copy_directory(path const& from, path const& to, system::error_code* ec = 0);
#endif
BOOST_FILESYSTEM_DECL
bool copy_file(path const& from, path const& to,                  // See ticket #2925
               unsigned int options, system::error_code* ec = 0); // see copy_options for options
BOOST_FILESYSTEM_DECL
void copy_symlink(path const& existing_symlink, path const& new_symlink, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
bool create_directories(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
bool create_directory(path const& p, const path* existing, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void create_directory_symlink(path const& to, path const& from, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void create_hard_link(path const& to, path const& from, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void create_symlink(path const& to, path const& from, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path current_path(system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void current_path(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
bool equivalent(path const& p1, path const& p2, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
boost::uintmax_t file_size(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
boost::uintmax_t hard_link_count(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
std::time_t creation_time(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
std::time_t last_write_time(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void last_write_time(path const& p, const std::time_t new_time, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void permissions(path const& p, perms prms, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path read_symlink(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path relative(path const& p, path const& base, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
bool remove(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
boost::uintmax_t remove_all(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void rename(path const& old_p, path const& new_p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
void resize_file(path const& p, uintmax_t size, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
space_info space(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path system_complete(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path temp_directory_path(system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path unique_path(path const& p, system::error_code* ec = 0);
BOOST_FILESYSTEM_DECL
path weakly_canonical(path const& p, path const& base, system::error_code* ec = 0);

} // namespace detail

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                             status query functions                                   //
//                                                                                      //
//--------------------------------------------------------------------------------------//

inline file_status status(path const& p)
{
    return detail::status(p);
}

inline file_status status(path const& p, system::error_code& ec)
{
    return detail::status(p, &ec);
}

inline file_status symlink_status(path const& p)
{
    return detail::symlink_status(p);
}

inline file_status symlink_status(path const& p, system::error_code& ec)
{
    return detail::symlink_status(p, &ec);
}

inline bool exists(path const& p)
{
    return exists(detail::status(p));
}

inline bool exists(path const& p, system::error_code& ec)
{
    return exists(detail::status(p, &ec));
}

inline bool is_directory(path const& p)
{
    return is_directory(detail::status(p));
}

inline bool is_directory(path const& p, system::error_code& ec)
{
    return is_directory(detail::status(p, &ec));
}

inline bool is_regular_file(path const& p)
{
    return is_regular_file(detail::status(p));
}

inline bool is_regular_file(path const& p, system::error_code& ec)
{
    return is_regular_file(detail::status(p, &ec));
}

inline bool is_other(path const& p)
{
    return is_other(detail::status(p));
}

inline bool is_other(path const& p, system::error_code& ec)
{
    return is_other(detail::status(p, &ec));
}

inline bool is_symlink(path const& p)
{
    return is_symlink(detail::symlink_status(p));
}

inline bool is_symlink(path const& p, system::error_code& ec)
{
    return is_symlink(detail::symlink_status(p, &ec));
}

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
inline bool is_regular(path const& p)
{
    return is_regular(detail::status(p));
}

inline bool is_regular(path const& p, system::error_code& ec)
{
    return is_regular(detail::status(p, &ec));
}
#endif

inline bool is_empty(path const& p)
{
    return detail::is_empty(p);
}

inline bool is_empty(path const& p, system::error_code& ec)
{
    return detail::is_empty(p, &ec);
}

//--------------------------------------------------------------------------------------//
//                                                                                      //
//                             operational functions                                    //
//                                                                                      //
//--------------------------------------------------------------------------------------//

inline path initial_path()
{
    return detail::initial_path();
}

inline path initial_path(system::error_code& ec)
{
    return detail::initial_path(&ec);
}

template< class Path >
path initial_path()
{
    return initial_path();
}
template< class Path >
path initial_path(system::error_code& ec)
{
    return detail::initial_path(&ec);
}

inline path current_path()
{
    return detail::current_path();
}

inline path current_path(system::error_code& ec)
{
    return detail::current_path(&ec);
}

inline void current_path(path const& p)
{
    detail::current_path(p);
}

inline void current_path(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::current_path(p, &ec);
}

inline path absolute(path const& p, path const& base = current_path())
{
    return detail::absolute(p, base);
}

inline path absolute(path const& p, system::error_code& ec)
{
    path base = current_path(ec);
    if (ec)
        return path();
    return detail::absolute(p, base, &ec);
}

inline path absolute(path const& p, path const& base, system::error_code& ec)
{
    return detail::absolute(p, base, &ec);
}

inline path canonical(path const& p, path const& base = current_path())
{
    return detail::canonical(p, base);
}

inline path canonical(path const& p, system::error_code& ec)
{
    path base = current_path(ec);
    if (ec)
        return path();
    return detail::canonical(p, base, &ec);
}

inline path canonical(path const& p, path const& base, system::error_code& ec)
{
    return detail::canonical(p, base, &ec);
}

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
inline path complete(path const& p)
{
    return absolute(p, initial_path());
}

inline path complete(path const& p, path const& base)
{
    return absolute(p, base);
}
#endif

inline void copy(path const& from, path const& to)
{
    detail::copy(from, to, static_cast< unsigned int >(copy_options::none));
}

inline void copy(path const& from, path const& to, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::copy(from, to, static_cast< unsigned int >(copy_options::none), &ec);
}

inline void copy(path const& from, path const& to, BOOST_SCOPED_ENUM_NATIVE(copy_options) options)
{
    detail::copy(from, to, static_cast< unsigned int >(options));
}

inline void copy(path const& from, path const& to, BOOST_SCOPED_ENUM_NATIVE(copy_options) options, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::copy(from, to, static_cast< unsigned int >(options), &ec);
}

#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
inline void copy_directory(path const& from, path const& to)
{
    detail::copy_directory(from, to);
}

inline void copy_directory(path const& from, path const& to, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::copy_directory(from, to, &ec);
}
#endif

inline bool copy_file(path const& from, path const& to)
{
    return detail::copy_file(from, to, static_cast< unsigned int >(copy_options::none));
}

inline bool copy_file(path const& from, path const& to, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::copy_file(from, to, static_cast< unsigned int >(copy_options::none), &ec);
}

inline bool copy_file(path const& from, path const& to, // See ticket #2925
                      BOOST_SCOPED_ENUM_NATIVE(copy_options) options)
{
    return detail::copy_file(from, to, static_cast< unsigned int >(options));
}

inline bool copy_file(path const& from, path const& to, // See ticket #2925
                      BOOST_SCOPED_ENUM_NATIVE(copy_options) options, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::copy_file(from, to, static_cast< unsigned int >(options), &ec);
}

#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
inline bool copy_file(path const& from, path const& to, // See ticket #2925
                      BOOST_SCOPED_ENUM_NATIVE(copy_option) options)
{
    return detail::copy_file(from, to, static_cast< unsigned int >(options));
}

inline bool copy_file(path const& from, path const& to, // See ticket #2925
                      BOOST_SCOPED_ENUM_NATIVE(copy_option) options, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::copy_file(from, to, static_cast< unsigned int >(options), &ec);
}
#endif // !defined(BOOST_FILESYSTEM_NO_DEPRECATED)

inline void copy_symlink(path const& existing_symlink, path const& new_symlink)
{
    detail::copy_symlink(existing_symlink, new_symlink);
}

inline void copy_symlink(path const& existing_symlink, path const& new_symlink, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::copy_symlink(existing_symlink, new_symlink, &ec);
}

inline bool create_directories(path const& p)
{
    return detail::create_directories(p);
}

inline bool create_directories(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::create_directories(p, &ec);
}

inline bool create_directory(path const& p)
{
    return detail::create_directory(p, 0);
}

inline bool create_directory(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::create_directory(p, 0, &ec);
}

inline bool create_directory(path const& p, path const& existing)
{
    return detail::create_directory(p, &existing);
}

inline bool create_directory(path const& p, path const& existing, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::create_directory(p, &existing, &ec);
}

inline void create_directory_symlink(path const& to, path const& from)
{
    detail::create_directory_symlink(to, from);
}

inline void create_directory_symlink(path const& to, path const& from, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::create_directory_symlink(to, from, &ec);
}

inline void create_hard_link(path const& to, path const& new_hard_link)
{
    detail::create_hard_link(to, new_hard_link);
}

inline void create_hard_link(path const& to, path const& new_hard_link, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::create_hard_link(to, new_hard_link, &ec);
}

inline void create_symlink(path const& to, path const& new_symlink)
{
    detail::create_symlink(to, new_symlink);
}

inline void create_symlink(path const& to, path const& new_symlink, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::create_symlink(to, new_symlink, &ec);
}

inline bool equivalent(path const& p1, path const& p2)
{
    return detail::equivalent(p1, p2);
}

inline bool equivalent(path const& p1, path const& p2, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::equivalent(p1, p2, &ec);
}

inline boost::uintmax_t file_size(path const& p)
{
    return detail::file_size(p);
}

inline boost::uintmax_t file_size(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::file_size(p, &ec);
}

inline boost::uintmax_t hard_link_count(path const& p)
{
    return detail::hard_link_count(p);
}

inline boost::uintmax_t hard_link_count(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::hard_link_count(p, &ec);
}

inline std::time_t creation_time(path const& p)
{
    return detail::creation_time(p);
}

inline std::time_t creation_time(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::creation_time(p, &ec);
}

inline std::time_t last_write_time(path const& p)
{
    return detail::last_write_time(p);
}

inline std::time_t last_write_time(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::last_write_time(p, &ec);
}

inline void last_write_time(path const& p, const std::time_t new_time)
{
    detail::last_write_time(p, new_time);
}

inline void last_write_time(path const& p, const std::time_t new_time, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::last_write_time(p, new_time, &ec);
}

inline void permissions(path const& p, perms prms)
{
    detail::permissions(p, prms);
}

inline void permissions(path const& p, perms prms, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::permissions(p, prms, &ec);
}

inline path read_symlink(path const& p)
{
    return detail::read_symlink(p);
}

inline path read_symlink(path const& p, system::error_code& ec)
{
    return detail::read_symlink(p, &ec);
}

inline bool remove(path const& p)
{
    return detail::remove(p);
}

inline bool remove(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::remove(p, &ec);
}

inline boost::uintmax_t remove_all(path const& p)
{
    return detail::remove_all(p);
}

inline boost::uintmax_t remove_all(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::remove_all(p, &ec);
}

inline void rename(path const& old_p, path const& new_p)
{
    detail::rename(old_p, new_p);
}

inline void rename(path const& old_p, path const& new_p, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::rename(old_p, new_p, &ec);
}

// name suggested by Scott McMurray
inline void resize_file(path const& p, uintmax_t size)
{
    detail::resize_file(p, size);
}

inline void resize_file(path const& p, uintmax_t size, system::error_code& ec) BOOST_NOEXCEPT
{
    detail::resize_file(p, size, &ec);
}

inline path relative(path const& p, path const& base = current_path())
{
    return detail::relative(p, base);
}

inline path relative(path const& p, system::error_code& ec)
{
    path base = current_path(ec);
    if (ec)
        return path();
    return detail::relative(p, base, &ec);
}

inline path relative(path const& p, path const& base, system::error_code& ec)
{
    return detail::relative(p, base, &ec);
}

inline space_info space(path const& p)
{
    return detail::space(p);
}

inline space_info space(path const& p, system::error_code& ec) BOOST_NOEXCEPT
{
    return detail::space(p, &ec);
}

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
inline bool symbolic_link_exists(path const& p)
{
    return is_symlink(filesystem::symlink_status(p));
}
#endif

inline path system_complete(path const& p)
{
    return detail::system_complete(p);
}

inline path system_complete(path const& p, system::error_code& ec)
{
    return detail::system_complete(p, &ec);
}

inline path temp_directory_path()
{
    return detail::temp_directory_path();
}

inline path temp_directory_path(system::error_code& ec)
{
    return detail::temp_directory_path(&ec);
}

inline path unique_path(path const& p = "%%%%-%%%%-%%%%-%%%%")
{
    return detail::unique_path(p);
}

inline path unique_path(path const& p, system::error_code& ec)
{
    return detail::unique_path(p, &ec);
}

inline path weakly_canonical(path const& p, path const& base = current_path())
{
    return detail::weakly_canonical(p, base);
}

inline path weakly_canonical(path const& p, system::error_code& ec)
{
    path base = current_path(ec);
    if (ec)
        return path();
    return detail::weakly_canonical(p, base, &ec);
}

inline path weakly_canonical(path const& p, path const& base, system::error_code& ec)
{
    return detail::weakly_canonical(p, base, &ec);
}

//  test helper  -----------------------------------------------------------------------//

//  Not part of the documented interface since false positives are possible;
//  there is no law that says that an OS that has large stat.st_size
//  actually supports large file sizes.

namespace detail {

BOOST_FILESYSTEM_DECL bool possible_large_file_size_support();

} // namespace detail

} // namespace filesystem
} // namespace boost

#include <boost/filesystem/detail/footer.hpp>

#endif // BOOST_FILESYSTEM_OPERATIONS_HPP
