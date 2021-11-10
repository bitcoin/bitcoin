#ifndef BOOST_ARCHIVE_BASIC_STREAMBUF_LOCALE_SAVER_HPP
#define BOOST_ARCHIVE_BASIC_STREAMBUF_LOCALE_SAVER_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_streambuf_locale_saver.hpp

// (C) Copyright 2005 Robert Ramey - http://www.rrsd.com

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// note derived from boost/io/ios_state.hpp
// Copyright 2002, 2005 Daryle Walker.  Use, modification, and distribution
// are subject to the Boost Software License, Version 1.0.  (See accompanying
// file LICENSE_1_0.txt or a copy at <http://www.boost.org/LICENSE_1_0.txt>.)

//  See <http://www.boost.org/libs/io/> for the library's home page.

#ifndef BOOST_NO_STD_LOCALE

#include <locale>     // for std::locale
#include <ios>
#include <streambuf>  // for std::basic_streambuf

#include <boost/config.hpp>
#include <boost/noncopyable.hpp>

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

namespace boost{
namespace archive{

template < typename Ch, class Tr >
class basic_streambuf_locale_saver :
    private boost::noncopyable
{
public:
    explicit basic_streambuf_locale_saver(std::basic_streambuf<Ch, Tr> &s) :
        m_streambuf(s),
        m_locale(s.getloc())
    {}
    ~basic_streambuf_locale_saver(){
        m_streambuf.pubsync();
        m_streambuf.pubimbue(m_locale);
    }
private:
    std::basic_streambuf<Ch, Tr> &       m_streambuf;
    std::locale const  m_locale;
};

template < typename Ch, class Tr >
class basic_istream_locale_saver :
    private boost::noncopyable
{
public:
    explicit basic_istream_locale_saver(std::basic_istream<Ch, Tr> &s) :
        m_istream(s),
        m_locale(s.getloc())
    {}
    ~basic_istream_locale_saver(){
        // libstdc++ crashes without this
        m_istream.sync();
        m_istream.imbue(m_locale);
    }
private:
    std::basic_istream<Ch, Tr> & m_istream;
    std::locale const  m_locale;
};

template < typename Ch, class Tr >
class basic_ostream_locale_saver :
    private boost::noncopyable
{
public:
    explicit basic_ostream_locale_saver(std::basic_ostream<Ch, Tr> &s) :
        m_ostream(s),
        m_locale(s.getloc())
    {}
    ~basic_ostream_locale_saver(){
        m_ostream.flush();
        m_ostream.imbue(m_locale);
    }
private:
    std::basic_ostream<Ch, Tr> & m_ostream;
    std::locale const  m_locale;
};


} // archive
} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif // BOOST_NO_STD_LOCALE
#endif // BOOST_ARCHIVE_BASIC_STREAMBUF_LOCALE_SAVER_HPP
