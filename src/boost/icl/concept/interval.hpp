/*-----------------------------------------------------------------------------+
Copyright (c) 2010-2010: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/
#ifndef BOOST_ICL_CONCEPT_INTERVAL_HPP_JOFA_100323
#define BOOST_ICL_CONCEPT_INTERVAL_HPP_JOFA_100323

#include <boost/assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/not.hpp>
#include <boost/icl/detail/design_config.hpp>
#include <boost/icl/type_traits/unit_element.hpp>
#include <boost/icl/type_traits/identity_element.hpp>
#include <boost/icl/type_traits/infinity.hpp>
#include <boost/icl/type_traits/succ_pred.hpp>
#include <boost/icl/type_traits/is_numeric.hpp>
#include <boost/icl/type_traits/is_discrete.hpp>
#include <boost/icl/type_traits/is_continuous.hpp>
#include <boost/icl/type_traits/is_asymmetric_interval.hpp>
#include <boost/icl/type_traits/is_discrete_interval.hpp>
#include <boost/icl/type_traits/is_continuous_interval.hpp>

#include <boost/icl/concept/interval_bounds.hpp>
#include <boost/icl/interval_traits.hpp>
#include <boost/icl/dynamic_interval_traits.hpp>

#include <algorithm>

namespace boost{namespace icl
{

//==============================================================================
//= Ordering
//==============================================================================
template<class Type>
inline typename enable_if<is_interval<Type>, bool>::type
domain_less(const typename interval_traits<Type>::domain_type& left,
            const typename interval_traits<Type>::domain_type& right)
{
    return typename interval_traits<Type>::domain_compare()(left, right);
}

template<class Type>
inline typename enable_if<is_interval<Type>, bool>::type
domain_less_equal(const typename interval_traits<Type>::domain_type& left,
                  const typename interval_traits<Type>::domain_type& right)
{
    return !(typename interval_traits<Type>::domain_compare()(right, left));
}

template<class Type>
inline typename enable_if<is_interval<Type>, bool>::type
domain_equal(const typename interval_traits<Type>::domain_type& left,
             const typename interval_traits<Type>::domain_type& right)
{
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    return !(domain_compare()(left, right)) && !(domain_compare()(right, left));
}

template<class Type>
inline typename enable_if< is_interval<Type>
                         , typename interval_traits<Type>::domain_type>::type
domain_next(const typename interval_traits<Type>::domain_type value)
{
    typedef typename interval_traits<Type>::domain_type domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    return icl::successor<domain_type,domain_compare>::apply(value);
}

template<class Type>
inline typename enable_if< is_interval<Type>
                         , typename interval_traits<Type>::domain_type>::type
domain_prior(const typename interval_traits<Type>::domain_type value)
{
    typedef typename interval_traits<Type>::domain_type domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    return icl::predecessor<domain_type,domain_compare>::apply(value);
}

//==============================================================================
//= Construct<Interval> singleton
//==============================================================================
template<class Type>
typename enable_if
<
    mpl::and_< is_static_right_open<Type>
             , is_discrete<typename interval_traits<Type>::domain_type> >
  , Type
>::type
singleton(const typename interval_traits<Type>::domain_type& value)
{
    //ASSERT: This always creates an interval with exactly one element
    return interval_traits<Type>::construct(value, domain_next<Type>(value));
}

template<class Type>
typename enable_if
<
    mpl::and_< is_static_left_open<Type>
             , is_discrete<typename interval_traits<Type>::domain_type> >
  , Type
>::type
singleton(const typename interval_traits<Type>::domain_type& value)
{
    //ASSERT: This always creates an interval with exactly one element
    typedef typename interval_traits<Type>::domain_type    domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                 ::is_less_than(value) ));

    return interval_traits<Type>::construct(domain_prior<Type>(value), value);
}

template<class Type>
typename enable_if<is_discrete_static_open<Type>, Type>::type
singleton(const typename interval_traits<Type>::domain_type& value)
{
    //ASSERT: This always creates an interval with exactly one element
    typedef typename interval_traits<Type>::domain_type    domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                 ::is_less_than(value)));

    return interval_traits<Type>::construct( domain_prior<Type>(value)
                                           , domain_next<Type>(value));
}

template<class Type>
typename enable_if<is_discrete_static_closed<Type>, Type>::type
singleton(const typename interval_traits<Type>::domain_type& value)
{
    //ASSERT: This always creates an interval with exactly one element
    return interval_traits<Type>::construct(value, value);
}

template<class Type>
typename enable_if<has_dynamic_bounds<Type>, Type>::type
singleton(const typename interval_traits<Type>::domain_type& value)
{
    return dynamic_interval_traits<Type>::construct(value, value, interval_bounds::closed());
}

namespace detail
{

//==============================================================================
//= Construct<Interval> unit_trail == generalized singleton
// The smallest interval on an incrementable (and decrementable) type that can
// be constructed using ++ and -- and such that it contains a given value.
// If 'Type' is discrete, 'unit_trail' and 'singleton' are identical. So we
// can view 'unit_trail' as a generalized singleton for static intervals of
// continuous types.
//==============================================================================
template<class Type>
typename enable_if
<
    mpl::and_< is_static_right_open<Type>
             , boost::detail::is_incrementable<typename interval_traits<Type>::domain_type> >
  , Type
>::type
unit_trail(const typename interval_traits<Type>::domain_type& value)
{
    return interval_traits<Type>::construct(value, domain_next<Type>(value));
}

template<class Type>
typename enable_if
<
    mpl::and_< is_static_left_open<Type>
             , boost::detail::is_incrementable<typename interval_traits<Type>::domain_type> >
  , Type
>::type
unit_trail(const typename interval_traits<Type>::domain_type& value)
{
    typedef typename interval_traits<Type>::domain_type    domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                 ::is_less_than(value) ));

    return interval_traits<Type>::construct(domain_prior<Type>(value), value);
}

template<class Type>
typename enable_if
<
    mpl::and_< is_static_open<Type>
             , is_discrete<typename interval_traits<Type>::domain_type> >
  , Type
>::type
unit_trail(const typename interval_traits<Type>::domain_type& value)
{
    typedef typename interval_traits<Type>::domain_type    domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                 ::is_less_than(value)));

    return interval_traits<Type>::construct( domain_prior<Type>(value)
                                           ,  domain_next<Type>(value));
}

template<class Type>
typename enable_if
<
    mpl::and_< is_static_closed<Type>
             , is_discrete<typename interval_traits<Type>::domain_type> >
  , Type
>::type
unit_trail(const typename interval_traits<Type>::domain_type& value)
{
    return interval_traits<Type>::construct(value, value);
}

//NOTE: statically bounded closed or open intervals of continuous domain types
// are NOT supported by ICL. They can not be used with interval containers
// consistently.


template<class Type>
typename enable_if<has_dynamic_bounds<Type>, Type>::type
unit_trail(const typename interval_traits<Type>::domain_type& value)
{
    return dynamic_interval_traits<Type>::construct(value, value, interval_bounds::closed());
}

} //namespace detail

//==============================================================================
//= Construct<Interval> multon
//==============================================================================
template<class Type>
typename enable_if<has_static_bounds<Type>, Type>::type
construct(const typename interval_traits<Type>::domain_type& low,
          const typename interval_traits<Type>::domain_type& up  )
{
    return interval_traits<Type>::construct(low, up);
}

template<class Type>
typename enable_if<has_dynamic_bounds<Type>, Type>::type
construct(const typename interval_traits<Type>::domain_type& low,
          const typename interval_traits<Type>::domain_type& up,
          interval_bounds bounds = interval_bounds::right_open())
{
    return dynamic_interval_traits<Type>::construct(low, up, bounds);
}


//- construct form bounded values ----------------------------------------------
template<class Type>
typename enable_if<has_dynamic_bounds<Type>, Type>::type
construct(const typename Type::bounded_domain_type& low,
          const typename Type::bounded_domain_type& up)
{
    return dynamic_interval_traits<Type>::construct_bounded(low, up);
}

template<class Type>
typename enable_if<is_interval<Type>, Type>::type
span(const typename interval_traits<Type>::domain_type& left,
     const typename interval_traits<Type>::domain_type& right)
{
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    if(domain_compare()(left,right))
        return construct<Type>(left, right);
    else
        return construct<Type>(right, left);
}


//==============================================================================
template<class Type>
typename enable_if<is_static_right_open<Type>, Type>::type
hull(const typename interval_traits<Type>::domain_type& left,
     const typename interval_traits<Type>::domain_type& right)
{
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    if(domain_compare()(left,right))
        return construct<Type>(left, domain_next<Type>(right));
    else
        return construct<Type>(right, domain_next<Type>(left));
}

template<class Type>
typename enable_if<is_static_left_open<Type>, Type>::type
hull(const typename interval_traits<Type>::domain_type& left,
     const typename interval_traits<Type>::domain_type& right)
{
    typedef typename interval_traits<Type>::domain_type    domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    if(domain_compare()(left,right))
    {
        BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                     ::is_less_than(left) ));
        return construct<Type>(domain_prior<Type>(left), right);
    }
    else
    {
        BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                     ::is_less_than(right) ));
        return construct<Type>(domain_prior<Type>(right), left);
    }
}

template<class Type>
typename enable_if<is_static_closed<Type>, Type>::type
hull(const typename interval_traits<Type>::domain_type& left,
     const typename interval_traits<Type>::domain_type& right)
{
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    if(domain_compare()(left,right))
        return construct<Type>(left, right);
    else
        return construct<Type>(right, left);
}

template<class Type>
typename enable_if<is_static_open<Type>, Type>::type
hull(const typename interval_traits<Type>::domain_type& left,
     const typename interval_traits<Type>::domain_type& right)
{
    typedef typename interval_traits<Type>::domain_type    domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    if(domain_compare()(left,right))
    {
        BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                     ::is_less_than(left) ));
        return construct<Type>( domain_prior<Type>(left)
                              ,  domain_next<Type>(right));
    }
    else
    {
        BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                     ::is_less_than(right) ));
        return construct<Type>( domain_prior<Type>(right)
                              ,  domain_next<Type>(left));
    }
}

template<class Type>
typename enable_if<has_dynamic_bounds<Type>, Type>::type
hull(const typename interval_traits<Type>::domain_type& left,
     const typename interval_traits<Type>::domain_type& right)
{
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    if(domain_compare()(left,right))
        return construct<Type>(left, right, interval_bounds::closed());
    else
        return construct<Type>(right, left, interval_bounds::closed());
}

//==============================================================================
//= Selection
//==============================================================================

template<class Type>
inline typename enable_if<is_interval<Type>,
                          typename interval_traits<Type>::domain_type>::type
lower(const Type& object)
{
    return interval_traits<Type>::lower(object);
}

template<class Type>
inline typename enable_if<is_interval<Type>,
                          typename interval_traits<Type>::domain_type>::type
upper(const Type& object)
{
    return interval_traits<Type>::upper(object);
}


//- first ----------------------------------------------------------------------
template<class Type>
inline typename
enable_if< mpl::or_<is_static_right_open<Type>, is_static_closed<Type> >
         , typename interval_traits<Type>::domain_type>::type
first(const Type& object)
{
    return lower(object);
}

template<class Type>
inline typename
enable_if< mpl::and_< mpl::or_<is_static_left_open<Type>, is_static_open<Type> >
                    , is_discrete<typename interval_traits<Type>::domain_type> >
         , typename interval_traits<Type>::domain_type>::type
first(const Type& object)
{
    return domain_next<Type>(lower(object));
}

template<class Type>
inline typename enable_if<is_discrete_interval<Type>,
                          typename interval_traits<Type>::domain_type>::type
first(const Type& object)
{
    return is_left_closed(object.bounds()) ?
                                 lower(object) :
               domain_next<Type>(lower(object));
}

//- last -----------------------------------------------------------------------
template<class Type>
inline typename
enable_if< mpl::or_<is_static_left_open<Type>, is_static_closed<Type> >
         , typename interval_traits<Type>::domain_type>::type
last(const Type& object)
{
    return upper(object);
}

template<class Type>
inline typename
enable_if< mpl::and_< mpl::or_<is_static_right_open<Type>, is_static_open<Type> >
                    , is_discrete<typename interval_traits<Type>::domain_type>  >
         , typename interval_traits<Type>::domain_type>::type
last(const Type& object)
{
    typedef typename interval_traits<Type>::domain_type    domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                 ::is_less_than(upper(object)) ));
    return domain_prior<Type>(upper(object));
}

template<class Type>
inline typename enable_if<is_discrete_interval<Type>,
                          typename interval_traits<Type>::domain_type>::type
last(const Type& object)
{
    typedef typename interval_traits<Type>::domain_type    domain_type;
    typedef typename interval_traits<Type>::domain_compare domain_compare;
    BOOST_ASSERT((numeric_minimum<domain_type, domain_compare, is_numeric<domain_type>::value>
                                 ::is_less_than_or(upper(object), is_right_closed(object.bounds())) ));
    return is_right_closed(object.bounds()) ?
                                  upper(object) :
               domain_prior<Type>(upper(object));
}

//- last_next ------------------------------------------------------------------
template<class Type>
inline typename
enable_if< mpl::and_< mpl::or_<is_static_left_open<Type>, is_static_closed<Type> >
                    , is_discrete<typename interval_traits<Type>::domain_type>  >
         , typename interval_traits<Type>::domain_type>::type
last_next(const Type& object)
{
    return domain_next<Type>(upper(object));
}

template<class Type>
inline typename
enable_if< mpl::and_< mpl::or_<is_static_right_open<Type>, is_static_open<Type> >
                    , is_discrete<typename interval_traits<Type>::domain_type>  >
         , typename interval_traits<Type>::domain_type>::type
last_next(const Type& object)
{
    //CL typedef typename interval_traits<Type>::domain_type domain_type;
    return upper(object); // NOTE: last_next is implemented to avoid calling pred(object)
}                         // For unsigned integral types this may cause underflow.

template<class Type>
inline typename enable_if<is_discrete_interval<Type>,
                          typename interval_traits<Type>::domain_type>::type
last_next(const Type& object)
{
    return is_right_closed(object.bounds()) ?
               domain_next<Type>(upper(object)):
                                 upper(object) ;
}

//------------------------------------------------------------------------------
template<class Type>
typename enable_if<has_dynamic_bounds<Type>,
                   typename Type::bounded_domain_type>::type
bounded_lower(const Type& object)
{
    return typename
        Type::bounded_domain_type(lower(object), object.bounds().left());
}

template<class Type>
typename enable_if<has_dynamic_bounds<Type>,
                   typename Type::bounded_domain_type>::type
reverse_bounded_lower(const Type& object)
{
    return typename
        Type::bounded_domain_type(lower(object),
                                  object.bounds().reverse_left());
}

template<class Type>
typename enable_if<has_dynamic_bounds<Type>,
                   typename Type::bounded_domain_type>::type
bounded_upper(const Type& object)
{
    return typename
        Type::bounded_domain_type(upper(object),
                                  object.bounds().right());
}

template<class Type>
typename enable_if<has_dynamic_bounds<Type>,
                   typename Type::bounded_domain_type>::type
reverse_bounded_upper(const Type& object)
{
    return typename
        Type::bounded_domain_type(upper(object),
                                  object.bounds().reverse_right());
}

//- bounds ---------------------------------------------------------------------
template<class Type>
inline typename enable_if<has_dynamic_bounds<Type>, interval_bounds>::type
bounds(const Type& object)
{
    return object.bounds();
}

template<class Type>
inline typename enable_if<has_static_bounds<Type>, interval_bounds>::type
bounds(const Type&)
{
    return interval_bounds(interval_bound_type<Type>::value);
}


//==============================================================================
//= Emptieness
//==============================================================================
/** Is the interval empty? */
template<class Type>
typename boost::enable_if<is_asymmetric_interval<Type>, bool>::type
is_empty(const Type& object)
{
    return domain_less_equal<Type>(upper(object), lower(object));
}

