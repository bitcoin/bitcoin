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
//  Description : simple helpers for creating cusom output manipulators
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_CUSTOM_MANIP_HPP
#define BOOST_TEST_UTILS_CUSTOM_MANIP_HPP

// STL
#include <iosfwd>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace utils {

// ************************************************************************** //
// **************          custom manipulators helpers         ************** //
// ************************************************************************** //

template<typename Manip>
struct custom_printer {
    explicit custom_printer( std::ostream& ostr ) : m_ostr( &ostr ) {}

    std::ostream& operator*() const { return *m_ostr; }

private:
    std::ostream* const m_ostr;
};

//____________________________________________________________________________//

template<typename Uniq> struct custom_manip {};

//____________________________________________________________________________//

template<typename Uniq>
inline custom_printer<custom_manip<Uniq> >
operator<<( std::ostream& ostr, custom_manip<Uniq> const& ) { return custom_printer<custom_manip<Uniq> >( ostr ); }

//____________________________________________________________________________//

} // namespace utils
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_CUSTOM_MANIP_HPP
