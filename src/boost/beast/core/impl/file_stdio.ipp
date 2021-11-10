//
// Copyright (c) 2015-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_IMPL_FILE_STDIO_IPP
#define BOOST_BEAST_CORE_IMPL_FILE_STDIO_IPP

#include <boost/beast/core/file_stdio.hpp>
#include <boost/beast/core/detail/win32_unicode_path.hpp>
#include <boost/config/workaround.hpp>
#include <boost/core/exchange.hpp>
#include <limits>

namespace boost {
namespace beast {

file_stdio::
~file_stdio()
{
    if(f_)
        fclose(f_);
}

file_stdio::
file_stdio(file_stdio&& other)
    : f_(boost::exchange(other.f_, nullptr))
{
}

file_stdio&
file_stdio::
operator=(file_stdio&& other)
{
    if(&other == this)
        return *this;
    if(f_)
        fclose(f_);
    f_ = other.f_;
    other.f_ = nullptr;
    return *this;
}

void
file_stdio::
native_handle(std::FILE* f)
{
    if(f_)
        fclose(f_);
    f_ = f;
}

void
file_stdio::
close(error_code& ec)
{
    if(f_)
    {
        int failed = fclose(f_);
        f_ = nullptr;
        if(failed)
        {
            ec.assign(errno, generic_category());
            return;
        }
    }
    ec = {};
}

void
file_stdio::
open(char const* path, file_mode mode, error_code& ec)
{
    if(f_)
    {
        fclose(f_);
        f_ = nullptr;
    }
    ec = {};
#ifdef BOOST_MSVC
    boost::winapi::WCHAR_ const* s;
    detail::win32_unicode_path unicode_path(path, ec);
    if (ec)
        return;
#else
    char const* s;
#endif
    switch(mode)
    {
    default:
    case file_mode::read:
    #ifdef BOOST_MSVC
        s = L"rb";
    #else
        s = "rb";
    #endif
        break;

    case file_mode::scan:
    #ifdef BOOST_MSVC
        s = L"rbS";
    #else
        s = "rb";
    #endif
        break;

    case file_mode::write:
    #ifdef BOOST_MSVC
        s = L"wb+";
    #else
        s = "wb+";
    #endif
        break;

    case file_mode::write_new:
    {
#if BOOST_WORKAROUND(BOOST_MSVC, < 1910)
        std::FILE* f0;
        auto const ev = ::_wfopen_s(&f0, unicode_path.c_str(), L"rb");
        if(! ev)
        {
            std::fclose(f0);
            ec = make_error_code(errc::file_exists);
            return;
        }
        else if(ev !=
            errc::no_such_file_or_directory)
        {
            ec.assign(ev, generic_category());
            return;
        }
        s = L"wb";
#elif defined(BOOST_MSVC)
        s = L"wbx";
#else
        s = "wbx";
#endif
        break;
    }

    case file_mode::write_existing:
    #ifdef BOOST_MSVC
        s = L"rb+";
    #else
        s = "rb+";
    #endif
        break;

    case file_mode::append:
    #ifdef BOOST_MSVC
        s = L"ab";
    #else
        s = "ab";
    #endif
        break;

    case file_mode::append_existing:
    {
#ifdef BOOST_MSVC
        std::FILE* f0;
        auto const ev =
            ::_wfopen_s(&f0, unicode_path.c_str(), L"rb+");
        if(ev)
        {
            ec.assign(ev, generic_category());
            return;
        }
#else
        auto const f0 =
            std::fopen(path, "rb+");
        if(! f0)
        {
            ec.assign(errno, generic_category());
            return;
        }
#endif
        std::fclose(f0);
    #ifdef BOOST_MSVC
        s = L"ab";
    #else
        s = "ab";
    #endif
        break;
    }
    }

#ifdef BOOST_MSVC
    auto const ev = ::_wfopen_s(&f_, unicode_path.c_str(), s);
    if(ev)
    {
        f_ = nullptr;
        ec.assign(ev, generic_category());
        return;
    }
#else
    f_ = std::fopen(path, s);
    if(! f_)
    {
        ec.assign(errno, generic_category());
        return;
    }
#endif
}

std::uint64_t
file_stdio::
size(error_code& ec) const
{
    if(! f_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    long pos = std::ftell(f_);
    if(pos == -1L)
    {
        ec.assign(errno, generic_category());
        return 0;
    }
    int result = std::fseek(f_, 0, SEEK_END);
    if(result != 0)
    {
        ec.assign(errno, generic_category());
        return 0;
    }
    long size = std::ftell(f_);
    if(size == -1L)
    {
        ec.assign(errno, generic_category());
        std::fseek(f_, pos, SEEK_SET);
        return 0;
    }
    result = std::fseek(f_, pos, SEEK_SET);
    if(result != 0)
        ec.assign(errno, generic_category());
    else
        ec = {};
    return size;
}

std::uint64_t
file_stdio::
pos(error_code& ec) const
{
    if(! f_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    long pos = std::ftell(f_);
    if(pos == -1L)
    {
        ec.assign(errno, generic_category());
        return 0;
    }
    ec = {};
    return pos;
}

void
file_stdio::
seek(std::uint64_t offset, error_code& ec)
{
    if(! f_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return;
    }
    if(offset > static_cast<std::uint64_t>((std::numeric_limits<long>::max)()))
    {
        ec = make_error_code(errc::invalid_seek);
        return;
    }
    int result = std::fseek(f_,
        static_cast<long>(offset), SEEK_SET);
    if(result != 0)
        ec.assign(errno, generic_category());
    else
        ec = {};
}

std::size_t
file_stdio::
read(void* buffer, std::size_t n, error_code& ec) const
{
    if(! f_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    auto nread = std::fread(buffer, 1, n, f_);
    if(std::ferror(f_))
    {
        ec.assign(errno, generic_category());
        return 0;
    }
    return nread;
}

std::size_t
file_stdio::
write(void const* buffer, std::size_t n, error_code& ec)
{
    if(! f_)
    {
        ec = make_error_code(errc::bad_file_descriptor);
        return 0;
    }
    auto nwritten = std::fwrite(buffer, 1, n, f_);
    if(std::ferror(f_))
    {
        ec.assign(errno, generic_category());
        return 0;
    }
    return nwritten;
}

} // beast
} // boost

#endif
