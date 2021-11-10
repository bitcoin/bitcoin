//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file 
//!@brief algorithms for comparing floating point values
// ***************************************************************************

#ifndef BOOST_TEST_FLOATING_POINT_COMPARISON_HPP_071894GER
#define BOOST_TEST_FLOATING_POINT_COMPARISON_HPP_071894GER

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/tools/assertion_result.hpp>

// Boost
#include <boost/limits.hpp>  // for std::numeric_limits
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/utility/enable_if.hpp>

// STL
#include <iosfwd>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace math {
namespace fpc {

// ************************************************************************** //
// **************              fpc::tolerance_based            ************** //
// ************************************************************************** //


//! @internal
//! Protects the instanciation of std::numeric_limits from non-supported types (eg. T=array)
template <typename T, bool enabled>
struct tolerance_based_delegate;

template <typename T>
struct tolerance_based_delegate<T, false> : mpl::false_ {};

// from https://stackoverflow.com/a/16509511/1617295
template<typename T>
class is_abstract_class_or_function
{
    typedef char (&Two)[2];
    template<typename U> static char test(U(*)[1]);
    template<typename U> static Two test(...);

public:
    static const bool value =
           !is_reference<T>::value
        && !is_void<T>::value
        && (sizeof(test<T>(0)) == sizeof(Two));
};

// warning: we cannot instanciate std::numeric_limits for incomplete types, we use is_abstract_class_or_function
// prior to the specialization below
template <typename T>
struct tolerance_based_delegate<T, true>
: mpl::bool_<
    is_floating_point<T>::value ||
    (!std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_specialized && !std::numeric_limits<T>::is_exact)>
{};


/*!@brief Indicates if a type can be compared using a tolerance scheme
 *
 * This is a metafunction that should evaluate to @c mpl::true_ if the type
 * @c T can be compared using a tolerance based method, typically for floating point
 * types.
 *
 * This metafunction can be specialized further to declare user types that are
 * floating point (eg. boost.multiprecision).
 */
template <typename T>
struct tolerance_based : tolerance_based_delegate<T, !is_array<T>::value && !is_abstract_class_or_function<T>::value>::type {};

// ************************************************************************** //
// **************                 fpc::strength                ************** //
// ************************************************************************** //

//! Method for comparing floating point numbers
enum strength {
    FPC_STRONG, //!< "Very close"   - equation 2' in docs, the default
    FPC_WEAK    //!< "Close enough" - equation 3' in docs.
};


// ************************************************************************** //
// **************         tolerance presentation types         ************** //
// ************************************************************************** //

template<typename FPT>
struct percent_tolerance_t {
    explicit    percent_tolerance_t( FPT v ) : m_value( v ) {}

