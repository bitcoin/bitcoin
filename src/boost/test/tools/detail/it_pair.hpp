//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 74248 $
//
//  Description : support for backward compatible collection comparison interface
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_DETAIL_IT_PAIR_HPP_112812GER
#define BOOST_TEST_TOOLS_DETAIL_IT_PAIR_HPP_112812GER

#ifdef BOOST_TEST_NO_OLD_TOOLS

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {
namespace tt_detail {

// ************************************************************************** //
// **************       backward compatibility support         ************** //
// ************************************************************************** //

template<typename It>
struct it_pair {
    typedef It const_iterator;
    typedef typename std::iterator_traits<It>::value_type value_type;

    it_pair( It const& b, It const& e ) : m_begin( b ), m_size( 0 )
    {
        It tmp = b;
        while( tmp != e ) { ++m_size; ++tmp; }
    }

    It      begin() const   { return m_begin; }
    It      end() const     { return m_begin + m_size; }
    size_t  size() const    { return m_size; }

private:
    It      m_begin;
    size_t  m_size;
};

//____________________________________________________________________________//

template<typename It>
it_pair<It>
make_it_pair( It const& b, It const& e ) { return it_pair<It>( b, e ); }

//____________________________________________________________________________//

template<typename T>
it_pair<T const*>
make_it_pair( T const* b, T const* e ) { return it_pair<T const*>( b, e ); }

//____________________________________________________________________________//

} // namespace tt_detail
} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_NO_OLD_TOOLS

#endif // BOOST_TEST_TOOLS_DETAIL_IT_PAIR_HPP_112812GER
