//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief Collection comparison with enhanced reporting
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_COLLECTION_COMPARISON_OP_HPP_050815GER
#define BOOST_TEST_TOOLS_COLLECTION_COMPARISON_OP_HPP_050815GER

// Boost.Test
#include <boost/test/tools/assertion.hpp>

#include <boost/test/utils/is_forward_iterable.hpp>
#include <boost/test/utils/is_cstring.hpp>

// Boost
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/decay.hpp>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace test_tools {
namespace assertion {

// ************************************************************************** //
// ************* selectors for specialized comparizon routines ************** //
// ************************************************************************** //

template<typename T>
struct specialized_compare : public mpl::false_ {};

template <typename T>
struct is_c_array : public mpl::false_ {};

template<typename T, std::size_t N>
struct is_c_array<T [N]> : public mpl::true_ {};

template<typename T, std::size_t N>
struct is_c_array<T (&)[N]> : public mpl::true_ {};

#define BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(Col)          \
namespace boost { namespace test_tools { namespace assertion {  \
template<>                                                      \
struct specialized_compare<Col> : public mpl::true_ {};         \
}}}                                                             \
/**/

// ************************************************************************** //
// **************            lexicographic_compare             ************** //
// ************************************************************************** //

namespace op {

template <typename OP, bool can_be_equal, bool prefer_shorter,
          typename Lhs, typename Rhs>
inline
typename boost::enable_if_c<
       unit_test::is_forward_iterable<Lhs>::value && !unit_test::is_cstring<Lhs>::value
    && unit_test::is_forward_iterable<Rhs>::value && !unit_test::is_cstring<Rhs>::value,
    assertion_result>::type
lexicographic_compare( Lhs const& lhs, Rhs const& rhs )
{
    assertion_result ar( true );

    typedef unit_test::bt_iterator_traits<Lhs> t_Lhs_iterator;
    typedef unit_test::bt_iterator_traits<Rhs> t_Rhs_iterator;

    typename t_Lhs_iterator::const_iterator first1 = t_Lhs_iterator::begin(lhs);
    typename t_Rhs_iterator::const_iterator first2 = t_Rhs_iterator::begin(rhs);
    typename t_Lhs_iterator::const_iterator last1  = t_Lhs_iterator::end(lhs);
    typename t_Rhs_iterator::const_iterator last2  = t_Rhs_iterator::end(rhs);
    std::size_t                             pos    = 0;

    for( ; (first1 != last1) && (first2 != last2); ++first1, ++first2, ++pos ) {
        assertion_result const& element_ar = OP::eval(*first1, *first2);
        if( !can_be_equal && element_ar )
            return ar; // a < b

        assertion_result const& reverse_ar = OP::eval(*first2, *first1);
        if( element_ar && !reverse_ar )
            return ar; // a<=b and !(b<=a) => a < b => return true

        if( element_ar || !reverse_ar ) {
            continue; // (a<=b and b<=a) or (!(a<b) and !(b<a)) => a == b => keep looking
        }

        // !(a<=b) and b<=a => b < a => return false
        ar = false;
        ar.message() << "\nFailure at position " << pos << ":";
        ar.message() << "\n  - condition [" << tt_detail::print_helper(*first1) << OP::forward() << tt_detail::print_helper(*first2) << "] is false";
        if(!element_ar.has_empty_message())
            ar.message() << ": " << element_ar.message();
        ar.message() << "\n  - inverse condition [" << tt_detail::print_helper(*first2) << OP::forward() << tt_detail::print_helper(*first1) << "] is true";
        if(!reverse_ar.has_empty_message())
            ar.message() << ": " << reverse_ar.message();
        return ar;
    }

    if( first1 != last1 ) {
        if( prefer_shorter ) {
            ar = false;
            ar.message() << "\nFirst collection has extra trailing elements.";
        }
    }
    else if( first2 != last2 ) {
        if( !prefer_shorter ) {
            ar = false;
            ar.message() << "\nSecond collection has extra trailing elements.";
        }
    }
    else if( !can_be_equal ) {
        ar = false;
        ar.message() << "\nCollections appear to be equal.";
    }

    return ar;
}

template <typename OP, bool can_be_equal, bool prefer_shorter,
          typename Lhs, typename Rhs>
inline
typename boost::enable_if_c<
    (unit_test::is_cstring<Lhs>::value || unit_test::is_cstring<Rhs>::value),
    assertion_result>::type
lexicographic_compare( Lhs const& lhs, Rhs const& rhs )
{
    typedef typename unit_test::deduce_cstring_transform<Lhs>::type lhs_char_type;
    typedef typename unit_test::deduce_cstring_transform<Rhs>::type rhs_char_type;

    return lexicographic_compare<OP, can_be_equal, prefer_shorter>(
        lhs_char_type(lhs),
        rhs_char_type(rhs));
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************               equality_compare               ************** //
// ************************************************************************** //

template <typename OP, typename Lhs, typename Rhs>
inline
typename boost::enable_if_c<
       unit_test::is_forward_iterable<Lhs>::value && !unit_test::is_cstring<Lhs>::value
    && unit_test::is_forward_iterable<Rhs>::value && !unit_test::is_cstring<Rhs>::value,
    assertion_result>::type
element_compare( Lhs const& lhs, Rhs const& rhs )
{
    typedef unit_test::bt_iterator_traits<Lhs> t_Lhs_iterator;
    typedef unit_test::bt_iterator_traits<Rhs> t_Rhs_iterator;

    assertion_result ar( true );

    if( t_Lhs_iterator::size(lhs) != t_Rhs_iterator::size(rhs) ) {
        ar = false;
        ar.message() << "\nCollections size mismatch: " << t_Lhs_iterator::size(lhs) << " != " << t_Rhs_iterator::size(rhs);
        return ar;
    }

    typename t_Lhs_iterator::const_iterator left  = t_Lhs_iterator::begin(lhs);
    typename t_Rhs_iterator::const_iterator right = t_Rhs_iterator::begin(rhs);
    std::size_t                             pos   = 0;

    for( ; pos < t_Lhs_iterator::size(lhs); ++left, ++right, ++pos ) {
        assertion_result const element_ar = OP::eval( *left, *right );
        if( element_ar )
            continue;

        ar = false;
        ar.message() << "\n  - mismatch at position " << pos << ": ["
                     << tt_detail::print_helper(*left)
                     << OP::forward()
                     << tt_detail::print_helper(*right)
                     << "] is false";
        if(!element_ar.has_empty_message())
            ar.message() << ": " << element_ar.message();
    }

    return ar;
}

// In case string comparison is branching here
template <typename OP, typename Lhs, typename Rhs>
inline
typename boost::enable_if_c<
    (unit_test::is_cstring<Lhs>::value || unit_test::is_cstring<Rhs>::value),
    assertion_result>::type
element_compare( Lhs const& lhs, Rhs const& rhs )
{
    typedef typename unit_test::deduce_cstring_transform<Lhs>::type lhs_char_type;
    typedef typename unit_test::deduce_cstring_transform<Rhs>::type rhs_char_type;

    return element_compare<OP>(lhs_char_type(lhs),
                               rhs_char_type(rhs));
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************             non_equality_compare             ************** //
// ************************************************************************** //

template <typename OP, typename Lhs, typename Rhs>
inline assertion_result
non_equality_compare( Lhs const& lhs, Rhs const& rhs )
{
    typedef unit_test::bt_iterator_traits<Lhs> t_Lhs_iterator;
    typedef unit_test::bt_iterator_traits<Rhs> t_Rhs_iterator;

    assertion_result ar( true );

    if( t_Lhs_iterator::size(lhs) != t_Rhs_iterator::size(rhs) )
        return ar;

    typename t_Lhs_iterator::const_iterator left = t_Lhs_iterator::begin(lhs);
    typename t_Rhs_iterator::const_iterator right = t_Rhs_iterator::begin(rhs);
    typename t_Lhs_iterator::const_iterator end = t_Lhs_iterator::end(lhs);

    for( ; left != end; ++left, ++right ) {
        if( OP::eval( *left, *right ) )
            return ar;
    }

    ar = false;
    ar.message() << "\nCollections appear to be equal";

    return ar;
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************                   cctraits                   ************** //
// ************************************************************************** //
// set of collection comparison traits per comparison OP

template<typename OP>
struct cctraits;

template<typename Lhs, typename Rhs>
struct cctraits<op::EQ<Lhs, Rhs> > {
    typedef specialized_compare<Lhs> is_specialized;
};

template<typename Lhs, typename Rhs>
struct cctraits<op::NE<Lhs, Rhs> > {
    typedef specialized_compare<Lhs> is_specialized;
};

template<typename Lhs, typename Rhs>
struct cctraits<op::LT<Lhs, Rhs> > {
    static const bool can_be_equal = false;
    static const bool prefer_short = true;

    typedef specialized_compare<Lhs> is_specialized;
};

template<typename Lhs, typename Rhs>
struct cctraits<op::LE<Lhs, Rhs> > {
    static const bool can_be_equal = true;
    static const bool prefer_short = true;

    typedef specialized_compare<Lhs> is_specialized;
};

template<typename Lhs, typename Rhs>
struct cctraits<op::GT<Lhs, Rhs> > {
    static const bool can_be_equal = false;
    static const bool prefer_short = false;

    typedef specialized_compare<Lhs> is_specialized;
};

template<typename Lhs, typename Rhs>
struct cctraits<op::GE<Lhs, Rhs> > {
    static const bool can_be_equal = true;
    static const bool prefer_short = false;

    typedef specialized_compare<Lhs> is_specialized;
};

// ************************************************************************** //
// **************              compare_collections             ************** //
// ************************************************************************** //
// Overloaded set of functions dispatching to specific implementation of comparison

template <typename Lhs, typename Rhs, typename L, typename R>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<op::EQ<L, R> >*, mpl::true_ )
{
    return assertion::op::element_compare<op::EQ<L, R> >( lhs, rhs );
}

//____________________________________________________________________________//

template <typename Lhs, typename Rhs, typename L, typename R>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<op::EQ<L, R> >*, mpl::false_ )
{
    return lhs == rhs;
}

//____________________________________________________________________________//

template <typename Lhs, typename Rhs, typename L, typename R>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<op::NE<L, R> >*, mpl::true_ )
{
    return assertion::op::non_equality_compare<op::NE<L, R> >( lhs, rhs );
}

//____________________________________________________________________________//

template <typename Lhs, typename Rhs, typename L, typename R>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<op::NE<L, R> >*, mpl::false_ )
{
    return lhs != rhs;
}

//____________________________________________________________________________//

template <typename OP, typename Lhs, typename Rhs>
inline assertion_result
lexicographic_compare( Lhs const& lhs, Rhs const& rhs )
{
    return assertion::op::lexicographic_compare<OP, cctraits<OP>::can_be_equal, cctraits<OP>::prefer_short>( lhs, rhs );
}

//____________________________________________________________________________//

template <typename Lhs, typename Rhs, typename OP>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<OP>*, mpl::true_ )
{
    return lexicographic_compare<OP>( lhs, rhs );
}

//____________________________________________________________________________//

template <typename Lhs, typename Rhs, typename L, typename R>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<op::LT<L, R> >*, mpl::false_ )
{
    return lhs < rhs;
}

