//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief Floating point comparison with enhanced reporting
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_FPC_OP_HPP_050915GER
#define BOOST_TEST_TOOLS_FPC_OP_HPP_050915GER

// Boost.Test
#include <boost/test/tools/assertion.hpp>

#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/tools/fpc_tolerance.hpp>

// Boost
#include <boost/type_traits/common_type.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/utility/enable_if.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {
namespace assertion {
namespace op {

// ************************************************************************** //
// **************                   fpctraits                  ************** //
// ************************************************************************** //
// set of floating point comparison traits per comparison OP

template<typename OP>
struct fpctraits {
    // indicate if we should perform the operation with a "logical OR"
    // with the "equality under tolerance".
    static const bool equality_logical_disjunction = true;
};

template <typename Lhs, typename Rhs>
struct fpctraits<op::LT<Lhs,Rhs> > {
    static const bool equality_logical_disjunction = false;
};

template <typename Lhs, typename Rhs>
struct fpctraits<op::GT<Lhs,Rhs> > {
    static const bool equality_logical_disjunction = false;
};

//____________________________________________________________________________//

// ************************************************************************** //
// ************** set of overloads to select correct fpc algo  ************** //
// ************************************************************************** //
// we really only care about EQ vs NE. All other comparisons use direct first
// and then need EQ. For example a <= b (tolerance t) IFF a <= b OR a == b (tolerance t)

template <typename FPT, typename Lhs, typename Rhs, typename OP>
inline assertion_result
compare_fpv( Lhs const& lhs, Rhs const& rhs, OP* cmp_operator)
{
    assertion_result result_direct_compare = cmp_operator->eval_direct(lhs, rhs);
    if(fpctraits<OP>::equality_logical_disjunction) {
        // this look like this can be simplified, but combining result && compare_fpv
        // looses the message in the return value of compare_fpv
        if( result_direct_compare ) {
            result_direct_compare.message() << "operation" << OP::forward() << "on arguments yields 'true'.";
            return result_direct_compare;
        }
        // result || compare_fpv(EQ)
        assertion_result result_eq = compare_fpv<FPT>(lhs, rhs, (op::EQ<Lhs, Rhs>*)0);
        result_direct_compare = result_direct_compare || result_eq;
        if( !result_eq ) {
            result_direct_compare.message() << "operation" << op::EQ<Lhs, Rhs>::forward() << "on arguments yields 'false': " << result_eq.message() << ".";
        }
        return result_direct_compare;
    }
    if( !result_direct_compare ) {
        result_direct_compare.message() << "operation" << OP::forward() << " on arguments yields 'false'.";
        return result_direct_compare;
    }
    // result && compare_fpv(NE)
    assertion_result result_neq = compare_fpv<FPT>(lhs, rhs, (op::NE<Lhs, Rhs>*)0);
    result_direct_compare = result_direct_compare && result_neq;
    if( !result_neq ) {
        result_direct_compare.message() << "operation" << op::NE<Lhs, Rhs>::forward() << "on arguments yields 'false': " << result_neq.message() << ".";
    }
    return result_direct_compare;
}

//____________________________________________________________________________//

template <typename FPT, typename Lhs, typename Rhs>
inline assertion_result
compare_fpv_near_zero( FPT const& fpv, op::EQ<Lhs,Rhs>* )
{
    fpc::small_with_tolerance<FPT> P( fpc_tolerance<FPT>() );

    assertion_result ar( P( fpv ) );
    if( !ar )
        ar.message() << "absolute value exceeds tolerance [|" << fpv << "| > "<< fpc_tolerance<FPT>() << ']';

    return ar;
}

//____________________________________________________________________________//

template <typename FPT, typename Lhs, typename Rhs>
inline assertion_result
compare_fpv_near_zero( FPT const& fpv, op::NE<Lhs,Rhs>* )
{
    fpc::small_with_tolerance<FPT> P( fpc_tolerance<FPT>() );

    assertion_result ar( !P( fpv ) );
    if( !ar )
        ar.message() << "absolute value is within tolerance [|" << fpv << "| < "<< fpc_tolerance<FPT>() << ']';
    return ar;
}

//____________________________________________________________________________//

template <typename FPT, typename Lhs, typename Rhs>
inline assertion_result
compare_fpv( Lhs const& lhs, Rhs const& rhs, op::EQ<Lhs,Rhs>* )
{
    if( lhs == 0 ) {
        return compare_fpv_near_zero<FPT>( rhs, (op::EQ<Lhs,Rhs>*)0 );
    }
    else if( rhs == 0) {
        return compare_fpv_near_zero<FPT>( lhs, (op::EQ<Lhs,Rhs>*)0 );
    }
    else {
        fpc::close_at_tolerance<FPT> P( fpc_tolerance<FPT>(), fpc::FPC_STRONG );

        assertion_result ar( P( lhs, rhs ) );
        if( !ar )
            ar.message() << "relative difference exceeds tolerance ["
                         << P.tested_rel_diff() << " > " << P.fraction_tolerance() << ']';
        return ar;
    }
}

//____________________________________________________________________________//

template <typename FPT, typename Lhs, typename Rhs>
inline assertion_result
compare_fpv( Lhs const& lhs, Rhs const& rhs, op::NE<Lhs,Rhs>* )
{
    if( lhs == 0 ) {
        return compare_fpv_near_zero<FPT>( rhs, (op::NE<Lhs,Rhs>*)0 );
    }
    else if( rhs == 0 ) {
        return compare_fpv_near_zero<FPT>( lhs, (op::NE<Lhs,Rhs>*)0 );
    }
    else {
        fpc::close_at_tolerance<FPT> P( fpc_tolerance<FPT>(), fpc::FPC_WEAK );

        assertion_result ar( !P( lhs, rhs ) );
        if( !ar )
            ar.message() << "relative difference is within tolerance ["
                         << P.tested_rel_diff() << " < " << fpc_tolerance<FPT>() << ']';

        return ar;
    }
}

//____________________________________________________________________________//

#define DEFINE_FPV_COMPARISON( oper, name, rev, name_inverse )          \
template<typename Lhs,typename Rhs>                                     \
struct name<Lhs,Rhs,typename boost::enable_if_c<                        \
    (fpc::tolerance_based<Lhs>::value &&                                \
     fpc::tolerance_based<Rhs>::value) ||                               \
    (fpc::tolerance_based<Lhs>::value &&                                \
     boost::is_arithmetic<Rhs>::value) ||                               \
    (boost::is_arithmetic<Lhs>::value &&                                \
     fpc::tolerance_based<Rhs>::value)                                  \
     >::type> {                                                         \
public:                                                                 \
    typedef typename common_type<Lhs,Rhs>::type FPT;                    \
    typedef name<Lhs,Rhs> OP;                                           \
    typedef name_inverse<Lhs, Rhs> inverse;                             \
                                                                        \
    typedef assertion_result result_type;                               \
                                                                        \
    static bool                                                         \
    eval_direct( Lhs const& lhs, Rhs const& rhs )                       \
    {                                                                   \
        return lhs oper rhs;                                            \
    }                                                                   \
                                                                        \
    static assertion_result                                             \
    eval( Lhs const& lhs, Rhs const& rhs )                              \
    {                                                                   \
        if( fpc_tolerance<FPT>() == FPT(0)                              \
            || (std::numeric_limits<Lhs>::has_infinity                  \
                && (lhs == std::numeric_limits<Lhs>::infinity()))       \
            || (std::numeric_limits<Rhs>::has_infinity                  \
                && (rhs == std::numeric_limits<Rhs>::infinity())))      \
        {                                                               \
            return eval_direct( lhs, rhs );                             \
        }                                                               \
                                                                        \
        return compare_fpv<FPT>( lhs, rhs, (OP*)0 );                    \
    }                                                                   \
                                                                        \
    template<typename PrevExprType>                                     \
    static void                                                         \
    report( std::ostream&       ostr,                                   \
            PrevExprType const& lhs,                                    \
            Rhs const&          rhs )                                   \
    {                                                                   \
        lhs.report( ostr );                                             \
        ostr << revert()                                                \
             << tt_detail::print_helper( rhs );                         \
    }                                                                   \
                                                                        \
    static char const* forward()                                        \
    { return " " #oper " "; }                                           \
    static char const* revert()                                         \
    { return " " #rev " "; }                                            \
};                                                                      \
/**/

BOOST_TEST_FOR_EACH_COMP_OP( DEFINE_FPV_COMPARISON )
#undef DEFINE_FPV_COMPARISON

//____________________________________________________________________________//

} // namespace op
} // namespace assertion
} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_FPC_OP_HPP_050915GER