template<class Type>
typename boost::enable_if<is_static_closed<Type>, bool>::type
is_empty(const Type& object)
{
    return domain_less<Type>(upper(object), lower(object));
}

template<class Type>
typename boost::enable_if<is_static_open<Type>, bool>::type
is_empty(const Type& object)
{
    return domain_less_equal<Type>(upper(object),                   lower(object) )
        || domain_less_equal<Type>(upper(object), domain_next<Type>(lower(object)));
}

template<class Type>
typename boost::enable_if<is_discrete_interval<Type>, bool>::type
is_empty(const Type& object)
{
    if(object.bounds() == interval_bounds::closed())
        return domain_less<Type>(upper(object), lower(object));
    else if(object.bounds() == interval_bounds::open())
        return domain_less_equal<Type>(upper(object),                   lower(object) )
            || domain_less_equal<Type>(upper(object), domain_next<Type>(lower(object)));
    else
        return domain_less_equal<Type>(upper(object), lower(object));
}

template<class Type>
typename boost::enable_if<is_continuous_interval<Type>, bool>::type
is_empty(const Type& object)
{
    return     domain_less<Type>(upper(object), lower(object))
        || (   domain_equal<Type>(upper(object), lower(object))
            && object.bounds() != interval_bounds::closed()    );
}

