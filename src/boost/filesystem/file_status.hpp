//  boost/filesystem/file_status.hpp  --------------------------------------------------//

//  Copyright Beman Dawes 2002-2009
//  Copyright Jan Langer 2002
//  Copyright Dietmar Kuehl 2001
//  Copyright Vladimir Prus 2002
//  Copyright Andrey Semashev 2019

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  Library home page: http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#ifndef BOOST_FILESYSTEM_FILE_STATUS_HPP
#define BOOST_FILESYSTEM_FILE_STATUS_HPP

#include <boost/filesystem/config.hpp>
#include <boost/detail/bitmask.hpp>

#include <boost/filesystem/detail/header.hpp> // must be the last #include

//--------------------------------------------------------------------------------------//

namespace boost {
namespace filesystem {

//--------------------------------------------------------------------------------------//
//                                     file_type                                        //
//--------------------------------------------------------------------------------------//

enum file_type
{
    status_error,
#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
    status_unknown = status_error,
#endif
    file_not_found,
    regular_file,
    directory_file,
    // the following may not apply to some operating systems or file systems
    symlink_file,
    block_file,
    character_file,
    fifo_file,
    socket_file,
    reparse_file, // Windows: FILE_ATTRIBUTE_REPARSE_POINT that is not a symlink
    type_unknown, // file does exist, but isn't one of the above types or
                  // we don't have strong enough permission to find its type

    _detail_directory_symlink // internal use only; never exposed to users
};

//--------------------------------------------------------------------------------------//
//                                       perms                                          //
//--------------------------------------------------------------------------------------//

enum perms
{
    no_perms = 0, // file_not_found is no_perms rather than perms_not_known

    // POSIX equivalent macros given in comments.
    // Values are from POSIX and are given in octal per the POSIX standard.

    // permission bits

    owner_read = 0400,  // S_IRUSR, Read permission, owner
    owner_write = 0200, // S_IWUSR, Write permission, owner
    owner_exe = 0100,   // S_IXUSR, Execute/search permission, owner
    owner_all = 0700,   // S_IRWXU, Read, write, execute/search by owner

    group_read = 040,  // S_IRGRP, Read permission, group
    group_write = 020, // S_IWGRP, Write permission, group
    group_exe = 010,   // S_IXGRP, Execute/search permission, group
    group_all = 070,   // S_IRWXG, Read, write, execute/search by group

    others_read = 04,  // S_IROTH, Read permission, others
    others_write = 02, // S_IWOTH, Write permission, others
    others_exe = 01,   // S_IXOTH, Execute/search permission, others
    others_all = 07,   // S_IRWXO, Read, write, execute/search by others

    all_all = 0777, // owner_all|group_all|others_all

    // other POSIX bits

    set_uid_on_exe = 04000, // S_ISUID, Set-user-ID on execution
    set_gid_on_exe = 02000, // S_ISGID, Set-group-ID on execution
    sticky_bit = 01000,     // S_ISVTX,
                            // (POSIX XSI) On directories, restricted deletion flag
                            // (V7) 'sticky bit': save swapped text even after use
                            // (SunOS) On non-directories: don't cache this file
                            // (SVID-v4.2) On directories: restricted deletion flag
                            // Also see http://en.wikipedia.org/wiki/Sticky_bit

    perms_mask = 07777, // all_all|set_uid_on_exe|set_gid_on_exe|sticky_bit

    perms_not_known = 0xFFFF, // present when directory_entry cache not loaded

    // options for permissions() function

    add_perms = 0x1000,    // adds the given permission bits to the current bits
    remove_perms = 0x2000, // removes the given permission bits from the current bits;
                           // choose add_perms or remove_perms, not both; if neither add_perms
                           // nor remove_perms is given, replace the current bits with
                           // the given bits.

    symlink_perms = 0x4000, // on POSIX, don't resolve symlinks; implied on Windows

    // BOOST_BITMASK op~ casts to int32_least_t, producing invalid enum values
    _detail_extend_perms_32_1 = 0x7fffffff,
    _detail_extend_perms_32_2 = -0x7fffffff - 1
};

BOOST_BITMASK(perms)

//--------------------------------------------------------------------------------------//
//                                    file_status                                       //
//--------------------------------------------------------------------------------------//

class file_status
{
public:
    BOOST_CONSTEXPR file_status() BOOST_NOEXCEPT :
        m_value(status_error),
        m_perms(perms_not_known)
    {
    }
    explicit BOOST_CONSTEXPR file_status(file_type v) BOOST_NOEXCEPT :
        m_value(v),
        m_perms(perms_not_known)
    {
    }
    BOOST_CONSTEXPR file_status(file_type v, perms prms) BOOST_NOEXCEPT :
        m_value(v),
        m_perms(prms)
    {
    }