//____________________________________________________________________________//

template <typename Lhs, typename Rhs, typename L, typename R>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<op::LE<L, R> >*, mpl::false_ )
{
    return lhs <= rhs;
}

//____________________________________________________________________________//

template <typename Lhs, typename Rhs, typename L, typename R>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<op::GT<L, R> >*, mpl::false_ )
{
    return lhs > rhs;
}

//____________________________________________________________________________//

template <typename Lhs, typename Rhs, typename L, typename R>
inline assertion_result
compare_collections( Lhs const& lhs, Rhs const& rhs, boost::type<op::GE<L, R> >*, mpl::false_ )
{
    return lhs >= rhs;
}

//____________________________________________________________________________//

// ************************************************************************** //
// ********* specialization of comparison operators for collections ********* //
// ************************************************************************** //

#define DEFINE_COLLECTION_COMPARISON( oper, name, rev, name_inverse ) \
template<typename Lhs,typename Rhs>                                 \
struct name<Lhs,Rhs,typename boost::enable_if_c<                    \
    unit_test::is_forward_iterable<Lhs>::value                      \
    &&   !unit_test::is_cstring_comparable<Lhs>::value              \
    && unit_test::is_forward_iterable<Rhs>::value                   \
    &&   !unit_test::is_cstring_comparable<Rhs>::value>::type> {    \
public:                                                             \
    typedef assertion_result result_type;                           \
    typedef name_inverse<Lhs, Rhs> inverse;                         \
    typedef unit_test::bt_iterator_traits<Lhs> t_Lhs_iterator_helper; \
    typedef unit_test::bt_iterator_traits<Rhs> t_Rhs_iterator_helper; \
                                                                    \
    typedef name<Lhs, Rhs> OP;                                      \
                                                                    \
    typedef typename                                                \
        mpl::if_c<                                                  \
          mpl::or_<                                                 \
              typename is_c_array<Lhs>::type,                       \
              typename is_c_array<Rhs>::type                        \
          >::value,                                                 \
          mpl::true_,                                               \
          typename                                                  \
              mpl::if_c<is_same<typename decay<Lhs>::type,          \
                                typename decay<Rhs>::type>::value,  \
                        typename cctraits<OP>::is_specialized,      \
                        mpl::false_>::type                          \
          >::type is_specialized;                                   \
                                                                    \
    typedef name<typename t_Lhs_iterator_helper::value_type,        \
                 typename t_Rhs_iterator_helper::value_type         \
                 > elem_op;                                         \
                                                                    \
    static assertion_result                                         \
    eval( Lhs const& lhs, Rhs const& rhs)                           \
    {                                                               \
        return assertion::op::compare_collections( lhs, rhs,        \
            (boost::type<elem_op>*)0,                               \
            is_specialized() );                                     \
    }                                                               \
                                                                    \
    template<typename PrevExprType>                                 \
    static void                                                     \
    report( std::ostream&,                                          \
            PrevExprType const&,                                    \
            Rhs const& ) {}                                         \
                                                                    \
    static char const* forward()                                    \
    { return " " #oper " "; }                                       \
    static char const* revert()                                     \
    { return " " #rev " "; }                                        \
                                                                    \
};                                                                  \
/**/

BOOST_TEST_FOR_EACH_COMP_OP( DEFINE_COLLECTION_COMPARISON )
#undef DEFINE_COLLECTION_COMPARISON

//____________________________________________________________________________//

} // namespace op
} // namespace assertion
} // namespace test_tools
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_TOOLS_COLLECTION_COMPARISON_OP_HPP_050815GER