//==============================================================================
//= Equivalences and Orderings
//==============================================================================
//- exclusive_less -------------------------------------------------------------
/** Maximal element of <tt>left</tt> is less than the minimal element of
    <tt>right</tt> */
template<class Type>
inline typename boost::enable_if<is_asymmetric_interval<Type>, bool>::type
exclusive_less(const Type& left, const Type& right)
{
    return icl::is_empty(left) || icl::is_empty(right)
        || domain_less_equal<Type>(upper(left), lower(right));
}

template<class Type>
inline typename boost::enable_if<is_discrete_interval<Type>, bool>::type
exclusive_less(const Type& left, const Type& right)
{
    return icl::is_empty(left) || icl::is_empty(right)
        || domain_less<Type>(last(left), first(right));
}

template<class Type>
inline typename boost::
enable_if<has_symmetric_bounds<Type>, bool>::type
exclusive_less(const Type& left, const Type& right)
{
    return icl::is_empty(left) || icl::is_empty(right)
        || domain_less<Type>(last(left), first(right));
}

template<class Type>
inline typename boost::enable_if<is_continuous_interval<Type>, bool>::type
exclusive_less(const Type& left, const Type& right)
{
    return     icl::is_empty(left) || icl::is_empty(right)
        ||     domain_less<Type>(upper(left), lower(right))
        || (   domain_equal<Type>(upper(left), lower(right))
            && inner_bounds(left,right) != interval_bounds::open() );
}