    //  As of October 2015 the interaction between noexcept and =default is so troublesome
    //  for VC++, GCC, and probably other compilers, that =default is not used with noexcept
    //  functions. GCC is not even consistent for the same release on different platforms.

    BOOST_CONSTEXPR file_status(const file_status& rhs) BOOST_NOEXCEPT :
        m_value(rhs.m_value),
        m_perms(rhs.m_perms)
    {
    }
    BOOST_CXX14_CONSTEXPR file_status& operator=(const file_status& rhs) BOOST_NOEXCEPT
    {
        m_value = rhs.m_value;
        m_perms = rhs.m_perms;
        return *this;
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    // Note: std::move is not constexpr in C++11, that's why we're not using it here
    BOOST_CONSTEXPR file_status(file_status&& rhs) BOOST_NOEXCEPT :
        m_value(static_cast< file_type&& >(rhs.m_value)),
        m_perms(static_cast< enum perms&& >(rhs.m_perms))
    {
    }
    BOOST_CXX14_CONSTEXPR file_status& operator=(file_status&& rhs) BOOST_NOEXCEPT
    {
        m_value = static_cast< file_type&& >(rhs.m_value);
        m_perms = static_cast< enum perms&& >(rhs.m_perms);
        return *this;
    }
#endif

    // observers
    BOOST_CONSTEXPR file_type type() const BOOST_NOEXCEPT { return m_value; }
    BOOST_CONSTEXPR perms permissions() const BOOST_NOEXCEPT { return m_perms; }

    // modifiers
    BOOST_CXX14_CONSTEXPR void type(file_type v) BOOST_NOEXCEPT { m_value = v; }
    BOOST_CXX14_CONSTEXPR void permissions(perms prms) BOOST_NOEXCEPT { m_perms = prms; }

    BOOST_CONSTEXPR bool operator==(const file_status& rhs) const BOOST_NOEXCEPT
    {
        return type() == rhs.type() && permissions() == rhs.permissions();
    }
    BOOST_CONSTEXPR bool operator!=(const file_status& rhs) const BOOST_NOEXCEPT
    {
        return !(*this == rhs);
    }

private:
    file_type m_value;
    enum perms m_perms;
};

inline BOOST_CONSTEXPR bool type_present(file_status f) BOOST_NOEXCEPT
{
    return f.type() != status_error;
}

inline BOOST_CONSTEXPR bool permissions_present(file_status f) BOOST_NOEXCEPT
{
    return f.permissions() != perms_not_known;
}

inline BOOST_CONSTEXPR bool status_known(file_status f) BOOST_NOEXCEPT
{
    return filesystem::type_present(f) && filesystem::permissions_present(f);
}

inline BOOST_CONSTEXPR bool exists(file_status f) BOOST_NOEXCEPT
{
    return f.type() != status_error && f.type() != file_not_found;
}

inline BOOST_CONSTEXPR bool is_regular_file(file_status f) BOOST_NOEXCEPT
{
    return f.type() == regular_file;
}

inline BOOST_CONSTEXPR bool is_directory(file_status f) BOOST_NOEXCEPT
{
    return f.type() == directory_file;
}

inline BOOST_CONSTEXPR bool is_symlink(file_status f) BOOST_NOEXCEPT
{
    return f.type() == symlink_file;
}

inline BOOST_CONSTEXPR bool is_other(file_status f) BOOST_NOEXCEPT
{
    return filesystem::exists(f) && !filesystem::is_regular_file(f) && !filesystem::is_directory(f) && !filesystem::is_symlink(f);
}

#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
inline bool is_regular(file_status f) BOOST_NOEXCEPT
{
    return filesystem::is_regular_file(f);
}
#endif

} // namespace filesystem
} // namespace boost

#include <boost/filesystem/detail/footer.hpp> // pops abi_prefix.hpp pragmas

#endif // BOOST_FILESYSTEM_FILE_STATUS_HPP