    FPT m_value;
};

//____________________________________________________________________________//

template<typename FPT>
inline std::ostream& operator<<( std::ostream& out, percent_tolerance_t<FPT> t )
{
    return out << t.m_value;
}

//____________________________________________________________________________//

template<typename FPT>
inline percent_tolerance_t<FPT>
percent_tolerance( FPT v )
{
    return percent_tolerance_t<FPT>( v );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                    details                   ************** //
// ************************************************************************** //

namespace fpc_detail {

// FPT is Floating-Point Type: float, double, long double or User-Defined.
template<typename FPT>
inline FPT
fpt_abs( FPT fpv ) 
{
    return fpv < static_cast<FPT>(0) ? -fpv : fpv;
}

//____________________________________________________________________________//

template<typename FPT>
struct fpt_specialized_limits
{
  static FPT    min_value() { return (std::numeric_limits<FPT>::min)(); }
  static FPT    max_value() { return (std::numeric_limits<FPT>::max)(); }
};

template<typename FPT>
struct fpt_non_specialized_limits
{
  static FPT    min_value() { return static_cast<FPT>(0); }
  static FPT    max_value() { return static_cast<FPT>(1000000); } // for our purposes it doesn't really matter what value is returned here
};

template<typename FPT>
struct fpt_limits : boost::conditional<std::numeric_limits<FPT>::is_specialized,
                                       fpt_specialized_limits<FPT>,
                                       fpt_non_specialized_limits<FPT>
                                      >::type
{};

//____________________________________________________________________________//

// both f1 and f2 are unsigned here
template<typename FPT>
inline FPT
safe_fpt_division( FPT f1, FPT f2 )
{
    // Avoid overflow.
    if( (f2 < static_cast<FPT>(1))  && (f1 > f2*fpt_limits<FPT>::max_value()) )
        return fpt_limits<FPT>::max_value();

    // Avoid underflow.
    if( (fpt_abs(f1) <= fpt_limits<FPT>::min_value()) ||
        ((f2 > static_cast<FPT>(1)) && (f1 < f2*fpt_limits<FPT>::min_value())) )
        return static_cast<FPT>(0);

    return f1/f2;
}

//____________________________________________________________________________//

template<typename FPT, typename ToleranceType>
inline FPT
fraction_tolerance( ToleranceType tolerance )
{
  return static_cast<FPT>(tolerance);
} 

//____________________________________________________________________________//

template<typename FPT2, typename FPT>
inline FPT2
fraction_tolerance( percent_tolerance_t<FPT> tolerance )
{
    return FPT2(tolerance.m_value)*FPT2(0.01); 
}

//____________________________________________________________________________//

} // namespace fpc_detail

// ************************************************************************** //
// **************             close_at_tolerance               ************** //
// ************************************************************************** //


/*!@brief Predicate for comparing floating point numbers
 *
 * This predicate is used to compare floating point numbers. In addition the comparison produces maximum 
 * related difference, which can be used to generate detailed error message
 * The methods for comparing floating points are detailed in the documentation. The method is chosen
 * by the @ref boost::math::fpc::strength given at construction.
 *
 * This predicate is not suitable for comparing to 0 or to infinity.
 */
template<typename FPT>
class close_at_tolerance {
public:
    // Public typedefs
    typedef bool result_type;

    // Constructor
    template<typename ToleranceType>
    explicit    close_at_tolerance( ToleranceType tolerance, fpc::strength fpc_strength = FPC_STRONG ) 
    : m_fraction_tolerance( fpc_detail::fraction_tolerance<FPT>( tolerance ) )
    , m_strength( fpc_strength )
    , m_tested_rel_diff( 0 )
    {
        BOOST_ASSERT_MSG( m_fraction_tolerance >= FPT(0), "tolerance must not be negative!" ); // no reason for tolerance to be negative
    }

    // Access methods
    //! Returns the tolerance
    FPT                 fraction_tolerance() const  { return m_fraction_tolerance; }

    //! Returns the comparison method
    fpc::strength       strength() const            { return m_strength; }

    //! Returns the failing fraction
    FPT                 tested_rel_diff() const     { return m_tested_rel_diff; }

    /*! Compares two floating point numbers a and b such that their "left" relative difference |a-b|/a and/or
     * "right" relative difference |a-b|/b does not exceed specified relative (fraction) tolerance.
     *
     *  @param[in] left first floating point number to be compared
     *  @param[in] right second floating point number to be compared
     *
     * What is reported by @c tested_rel_diff in case of failure depends on the comparison method:
     * - for @c FPC_STRONG: the max of the two fractions
     * - for @c FPC_WEAK: the min of the two fractions
     * The rationale behind is to report the tolerance to set in order to make a test pass.
     */
    bool                operator()( FPT left, FPT right ) const
    {
        FPT diff              = fpc_detail::fpt_abs<FPT>( left - right );
        FPT fraction_of_right = fpc_detail::safe_fpt_division( diff, fpc_detail::fpt_abs( right ) );
        FPT fraction_of_left  = fpc_detail::safe_fpt_division( diff, fpc_detail::fpt_abs( left ) );

        FPT max_rel_diff = (std::max)( fraction_of_left, fraction_of_right );
        FPT min_rel_diff = (std::min)( fraction_of_left, fraction_of_right );

        m_tested_rel_diff = m_strength == FPC_STRONG ? max_rel_diff : min_rel_diff;

        return m_tested_rel_diff <= m_fraction_tolerance;
    }

private:
    // Data members
    FPT                 m_fraction_tolerance;
    fpc::strength       m_strength;
    mutable FPT         m_tested_rel_diff;
};

// ************************************************************************** //
// **************            small_with_tolerance              ************** //
// ************************************************************************** //


/*!@brief Predicate for comparing floating point numbers against 0
 *
 * Serves the same purpose as boost::math::fpc::close_at_tolerance, but used when one
 * of the operand is null. 
 */
template<typename FPT>
class small_with_tolerance {
public:
    // Public typedefs
    typedef bool result_type;

    // Constructor
    explicit    small_with_tolerance( FPT tolerance ) // <= absolute tolerance
    : m_tolerance( tolerance )
    {
        BOOST_ASSERT( m_tolerance >= FPT(0) ); // no reason for the tolerance to be negative
    }

    // Action method
    bool        operator()( FPT fpv ) const
    {
        return fpc::fpc_detail::fpt_abs( fpv ) <= m_tolerance;
    }

private:
    // Data members
    FPT         m_tolerance;
};

// ************************************************************************** //
// **************                  is_small                    ************** //
// ************************************************************************** //

template<typename FPT>
inline bool
is_small( FPT fpv, FPT tolerance )
{
    return small_with_tolerance<FPT>( tolerance )( fpv );
}

//____________________________________________________________________________//

} // namespace fpc
} // namespace math
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_FLOATING_POINT_COMAPARISON_HPP_071894GER