//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<has_static_bounds<Type>, bool>::type
lower_less(const Type& left, const Type& right)
{
    return domain_less<Type>(lower(left), lower(right));
}

template<class Type>
typename boost::enable_if<is_discrete_interval<Type>, bool>::type
lower_less(const Type& left, const Type& right)
{
    return domain_less<Type>(first(left), first(right));
}

template<class Type>
typename boost::enable_if<is_continuous_interval<Type>, bool>::type
lower_less(const Type& left, const Type& right)
{
    if(left_bounds(left,right) == interval_bounds::right_open())  //'[(' == 10
        return domain_less_equal<Type>(lower(left), lower(right));
    else
        return domain_less<Type>(lower(left), lower(right));
}


//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<has_static_bounds<Type>, bool>::type
upper_less(const Type& left, const Type& right)
{
    return domain_less<Type>(upper(left), upper(right));
}

template<class Type>
typename boost::enable_if<is_discrete_interval<Type>, bool>::type
upper_less(const Type& left, const Type& right)
{
    return domain_less<Type>(last(left), last(right));
}

template<class Type>
typename boost::enable_if<is_continuous_interval<Type>, bool>::type
upper_less(const Type& left, const Type& right)
{
    if(right_bounds(left,right) == interval_bounds::left_open())
        return domain_less_equal<Type>(upper(left), upper(right));
    else
        return domain_less<Type>(upper(left), upper(right));
}

//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>,
                          typename Type::bounded_domain_type   >::type
lower_min(const Type& left, const Type& right)
{
    return lower_less(left, right) ? bounded_lower(left) : bounded_lower(right);
}

//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>,
                          typename Type::bounded_domain_type   >::type
lower_max(const Type& left, const Type& right)
{
    return lower_less(left, right) ? bounded_lower(right) : bounded_lower(left);
}

//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>,
                          typename Type::bounded_domain_type   >::type
upper_max(const Type& left, const Type& right)
{
    return upper_less(left, right) ? bounded_upper(right) : bounded_upper(left);
}

//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>,
                          typename Type::bounded_domain_type   >::type
upper_min(const Type& left, const Type& right)
{
    return upper_less(left, right) ? bounded_upper(left) : bounded_upper(right);
}


//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<is_asymmetric_interval<Type>, bool>::type
lower_equal(const Type& left, const Type& right)
{
    return domain_equal<Type>(lower(left), lower(right));
}

template<class Type>
typename boost::enable_if<has_symmetric_bounds<Type>, bool>::type
lower_equal(const Type& left, const Type& right)
{
    return domain_equal<Type>(first(left), first(right));
}

template<class Type>
typename boost::enable_if<is_discrete_interval<Type>, bool>::type
lower_equal(const Type& left, const Type& right)
{
    return domain_equal<Type>(first(left), first(right));
}

template<class Type>
typename boost::enable_if<is_continuous_interval<Type>, bool>::type
lower_equal(const Type& left, const Type& right)
{
    return (left.bounds().left()==right.bounds().left())
        && domain_equal<Type>(lower(left), lower(right));
}


//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<is_asymmetric_interval<Type>, bool>::type
upper_equal(const Type& left, const Type& right)
{
    return domain_equal<Type>(upper(left), upper(right));
}

template<class Type>
typename boost::enable_if<has_symmetric_bounds<Type>, bool>::type
upper_equal(const Type& left, const Type& right)
{
    return domain_equal<Type>(last(left), last(right));
}

