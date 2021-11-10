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
//  Description : implementation details for old toolbox
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_OLD_IMPL_HPP_012705GER
#define BOOST_TEST_TOOLS_OLD_IMPL_HPP_012705GER

// Boost.Test
#include <boost/test/unit_test_log.hpp>
#include <boost/test/tools/assertion_result.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

#include <boost/test/tools/detail/fwd.hpp>
#include <boost/test/tools/detail/print_helper.hpp>

// Boost
#include <boost/limits.hpp>
#include <boost/numeric/conversion/conversion_traits.hpp> // for numeric::conversion_traits
#include <boost/type_traits/is_array.hpp>

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>

// STL
#include <cstddef>          // for std::size_t
#include <climits>          // for CHAR_BIT

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {
namespace tt_detail {

// ************************************************************************** //
// **************          old TOOLBOX Implementation          ************** //
// ************************************************************************** //

// This function adds level of indirection, but it makes sure we evaluate predicate
// arguments only once

#ifndef BOOST_TEST_PROD
#define TEMPL_PARAMS( z, m, dummy ) , typename BOOST_JOIN( Arg, m )

#define FUNC_PARAMS( z, m, dummy )                                                  \
 , BOOST_JOIN( Arg, m ) const& BOOST_JOIN( arg, m )                                 \
 , char const* BOOST_JOIN( BOOST_JOIN( arg, m ), _descr )                           \
/**/

#define PRED_PARAMS( z, m, dummy ) BOOST_PP_COMMA_IF( m ) BOOST_JOIN( arg, m )

#define ARG_INFO( z, m, dummy )                                                     \
 , BOOST_JOIN( BOOST_JOIN( arg, m ), _descr )                                       \
 , &static_cast<const unit_test::lazy_ostream&>(unit_test::lazy_ostream::instance() \
        << ::boost::test_tools::tt_detail::print_helper( BOOST_JOIN( arg, m ) ))    \
/**/

#define IMPL_FRWD( z, n, dummy )                                                    \
template<typename Pred                                                              \
         BOOST_PP_REPEAT_ ## z( BOOST_PP_ADD( n, 1 ), TEMPL_PARAMS, _ )>            \
inline bool                                                                         \
check_frwd( Pred P, unit_test::lazy_ostream const& assertion_descr,                 \
            const_string file_name, std::size_t line_num,                           \
            tool_level tl, check_type ct                                            \
            BOOST_PP_REPEAT_ ## z( BOOST_PP_ADD( n, 1 ), FUNC_PARAMS, _ )           \
)                                                                                   \
{                                                                                   \
    return                                                                          \
    report_assertion( P( BOOST_PP_REPEAT_ ## z(BOOST_PP_ADD(n, 1), PRED_PARAMS,_) ),\
                assertion_descr, file_name, line_num, tl, ct,                       \
                BOOST_PP_ADD( n, 1 )                                                \
                BOOST_PP_REPEAT_ ## z( BOOST_PP_ADD( n, 1 ), ARG_INFO, _ )          \
    );                                                                              \
}                                                                                   \
/**/

#ifndef BOOST_TEST_MAX_PREDICATE_ARITY
#define BOOST_TEST_MAX_PREDICATE_ARITY 5
#endif

BOOST_PP_REPEAT( BOOST_TEST_MAX_PREDICATE_ARITY, IMPL_FRWD, _ )

#undef TEMPL_PARAMS
#undef FUNC_PARAMS
#undef PRED_INFO
#undef ARG_INFO
#undef IMPL_FRWD

#endif

//____________________________________________________________________________//

template <class Left, class Right>
inline assertion_result equal_impl( Left const& left, Right const& right )
{
    return left == right;
}

//____________________________________________________________________________//

inline assertion_result equal_impl( char* left, char const* right ) { return equal_impl( static_cast<char const*>(left), static_cast<char const*>(right) ); }
inline assertion_result equal_impl( char const* left, char* right ) { return equal_impl( static_cast<char const*>(left), static_cast<char const*>(right) ); }
inline assertion_result equal_impl( char* left, char* right )       { return equal_impl( static_cast<char const*>(left), static_cast<char const*>(right) ); }

#if !defined( BOOST_NO_CWCHAR )
BOOST_TEST_DECL assertion_result equal_impl( wchar_t const* left, wchar_t const* right );
inline assertion_result equal_impl( wchar_t* left, wchar_t const* right ) { return equal_impl( static_cast<wchar_t const*>(left), static_cast<wchar_t const*>(right) ); }
inline assertion_result equal_impl( wchar_t const* left, wchar_t* right ) { return equal_impl( static_cast<wchar_t const*>(left), static_cast<wchar_t const*>(right) ); }
inline assertion_result equal_impl( wchar_t* left, wchar_t* right )       { return equal_impl( static_cast<wchar_t const*>(left), static_cast<wchar_t const*>(right) ); }
#endif

//____________________________________________________________________________//

struct equal_impl_frwd {
    template <typename Left, typename Right>
    inline assertion_result
    call_impl( Left const& left, Right const& right, mpl::false_ ) const
    {
        return equal_impl( left, right );
    }

    template <typename Left, typename Right>
    inline assertion_result
    call_impl( Left const& left, Right const& right, mpl::true_ ) const
    {
        return (*this)( right, &left[0] );
    }

    template <typename Left, typename Right>
    inline assertion_result
    operator()( Left const& left, Right const& right ) const
    {
        typedef typename is_array<Left>::type left_is_array;
        return call_impl( left, right, left_is_array() );
    }
};

//____________________________________________________________________________//

struct ne_impl {
    template <class Left, class Right>
    assertion_result operator()( Left const& left, Right const& right )
    {
        return !equal_impl_frwd()( left, right );
    }
};

//____________________________________________________________________________//

struct lt_impl {
    template <class Left, class Right>
    assertion_result operator()( Left const& left, Right const& right )
    {
        return left < right;
    }
};

//____________________________________________________________________________//

struct le_impl {
    template <class Left, class Right>
    assertion_result operator()( Left const& left, Right const& right )
    {
        return left <= right;
    }
};

//____________________________________________________________________________//

struct gt_impl {
    template <class Left, class Right>
    assertion_result operator()( Left const& left, Right const& right )
    {
        return left > right;
    }
};

//____________________________________________________________________________//

struct ge_impl {
    template <class Left, class Right>
    assertion_result operator()( Left const& left, Right const& right )
    {
        return left >= right;
    }
};

//____________________________________________________________________________//

struct equal_coll_impl {
    template <typename Left, typename Right>
    assertion_result operator()( Left left_begin, Left left_end, Right right_begin, Right right_end )
    {
        assertion_result    pr( true );
        std::size_t         pos = 0;

        for( ; left_begin != left_end && right_begin != right_end; ++left_begin, ++right_begin, ++pos ) {
            if( *left_begin != *right_begin ) {
                pr = false;
                pr.message() << "\nMismatch at position " << pos << ": "
                  << ::boost::test_tools::tt_detail::print_helper(*left_begin)
                  << " != "
                  << ::boost::test_tools::tt_detail::print_helper(*right_begin);
            }
        }

        if( left_begin != left_end ) {
            std::size_t r_size = pos;
            while( left_begin != left_end ) {
                ++pos;
                ++left_begin;
            }

            pr = false;
            pr.message() << "\nCollections size mismatch: " << pos << " != " << r_size;
        }

        if( right_begin != right_end ) {
            std::size_t l_size = pos;
            while( right_begin != right_end ) {
                ++pos;
                ++right_begin;
            }

            pr = false;
            pr.message() << "\nCollections size mismatch: " << l_size << " != " << pos;
        }

        return pr;
    }
};

//____________________________________________________________________________//

struct bitwise_equal_impl {
    template <class Left, class Right>
    assertion_result    operator()( Left const& left, Right const& right )
    {
        assertion_result    pr( true );

        std::size_t left_bit_size  = sizeof(Left)*CHAR_BIT;
        std::size_t right_bit_size = sizeof(Right)*CHAR_BIT;

        static Left const leftOne( 1 );
        static Right const rightOne( 1 );

        std::size_t total_bits = left_bit_size < right_bit_size ? left_bit_size : right_bit_size;

        for( std::size_t counter = 0; counter < total_bits; ++counter ) {
            if( ( left & ( leftOne << counter ) ) != ( right & ( rightOne << counter ) ) ) {
                pr = false;
                pr.message() << "\nMismatch at position " << counter;
            }
        }

        if( left_bit_size != right_bit_size ) {
            pr = false;
            pr.message() << "\nOperands bit sizes mismatch: " << left_bit_size << " != " << right_bit_size;
        }

        return pr;
    }
};

//____________________________________________________________________________//

template<typename FPT1, typename FPT2>
struct comp_supertype {
    // deduce "better" type from types of arguments being compared
    // if one type is floating and the second integral we use floating type and
    // value of integral type is promoted to the floating. The same for float and double
    // But we don't want to compare two values of integral types using this tool.
    typedef typename numeric::conversion_traits<FPT1,FPT2>::supertype type;
    BOOST_STATIC_ASSERT_MSG( !is_integral<type>::value, "Only floating-point types can be compared!");
};

} // namespace tt_detail

namespace fpc = math::fpc;

// ************************************************************************** //
// **************               check_is_close                 ************** //
// ************************************************************************** //

struct BOOST_TEST_DECL check_is_close_t {
    // Public typedefs
    typedef assertion_result result_type;

    template<typename FPT1, typename FPT2, typename ToleranceType>
    assertion_result
    operator()( FPT1 left, FPT2 right, ToleranceType tolerance ) const
    {
        fpc::close_at_tolerance<typename tt_detail::comp_supertype<FPT1,FPT2>::type> pred( tolerance, fpc::FPC_STRONG );

        assertion_result ar( pred( left, right ) );

        if( !ar )
            ar.message() << pred.tested_rel_diff();

        return ar;
    }
};

//____________________________________________________________________________//

template<typename FPT1, typename FPT2, typename ToleranceType>
inline assertion_result
check_is_close( FPT1 left, FPT2 right, ToleranceType tolerance )
{
    return check_is_close_t()( left, right, tolerance );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************               check_is_small                 ************** //
// ************************************************************************** //

struct BOOST_TEST_DECL check_is_small_t {
    // Public typedefs
    typedef bool result_type;

    template<typename FPT>
    bool
    operator()( FPT fpv, FPT tolerance ) const
    {
        return fpc::is_small( fpv, tolerance );
    }
};

//____________________________________________________________________________//

template<typename FPT>
inline bool
check_is_small( FPT fpv, FPT tolerance )
{
    return fpc::is_small( fpv, tolerance );
}

//____________________________________________________________________________//

} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_OLD_IMPL_HPP_012705GER
