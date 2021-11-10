//
// Copyright (c) 2015-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_IMPL_FILE_POSIX_IPP
#define BOOST_BEAST_CORE_IMPL_FILE_POSIX_IPP

#include <boost/beast/core/file_posix.hpp>

#if BOOST_BEAST_USE_POSIX_FILE

#include <boost/core/exchange.hpp>
#include <limits>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#if ! defined(BOOST_BEAST_NO_POSIX_FADVISE)
# if defined(__APPLE__) || (defined(__ANDROID__) && (__ANDROID_API__ < 21))
#  define BOOST_BEAST_NO_POSIX_FADVISE
# endif
#endif

#if ! defined(BOOST_BEAST_USE_POSIX_FADVISE)
# if ! defined(BOOST_BEAST_NO_POSIX_FADVISE)
#  define BOOST_BEAST_USE_POSIX_FADVISE 1
# else
#  define BOOST_BEAST_USE_POSIX_FADVISE 0
# endif
#endif

namespace boost {
namespace beast {

int
file_posix::
native_close(native_handle_type& fd)
{
/*  https://github.com/boostorg/beast/issues/1445

    This function is tuned for Linux / Mac OS:
    
    * only calls close() once
    * returns the error directly to the caller
    * does not loop on EINTR

    If this is incorrect for the platform, then the
    caller will need to implement their own type
    meeting the File requirements and use the correct
    behavior.

    See:
        http://man7.org/linux/man-pages/man2/close.2.html
*/
    int ev = 0;
    if(fd != -1)
    {
        if(::close(fd) != 0)
            ev = errno;
        fd = -1;
    }
    return ev;
}

file_posix::
~file_posix()
{
    native_close(fd_);
}

file_posix::
file_posix(file_posix&& other)
    : fd_(boost::exchange(other.fd_, -1))
{
}

file_posix&
file_posix::
operator=(file_posix&& other)
{
    if(&other == this)
        return *this;
    native_close(fd_);
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
}

void
file_posix::
native_handle(native_handle_type fd)
{
    native_close(fd_);
    fd_ = fd;
}

void
file_posix::
close(error_code& ec)
{
    auto const ev = native_close(fd_);
    if(ev)
        ec.assign(ev, system_category());
    else
        ec = {};
}

void
file_posix::
open(char const* path, file_mode mode, error_code& ec)
{
    auto const ev = native_close(fd_);
    if(ev)
        ec.assign(ev, system_category());
    else
        ec = {};

    int f = 0;
#if BOOST_BEAST_USE_POSIX_FADVISE
    int advise = 0;
#endif
    switch(mode)
    {
    default:
    case file_mode::read:
        f = O_RDONLY;
    #if BOOST_BEAST_USE_POSIX_FADVISE
        advise = POSIX_FADV_RANDOM;
    #endif
        break;
    case file_mode::scan:
        f = O_RDONLY;
    #if BOOST_BEAST_USE_POSIX_FADVISE
        advise = POSIX_FADV_SEQUENTIAL;
    #endif
        break;

    case file_mode::write:
        f = O_RDWR | O_CREAT | O_TRUNC;
    #if BOOST_BEAST_USE_POSIX_FADVISE
        advise = POSIX_FADV_RANDOM;
    #endif
        break;

    case file_mode::write_new:      
        f = O_RDWR | O_CREAT | O_EXCL;
    #if BOOST_BEAST_USE_POSIX_FADVISE
        advise = POSIX_FADV_RANDOM;
    #endif
        break;

    case file_mode::write_existing: 
        f = O_RDWR | O_EXCL;
    #if BOOST_BEAST_USE_POSIX_FADVISE
        advise = POSIX_FADV_RANDOM;
    #endif
        break;

    case file_mode::append:         
        f = O_WRONLY | O_CREAT | O_TRUNC;
    #if BOOST_BEAST_USE_POSIX_FADVISE
        advise = POSIX_FADV_SEQUENTIAL;
    #endif
        break;

    case file_mode::append_existing:
        f = O_WRONLY | O_APPEND;
    #if BOOST_BEAST_USE_POSIX_FADVISE
        advise = POSIX_FADV_SEQUENTIAL;
    #endif
        break;
    }
    for(;;)
    {
        fd_ = ::open(path, f, 0644);
        if(fd_ != -1)
            break;
        auto const ev = errno;
        if(ev != EINTR)
        {
            ec.assign(ev, system_category());
            return;
        }
    }
#if BOOST_BEAST_USE_POSIX_FADVISE
    if(::posix_fadvise(fd_, 0, 0, advise))
    {
        auto const ev = errno;
        native_close(fd_);
        ec.assign(ev, system_category());
        return;
    }
#endif
    ec = {};
}

std::uint64_t
file_posix::
size(error_code& ec) const
{
    if(fd_ == -1)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    struct stat st;
    if(::fstat(fd_, &st) != 0)
    {
        ec.assign(errno, system_category());
        return 0;
    }
    ec = {};
    return st.st_size;
}

std::uint64_t
file_posix::
pos(error_code& ec) const
{
    if(fd_ == -1)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    auto const result = ::lseek(fd_, 0, SEEK_CUR);
    if(result == (off_t)-1)
    {
        ec.assign(errno, system_category());
        return 0;
    }
    ec = {};
    return result;
}

void
file_posix::
seek(std::uint64_t offset, error_code& ec)
{
    if(fd_ == -1)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return;
    }
    auto const result = ::lseek(fd_, offset, SEEK_SET);
    if(result == static_cast<off_t>(-1))
    {
        ec.assign(errno, system_category());
        return;
    }
    ec = {};
}

std::size_t
file_posix::
read(void* buffer, std::size_t n, error_code& ec) const
{
    if(fd_ == -1)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    std::size_t nread = 0;
    while(n > 0)
    {
        // <limits> not required to define SSIZE_MAX so we avoid it
        constexpr auto ssmax =
            static_cast<std::size_t>((std::numeric_limits<
                decltype(::read(fd_, buffer, n))>::max)());
        auto const amount = (std::min)(
            n, ssmax);
        auto const result = ::read(fd_, buffer, amount);
        if(result == -1)
        {
            auto const ev = errno;
            if(ev == EINTR)
                continue;
            ec.assign(ev, system_category());
            return nread;
        }
        if(result == 0)
        {
            // short read
            return nread;
        }
        n -= result;
        nread += result;
        buffer = static_cast<char*>(buffer) + result;
    }
    return nread;
}

std::size_t
file_posix::
write(void const* buffer, std::size_t n, error_code& ec)
{
    if(fd_ == -1)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    std::size_t nwritten = 0;
    while(n > 0)
    {
        // <limits> not required to define SSIZE_MAX so we avoid it
        constexpr auto ssmax =
            static_cast<std::size_t>((std::numeric_limits<
                decltype(::write(fd_, buffer, n))>::max)());
        auto const amount = (std::min)(
            n, ssmax);
        auto const result = ::write(fd_, buffer, amount);
        if(result == -1)
        {
            auto const ev = errno;
            if(ev == EINTR)
                continue;
            ec.assign(ev, system_category());
            return nwritten;
        }
        n -= result;
        nwritten += result;
        buffer = static_cast<char const*>(buffer) + result;
    }
    return nwritten;
}

} // beast
} // boost

#endif

#endif