template<class Type>
typename boost::enable_if<is_discrete_interval<Type>, bool>::type
upper_equal(const Type& left, const Type& right)
{
    return domain_equal<Type>(last(left), last(right));
}

template<class Type>
typename boost::enable_if<is_continuous_interval<Type>, bool>::type
upper_equal(const Type& left, const Type& right)
{
    return (left.bounds().right()==right.bounds().right())
        && domain_equal<Type>(upper(left), upper(right));
}

//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<is_interval<Type>, bool>::type
lower_less_equal(const Type& left, const Type& right)
{
    return lower_less(left,right) || lower_equal(left,right);
}

template<class Type>
typename boost::enable_if<is_interval<Type>, bool>::type
upper_less_equal(const Type& left, const Type& right)
{
    return upper_less(left,right) || upper_equal(left,right);
}

//==============================================================================
//= Orderings, containedness (non empty)
//==============================================================================
namespace non_empty
{

    template<class Type>
    inline typename boost::enable_if<is_asymmetric_interval<Type>, bool>::type
    exclusive_less(const Type& left, const Type& right)
    {
        BOOST_ASSERT(!(icl::is_empty(left) || icl::is_empty(right)));
        return domain_less_equal<Type>(upper(left), lower(right));
    }

    template<class Type>
    inline typename boost::enable_if<is_discrete_interval<Type>, bool>::type
    exclusive_less(const Type& left, const Type& right)
    {
        BOOST_ASSERT(!(icl::is_empty(left) || icl::is_empty(right)));
        return domain_less<Type>(last(left), first(right));
    }

    template<class Type>
    inline typename boost::
    enable_if<has_symmetric_bounds<Type>, bool>::type
    exclusive_less(const Type& left, const Type& right)
    {
        BOOST_ASSERT(!(icl::is_empty(left) || icl::is_empty(right)));
        return domain_less<Type>(last(left), first(right));
    }

    template<class Type>
    inline typename boost::enable_if<is_continuous_interval<Type>, bool>::type
    exclusive_less(const Type& left, const Type& right)
    {
        BOOST_ASSERT(!(icl::is_empty(left) || icl::is_empty(right)));
        return     domain_less <Type>(upper(left), lower(right))
            || (   domain_equal<Type>(upper(left), lower(right))
                && inner_bounds(left,right) != interval_bounds::open() );
    }

    template<class Type>
    inline typename boost::enable_if<is_interval<Type>, bool>::type
    contains(const Type& super, const Type& sub)
    {
        return lower_less_equal(super,sub) && upper_less_equal(sub,super);
    }

} //namespace non_empty

//- contains -------------------------------------------------------------------
template<class Type>
inline typename boost::enable_if<is_interval<Type>, bool>::type
contains(const Type& super, const Type& sub)
{
    return icl::is_empty(sub) || non_empty::contains(super, sub);
}

template<class Type>
typename boost::enable_if<is_discrete_static<Type>, bool>::type
contains(const Type& super, const typename interval_traits<Type>::domain_type& element)
{
    return domain_less_equal<Type>(icl::first(super), element                  )
        && domain_less_equal<Type>(                   element, icl::last(super));
}

template<class Type>
typename boost::enable_if<is_continuous_left_open<Type>, bool>::type
contains(const Type& super, const typename interval_traits<Type>::domain_type& element)
{
    return domain_less      <Type>(icl::lower(super), element                   )
        && domain_less_equal<Type>(                   element, icl::upper(super));
}

template<class Type>
typename boost::enable_if<is_continuous_right_open<Type>, bool>::type
contains(const Type& super, const typename interval_traits<Type>::domain_type& element)
{
    return domain_less_equal<Type>(icl::lower(super), element                   )
        && domain_less      <Type>(                   element, icl::upper(super));
}

template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>, bool>::type
contains(const Type& super, const typename interval_traits<Type>::domain_type& element)
{
    return
        (is_left_closed(super.bounds())
            ? domain_less_equal<Type>(lower(super), element)
            :       domain_less<Type>(lower(super), element))
    &&
        (is_right_closed(super.bounds())
            ? domain_less_equal<Type>(element, upper(super))
            :       domain_less<Type>(element, upper(super)));
}

//- within ---------------------------------------------------------------------
template<class Type>
inline typename boost::enable_if<is_interval<Type>, bool>::type
within(const Type& sub, const Type& super)
{
    return contains(super,sub);
}

//- operator == ----------------------------------------------------------------
template<class Type>
typename boost::enable_if<is_interval<Type>, bool>::type
operator == (const Type& left, const Type& right)
{
    return (icl::is_empty(left) && icl::is_empty(right))
        || (lower_equal(left,right) && upper_equal(left,right));
}

template<class Type>
typename boost::enable_if<is_interval<Type>, bool>::type
operator != (const Type& left, const Type& right)
{
    return !(left == right);
}

//- operator < -----------------------------------------------------------------
template<class Type>
typename boost::enable_if<is_interval<Type>, bool>::type
operator < (const Type& left, const Type& right)
{
    if(icl::is_empty(left))
        return !icl::is_empty(right);
    else
        return lower_less(left,right)
            || (lower_equal(left,right) && upper_less(left,right));
}

template<class Type>
inline typename boost::enable_if<is_interval<Type>, bool>::type
operator > (const Type& left, const Type& right)
{
    return right < left;
}



//------------------------------------------------------------------------------
template<class Type>
typename boost::enable_if<is_asymmetric_interval<Type>, bool>::type
touches(const Type& left, const Type& right)
{
    return domain_equal<Type>(upper(left), lower(right));
}

