//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : wraps strstream and stringstream (depends with one is present)
//                to provide the unified interface
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_WRAP_STRINGSTREAM_HPP
#define BOOST_TEST_UTILS_WRAP_STRINGSTREAM_HPP

// Boost.Test
#include <boost/test/detail/config.hpp>

// STL
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>        // for std::ostrstream
#else
#include <sstream>          // for std::ostringstream
#endif // BOOST_NO_STRINGSTREAM

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {

// ************************************************************************** //
// **************            basic_wrap_stringstream           ************** //
// ************************************************************************** //

template<typename CharT>
class basic_wrap_stringstream {
public:
#if defined(BOOST_CLASSIC_IOSTREAMS)
    typedef std::ostringstream               wrapped_stream;
#elif defined(BOOST_NO_STRINGSTREAM)
    typedef std::basic_ostrstream<CharT>     wrapped_stream;
#else
    typedef std::basic_ostringstream<CharT>  wrapped_stream;
#endif // BOOST_NO_STRINGSTREAM
    // Access methods
    basic_wrap_stringstream&        ref();
    wrapped_stream&                 stream();
    std::basic_string<CharT> const& str();

private:
    // Data members
    wrapped_stream                  m_stream;
    std::basic_string<CharT>        m_str;
};

//____________________________________________________________________________//

template <typename CharT, typename T>
inline basic_wrap_stringstream<CharT>&
operator<<( basic_wrap_stringstream<CharT>& targ, T const& t )
{
    targ.stream() << t;
    return targ;
}

//____________________________________________________________________________//

template <typename CharT>
inline typename basic_wrap_stringstream<CharT>::wrapped_stream&
basic_wrap_stringstream<CharT>::stream()
{
    return m_stream;
}

//____________________________________________________________________________//

template <typename CharT>
inline basic_wrap_stringstream<CharT>&
basic_wrap_stringstream<CharT>::ref()
{
    return *this;
}

//____________________________________________________________________________//

template <typename CharT>
inline std::basic_string<CharT> const&
basic_wrap_stringstream<CharT>::str()
{

#ifdef BOOST_NO_STRINGSTREAM
    m_str.assign( m_stream.str(), m_stream.pcount() );
    m_stream.freeze( false );
#else
    m_str = m_stream.str();
#endif

    return m_str;
}

//____________________________________________________________________________//

template <typename CharT>
inline basic_wrap_stringstream<CharT>&
operator<<( basic_wrap_stringstream<CharT>& targ, basic_wrap_stringstream<CharT>& src )
{
    targ << src.str();
    return targ;
}

//____________________________________________________________________________//

#if BOOST_TEST_USE_STD_LOCALE

template <typename CharT>
inline basic_wrap_stringstream<CharT>&
operator<<( basic_wrap_stringstream<CharT>& targ, std::ios_base& (BOOST_TEST_CALL_DECL *man)(std::ios_base&) )
{
    targ.stream() << man;
    return targ;
}

//____________________________________________________________________________//

template<typename CharT,typename Elem,typename Tr>
inline basic_wrap_stringstream<CharT>&
operator<<( basic_wrap_stringstream<CharT>& targ, std::basic_ostream<Elem,Tr>& (BOOST_TEST_CALL_DECL *man)(std::basic_ostream<Elem, Tr>&) )
{
    targ.stream() << man;
    return targ;
}

//____________________________________________________________________________//

template<typename CharT,typename Elem,typename Tr>
inline basic_wrap_stringstream<CharT>&
operator<<( basic_wrap_stringstream<CharT>& targ, std::basic_ios<Elem, Tr>& (BOOST_TEST_CALL_DECL *man)(std::basic_ios<Elem, Tr>&) )
{
    targ.stream() << man;
    return targ;
}

//____________________________________________________________________________//

#endif

// ************************************************************************** //
// **************               wrap_stringstream              ************** //
// ************************************************************************** //

typedef basic_wrap_stringstream<char>       wrap_stringstream;
typedef basic_wrap_stringstream<wchar_t>    wrap_wstringstream;

}  // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif  // BOOST_TEST_UTILS_WRAP_STRINGSTREAM_HPP
