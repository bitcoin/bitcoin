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
//  Description : defines level of indiration facilitating workarounds for non printable types
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_IMPL_COMMON_HPP_012705GER
#define BOOST_TEST_TOOLS_IMPL_COMMON_HPP_012705GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/detail/global_typedef.hpp>

// Boost
#include <boost/mpl/or.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/is_abstract.hpp>
#include <boost/type_traits/has_left_shift.hpp>

#include <ios>
#include <iostream>
#include <limits>

#if !defined(BOOST_NO_CXX11_NULLPTR)
#include <cstddef>
#endif

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {
namespace tt_detail {

// ************************************************************************** //
// **************          boost_test_print_type               ************** //
// ************************************************************************** //

    namespace impl {
        template <class T>
        std::ostream& boost_test_print_type(std::ostream& ostr, T const& t) {
            BOOST_STATIC_ASSERT_MSG( (boost::has_left_shift<std::ostream,T>::value),
                                    "Type has to implement operator<< to be printable");
            ostr << t;
            return ostr;
        }

        struct boost_test_print_type_impl {
            template <class R>
            std::ostream& operator()(std::ostream& ostr, R const& r) const {
                return boost_test_print_type(ostr, r);
            }
        };
    }

    // To avoid ODR violations, see N4381
    template <class T> struct static_const { static const T value; };
    template <class T> const T static_const<T>::value = T();

    namespace {
        static const impl::boost_test_print_type_impl& boost_test_print_type =
            static_const<impl::boost_test_print_type_impl>::value;
    }


// ************************************************************************** //
// **************                print_log_value               ************** //
// ************************************************************************** //

template<typename T>
struct print_log_value {
    void    operator()( std::ostream& ostr, T const& t )
    {
        typedef typename mpl::or_<is_array<T>,is_function<T>,is_abstract<T> >::type cant_use_nl;

        std::streamsize old_precision = set_precision( ostr, cant_use_nl() );

        //ostr << t;
        using boost::test_tools::tt_detail::boost_test_print_type;
        boost_test_print_type(ostr, t);

        if( old_precision != (std::streamsize)-1 )
            ostr.precision( old_precision );
    }

    std::streamsize set_precision( std::ostream& ostr, mpl::false_ )
    {
        if( std::numeric_limits<T>::is_specialized && std::numeric_limits<T>::radix == 2 )
            return ostr.precision( 2 + std::numeric_limits<T>::digits * 301/1000 );
        else if ( std::numeric_limits<T>::is_specialized && std::numeric_limits<T>::radix == 10 ) {
#ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
            // (was BOOST_NO_NUMERIC_LIMITS_LOWEST but now deprecated).
            // No support for std::numeric_limits<double>::max_digits10,
            // so guess that a couple of guard digits more than digits10 will display any difference.
            return ostr.precision( 2 + std::numeric_limits<T>::digits10 );
#else
            // std::numeric_limits<double>::max_digits10; IS supported.
            // Any noisy or guard digits needed to display any difference are included in max_digits10.
            return ostr.precision( std::numeric_limits<T>::max_digits10 );
#endif
        }
        // else if T is not specialized for std::numeric_limits<>,
        // then will just get the default precision of 6 digits.
        return (std::streamsize)-1;
    }

    std::streamsize set_precision( std::ostream&, mpl::true_ ) { return (std::streamsize)-1; }
};

//____________________________________________________________________________//

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
template<typename T, std::size_t N >
struct print_log_value< T[N] > {
    void    operator()( std::ostream& ostr, T const* t )
    {
        ostr << t;
    }
};
#endif

//____________________________________________________________________________//

template<>
struct BOOST_TEST_DECL print_log_value<bool> {
    void    operator()( std::ostream& ostr, bool t );
};

//____________________________________________________________________________//

template<>
struct BOOST_TEST_DECL print_log_value<char> {
    void    operator()( std::ostream& ostr, char t );
};

//____________________________________________________________________________//

template<>
struct BOOST_TEST_DECL print_log_value<unsigned char> {
    void    operator()( std::ostream& ostr, unsigned char t );
};

//____________________________________________________________________________//

template<>
struct BOOST_TEST_DECL print_log_value<char const*> {
    void    operator()( std::ostream& ostr, char const* t );
};

//____________________________________________________________________________//

template<>
struct BOOST_TEST_DECL print_log_value<wchar_t const*> {
    void    operator()( std::ostream& ostr, wchar_t const* t );
};

#if !defined(BOOST_NO_CXX11_NULLPTR)
template<>
struct print_log_value<std::nullptr_t> {
    // declaration and definition is here because of #12969 https://svn.boost.org/trac10/ticket/12969
    void    operator()( std::ostream& ostr, std::nullptr_t /*t*/ ) {
        ostr << "nullptr";
    }
};
#endif

//____________________________________________________________________________//

// ************************************************************************** //
// **************                 print_helper                 ************** //
// ************************************************************************** //
// Adds level of indirection to the output operation, allowing us to customize
// it for types that do not support operator << directly or for any other reason

template<typename T>
struct print_helper_t {
    explicit    print_helper_t( T const& t ) : m_t( t ) {}

    T const&    m_t;
};

//____________________________________________________________________________//

#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
// Borland suffers premature pointer decay passing arrays by reference
template<typename T, std::size_t N >
struct print_helper_t< T[N] > {
    explicit    print_helper_t( T const * t ) : m_t( t ) {}

    T const *   m_t;
};
#endif

//____________________________________________________________________________//

template<typename T>
inline print_helper_t<T>
print_helper( T const& t )
{
    return print_helper_t<T>( t );
}

//____________________________________________________________________________//

template<typename T>
inline std::ostream&
operator<<( std::ostream& ostr, print_helper_t<T> const& ph )
{
    print_log_value<T>()( ostr, ph.m_t );

    return ostr;
}

//____________________________________________________________________________//

} // namespace tt_detail

// ************************************************************************** //
// **************       BOOST_TEST_DONT_PRINT_LOG_VALUE        ************** //
// ************************************************************************** //

#define BOOST_TEST_DONT_PRINT_LOG_VALUE( the_type )         \
namespace boost{ namespace test_tools{ namespace tt_detail{ \
template<>                                                  \
struct print_log_value<the_type > {                         \
    void    operator()( std::ostream&, the_type const& ) {} \
};                                                          \
}}}                                                         \
/**/

} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_IMPL_COMMON_HPP_012705GER