template<class Type>
typename boost::enable_if<has_symmetric_bounds<Type>, bool>::type
touches(const Type& left, const Type& right)
{
    return domain_equal<Type>(last_next(left), first(right));
}

template<class Type>
typename boost::enable_if<is_discrete_interval<Type>, bool>::type
touches(const Type& left, const Type& right)
{
    return domain_equal<Type>(domain_next<Type>(last(left)), first(right));
}

template<class Type>
typename boost::enable_if<is_continuous_interval<Type>, bool>::type
touches(const Type& left, const Type& right)
{
    return is_complementary(inner_bounds(left,right))
        && domain_equal<Type>(upper(left), lower(right));
}


//==============================================================================
//= Size
//==============================================================================
//- cardinality ----------------------------------------------------------------

template<class Type>
typename boost::enable_if<is_continuous_interval<Type>,
    typename size_type_of<interval_traits<Type> >::type>::type
cardinality(const Type& object)
{
    typedef typename size_type_of<interval_traits<Type> >::type SizeT;
    if(icl::is_empty(object))
        return icl::identity_element<SizeT>::value();
    else if(   object.bounds() == interval_bounds::closed()
            && domain_equal<Type>(lower(object), upper(object)))
        return icl::unit_element<SizeT>::value();
    else
        return icl::infinity<SizeT>::value();
}

template<class Type>
typename boost::enable_if<is_discrete_interval<Type>,
    typename size_type_of<interval_traits<Type> >::type>::type
cardinality(const Type& object)
{
    typedef typename size_type_of<interval_traits<Type> >::type SizeT;
    return icl::is_empty(object) ? identity_element<SizeT>::value()
                                 : static_cast<SizeT>(last_next(object) - first(object));
}

template<class Type>
typename boost::enable_if<is_continuous_asymmetric<Type>,
    typename size_type_of<interval_traits<Type> >::type>::type
cardinality(const Type& object)
{
    typedef typename size_type_of<interval_traits<Type> >::type SizeT;
    if(icl::is_empty(object))
        return icl::identity_element<SizeT>::value();
    else
        return icl::infinity<SizeT>::value();
}

template<class Type>
typename boost::enable_if<is_discrete_asymmetric<Type>,
    typename size_type_of<interval_traits<Type> >::type>::type
cardinality(const Type& object)
{
    typedef typename size_type_of<interval_traits<Type> >::type SizeT;
    return icl::is_empty(object) ? identity_element<SizeT>::value()
                                 : static_cast<SizeT>(last_next(object) - first(object));
}

template<class Type>
typename boost::enable_if<has_symmetric_bounds<Type>,
    typename size_type_of<interval_traits<Type> >::type>::type
cardinality(const Type& object)
{
    typedef typename size_type_of<interval_traits<Type> >::type SizeT;
    return icl::is_empty(object) ? identity_element<SizeT>::value()
                                 : static_cast<SizeT>(last_next(object) - first(object));
}



//- size -----------------------------------------------------------------------
template<class Type>
inline typename enable_if<is_interval<Type>,
    typename size_type_of<interval_traits<Type> >::type>::type
size(const Type& object)
{
    return cardinality(object);
}

//- length ---------------------------------------------------------------------
template<class Type>
inline typename boost::enable_if<is_continuous_interval<Type>,
    typename difference_type_of<interval_traits<Type> >::type>::type
length(const Type& object)
{
    typedef typename difference_type_of<interval_traits<Type> >::type DiffT;
    return icl::is_empty(object) ? identity_element<DiffT>::value()
                                 : upper(object) - lower(object);
}

template<class Type>
inline typename boost::enable_if<is_discrete_interval<Type>,
    typename difference_type_of<interval_traits<Type> >::type>::type
length(const Type& object)
{
    typedef typename difference_type_of<interval_traits<Type> >::type DiffT;
    return icl::is_empty(object) ? identity_element<DiffT>::value()
                                 : last_next(object) - first(object);
}

template<class Type>
typename boost::enable_if<is_continuous_asymmetric<Type>,
    typename difference_type_of<interval_traits<Type> >::type>::type
length(const Type& object)
{
    typedef typename difference_type_of<interval_traits<Type> >::type DiffT;
    return icl::is_empty(object) ? identity_element<DiffT>::value()
                                 : upper(object) - lower(object);
}

template<class Type>
inline typename boost::enable_if<is_discrete_static<Type>,
    typename difference_type_of<interval_traits<Type> >::type>::type
length(const Type& object)
{
    typedef typename difference_type_of<interval_traits<Type> >::type DiffT;
    return icl::is_empty(object) ? identity_element<DiffT>::value()
                                 : last_next(object) - first(object);
}

//- iterative_size -------------------------------------------------------------
template<class Type>
inline typename enable_if<is_interval<Type>,
    typename size_type_of<interval_traits<Type> >::type>::type
iterative_size(const Type&)
{
    return 2;
}


//==============================================================================
//= Addition
//==============================================================================
//- hull -----------------------------------------------------------------------
/** \c hull returns the smallest interval containing \c left and \c right. */
template<class Type>
typename boost::enable_if<has_static_bounds<Type>, Type>::type
hull(Type left, const Type& right)
{
    typedef typename interval_traits<Type>::domain_compare domain_compare;

    if(icl::is_empty(right))
        return left;
    else if(icl::is_empty(left))
        return right;

    return
        construct<Type>
        (
            (std::min)(lower(left), lower(right), domain_compare()),
            (std::max)(upper(left), upper(right), domain_compare())
        );
}

