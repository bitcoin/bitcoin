//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief C string comparison with enhanced reporting
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_CSTRING_COMPARISON_OP_HPP_050815GER
#define BOOST_TEST_TOOLS_CSTRING_COMPARISON_OP_HPP_050815GER

// Boost.Test
#include <boost/test/tools/assertion.hpp>

#include <boost/test/utils/is_cstring.hpp>
#include <boost/test/utils/basic_cstring/compare.hpp>

// Boost
#include <boost/utility/enable_if.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {
namespace assertion {
namespace op {

// ************************************************************************** //
// **************               string_compare                 ************** //
// ************************************************************************** //

#define DEFINE_CSTRING_COMPARISON( oper, name, rev, name_inverse )  \
template<typename Lhs,typename Rhs>                                 \
struct name<Lhs,Rhs,typename boost::enable_if_c<                    \
    (   unit_test::is_cstring_comparable<Lhs>::value                \
     && unit_test::is_cstring_comparable<Rhs>::value)               \
    >::type >                                                       \
{                                                                   \
    typedef typename unit_test::deduce_cstring_transform<Lhs>::type lhs_char_type; \
    typedef typename unit_test::deduce_cstring_transform<Rhs>::type rhs_char_type; \
public:                                                             \
    typedef assertion_result result_type;                           \
    typedef name_inverse<Lhs, Rhs> inverse;                         \
                                                                    \
    typedef name<                                                   \
        typename lhs_char_type::value_type,                         \
        typename rhs_char_type::value_type> elem_op;                \
                                                                    \
    static bool                                                     \
    eval( Lhs const& lhs, Rhs const& rhs)                           \
    {                                                               \
        return lhs_char_type(lhs) oper rhs_char_type(rhs);          \
    }                                                               \
                                                                    \
    template<typename PrevExprType>                                 \
    static void                                                     \
    report( std::ostream&       ostr,                               \
            PrevExprType const& lhs,                                \
            Rhs const&          rhs)                                \
    {                                                               \
        lhs.report( ostr );                                         \
        ostr << revert()                                            \
             << tt_detail::print_helper( rhs );                     \
    }                                                               \
                                                                    \
    static char const* forward()                                    \
    { return " " #oper " "; }                                       \
    static char const* revert()                                     \
    { return " " #rev " "; }                                        \
};                                                                  \
/**/

BOOST_TEST_FOR_EACH_COMP_OP( DEFINE_CSTRING_COMPARISON )
#undef DEFINE_CSTRING_COMPARISON

//____________________________________________________________________________//

} // namespace op
} // namespace assertion
} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_CSTRING_COMPARISON_OP_HPP_050815GER