template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>, Type>::type
hull(Type left, const Type& right)
{
    if(icl::is_empty(right))
        return left;
    else if(icl::is_empty(left))
        return right;

    return  dynamic_interval_traits<Type>::construct_bounded
            (
                lower_min(left, right),
                upper_max(left, right)
            );
}

//==============================================================================
//= Subtraction
//==============================================================================
//- left_subtract --------------------------------------------------------------
/** subtract \c left_minuend from the \c right interval on it's left side.
    Return the difference: The part of \c right right of \c left_minuend.
\code
right_over = right - left_minuend; //on the left.
...      d) : right
... c)      : left_minuend
     [c  d) : right_over
\endcode
*/
template<class Type>
typename boost::enable_if<is_asymmetric_interval<Type>, Type>::type
left_subtract(Type right, const Type& left_minuend)
{
    if(exclusive_less(left_minuend, right))
        return right;

    return construct<Type>(upper(left_minuend), upper(right));
}

template<class Type>
typename boost::enable_if<is_static_closed<Type>, Type>::type
left_subtract(Type right, const Type& left_minuend)
{
    if(exclusive_less(left_minuend, right))
        return right;
    else if(upper_less_equal(right, left_minuend))
        return identity_element<Type>::value();

    return construct<Type>(domain_next<Type>(upper(left_minuend)), upper(right));
}

template<class Type>
typename boost::enable_if<is_static_open<Type>, Type>::type
left_subtract(Type right, const Type& left_minuend)
{
    if(exclusive_less(left_minuend, right))
        return right;

    return construct<Type>(domain_prior<Type>(upper(left_minuend)), upper(right));
}

template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>, Type>::type
left_subtract(Type right, const Type& left_minuend)
{
    if(exclusive_less(left_minuend, right))
        return right;
    return  dynamic_interval_traits<Type>::construct_bounded
            ( reverse_bounded_upper(left_minuend), bounded_upper(right) );
}


//- right_subtract -------------------------------------------------------------
/** subtract \c right_minuend from the \c left interval on it's right side.
    Return the difference: The part of \c left left of \c right_minuend.
\code
left_over = left - right_minuend; //on the right side.
[a      ...  : left
     [b ...  : right_minuend
[a  b)       : left_over
\endcode
*/
template<class Type>
typename boost::enable_if<is_asymmetric_interval<Type>, Type>::type
right_subtract(Type left, const Type& right_minuend)
{
    if(exclusive_less(left, right_minuend))
        return left;
    return construct<Type>(lower(left), lower(right_minuend));
}

template<class Type>
typename boost::enable_if<is_static_closed<Type>, Type>::type
right_subtract(Type left, const Type& right_minuend)
{
    if(exclusive_less(left, right_minuend))
        return left;
    else if(lower_less_equal(right_minuend, left))
        return identity_element<Type>::value();

    return construct<Type>(lower(left), domain_prior<Type>(lower(right_minuend)));
}

template<class Type>
typename boost::enable_if<is_static_open<Type>, Type>::type
right_subtract(Type left, const Type& right_minuend)
{
    if(exclusive_less(left, right_minuend))
        return left;

    return construct<Type>(lower(left), domain_next<Type>(lower(right_minuend)));
}

template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>, Type>::type
right_subtract(Type left, const Type& right_minuend)
{
    if(exclusive_less(left, right_minuend))
        return left;

    return  dynamic_interval_traits<Type>::construct_bounded
            ( bounded_lower(left), reverse_bounded_lower(right_minuend) );
}

//==============================================================================
//= Intersection
//==============================================================================
//- operator & -----------------------------------------------------------------
/** Returns the intersection of \c left and \c right interval. */
template<class Type>
typename boost::enable_if<is_asymmetric_interval<Type>, Type>::type
operator & (Type left, const Type& right)
{
    typedef typename interval_traits<Type>::domain_compare domain_compare;

    if(icl::is_empty(left) || icl::is_empty(right))
        return identity_element<Type>::value();
    else
        return
        construct<Type>
        (
            (std::max)(icl::lower(left), icl::lower(right), domain_compare()),
            (std::min)(icl::upper(left), icl::upper(right), domain_compare())
        );
}

template<class Type>
typename boost::enable_if<has_symmetric_bounds<Type>, Type>::type
operator & (Type left, const Type& right)
{
    typedef typename interval_traits<Type>::domain_compare domain_compare;

    if(icl::is_empty(left) || icl::is_empty(right))
        return identity_element<Type>::value();
    else
        return
        construct<Type>
        (
            (std::max)(icl::lower(left), icl::lower(right), domain_compare()),
            (std::min)(icl::upper(left), icl::upper(right), domain_compare())
        );
}

template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>, Type>::type
operator & (Type left, const Type& right)
{
    if(icl::is_empty(left) || icl::is_empty(right))
        return identity_element<Type>::value();
    else
        return  dynamic_interval_traits<Type>::construct_bounded
                (
                    lower_max(left, right),
                    upper_min(left, right)
                );
}


//- intersects -----------------------------------------------------------------
template<class Type>
typename boost::enable_if<is_interval<Type>, bool>::type
intersects(const Type& left, const Type& right)
{
    return !(   icl::is_empty(left) || icl::is_empty(right)
             || exclusive_less(left,right) || exclusive_less(right,left));
}

//- disjoint -------------------------------------------------------------------
template<class Type>
typename boost::enable_if<is_interval<Type>, bool>::type
disjoint(const Type& left, const Type& right)
{
    return icl::is_empty(left) || icl::is_empty(right)
        || exclusive_less(left,right) || exclusive_less(right,left);
}

//==============================================================================
//= Complement
//==============================================================================

template<class Type>
typename boost::enable_if<is_asymmetric_interval<Type>, Type>::type
inner_complement(const Type& left, const Type& right)
{
    if(icl::is_empty(left) || icl::is_empty(right))
        return  identity_element<Type>::value();
    else if(exclusive_less(left, right))
        return construct<Type>(upper(left), lower(right));
    else if(exclusive_less(right, left))
        return construct<Type>(upper(right), lower(left));
    else
        return identity_element<Type>::value();
}

template<class Type>
typename boost::enable_if<is_discrete_static_closed<Type>, Type>::type
inner_complement(const Type& left, const Type& right)
{
    if(icl::is_empty(left) || icl::is_empty(right))
        return  identity_element<Type>::value();
    else if(exclusive_less(left, right))
        return construct<Type>(domain_next<Type>(upper(left)), domain_prior<Type>(lower(right)));
    else if(exclusive_less(right, left))
        return construct<Type>(domain_next<Type>(upper(right)), domain_prior<Type>(lower(left)));
    else
        return identity_element<Type>::value();
}

template<class Type>
typename boost::enable_if<is_discrete_static_open<Type>, Type>::type
inner_complement(const Type& left, const Type& right)
{
    if(icl::is_empty(left) || icl::is_empty(right))
        return  identity_element<Type>::value();
    else if(exclusive_less(left, right))
        return construct<Type>(last(left), first(right));
    else if(exclusive_less(right, left))
        return construct<Type>(last(right), first(left));
    else
        return identity_element<Type>::value();
}

template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>, Type>::type
inner_complement(const Type& left, const Type& right)
{
    if(icl::is_empty(left) || icl::is_empty(right))
        return  identity_element<Type>::value();
    else if(exclusive_less(left, right))
        return right_subtract(left_subtract(hull(left, right), left), right);
    else if(exclusive_less(right, left))
        return right_subtract(left_subtract(hull(right, left), right), left);
    else
        return identity_element<Type>::value();
}

template<class Type>
inline typename boost::enable_if<is_interval<Type>, Type>::type
between(const Type& left, const Type& right)
{
    return inner_complement(left, right);
}



//==============================================================================
//= Distance
//==============================================================================
template<class Type>
typename boost::
enable_if< mpl::and_< is_interval<Type>
                    , has_difference<typename interval_traits<Type>::domain_type>
                    , is_discrete<typename interval_traits<Type>::domain_type>
                    >
         , typename difference_type_of<interval_traits<Type> >::type>::type
distance(const Type& x1, const Type& x2)
{
    typedef typename difference_type_of<interval_traits<Type> >::type difference_type;

    if(icl::is_empty(x1) || icl::is_empty(x2))
        return icl::identity_element<difference_type>::value();
    else if(domain_less<Type>(last(x1), first(x2)))
        return static_cast<difference_type>(icl::pred(first(x2) - last(x1)));
    else if(domain_less<Type>(last(x2), first(x1)))
        return static_cast<difference_type>(icl::pred(first(x1) - last(x2)));
    else
        return icl::identity_element<difference_type>::value();
}

template<class Type>
typename boost::
enable_if< mpl::and_< is_interval<Type>
                    , has_difference<typename interval_traits<Type>::domain_type>
                    , is_continuous<typename interval_traits<Type>::domain_type>
                    >
         , typename difference_type_of<interval_traits<Type> >::type>::type
distance(const Type& x1, const Type& x2)
{
    typedef typename difference_type_of<interval_traits<Type> >::type DiffT;

    if(icl::is_empty(x1) || icl::is_empty(x2))
        return icl::identity_element<DiffT>::value();
    else if(domain_less<Type>(upper(x1), lower(x2)))
        return lower(x2) - upper(x1);
    else if(domain_less<Type>(upper(x2), lower(x1)))
        return lower(x1) - upper(x2);
    else
        return icl::identity_element<DiffT>::value();
}

//==============================================================================
//= Streaming, representation
//==============================================================================
template<class Type>
typename boost::
    enable_if< mpl::or_< is_static_left_open<Type>
                       , is_static_open<Type>    >, std::string>::type
left_bracket(const Type&) { return "("; }

template<class Type>
typename boost::
    enable_if< mpl::or_< is_static_right_open<Type>
                       , is_static_closed<Type>   >, std::string>::type
left_bracket(const Type&) { return "["; }

template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>, std::string>::type
left_bracket(const Type& object)
{
    return left_bracket(object.bounds());
}

//------------------------------------------------------------------------------
template<class Type>
typename boost::
    enable_if< mpl::or_< is_static_right_open<Type>
                       , is_static_open<Type>     >, std::string>::type
right_bracket(const Type&) { return ")"; }

template<class Type>
typename boost::
    enable_if< mpl::or_< is_static_left_open<Type>
                       , is_static_closed<Type>    >, std::string>::type
right_bracket(const Type&) { return "]"; }

template<class Type>
typename boost::enable_if<has_dynamic_bounds<Type>, std::string>::type
right_bracket(const Type& object)
{
    return right_bracket(object.bounds());
}

//------------------------------------------------------------------------------
template<class CharType, class CharTraits, class Type>
typename boost::enable_if<is_interval<Type>,
                          std::basic_ostream<CharType, CharTraits> >::type&
operator << (std::basic_ostream<CharType, CharTraits> &stream, Type const& object)
{
    if(boost::icl::is_empty(object))
        return stream << left_bracket<Type>(object) << right_bracket<Type>(object);
    else
        return stream << left_bracket<Type>(object)
                      << interval_traits<Type>::lower(object)
                      << ","
                      << interval_traits<Type>::upper(object)
                      << right_bracket<Type>(object) ;
}

}} // namespace icl boost

#endif
