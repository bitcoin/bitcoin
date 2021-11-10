
// Copyright Aleksey Gurtovoy 2001-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// *Preprocessed* version of the main "template_arity.hpp" header
// -- DO NOT modify by hand!

namespace boost { namespace mpl { namespace aux {
template< int N > struct arity_tag
{
    typedef char (&type)[N + 1];
};

template<
      int C1, int C2, int C3, int C4, int C5, int C6
    >
struct max_arity
{
    BOOST_STATIC_CONSTANT(int, value =
          ( C6 > 0 ? C6 : ( C5 > 0 ? C5 : ( C4 > 0 ? C4 : ( C3 > 0 ? C3 : ( C2 > 0 ? C2 : ( C1 > 0 ? C1 : -1 ) ) ) ) ) )
        );
};

arity_tag<0>::type arity_helper(...);

template<
      template< typename P1 > class F
    , typename T1
    >
typename arity_tag<1>::type
arity_helper(type_wrapper< F<T1> >, arity_tag<1>);

template<
      template< typename P1, typename P2 > class F
    , typename T1, typename T2
    >
typename arity_tag<2>::type
arity_helper(type_wrapper< F< T1,T2 > >, arity_tag<2>);

template<
      template< typename P1, typename P2, typename P3 > class F
    , typename T1, typename T2, typename T3
    >
typename arity_tag<3>::type
arity_helper(type_wrapper< F< T1,T2,T3 > >, arity_tag<3>);

template<
      template< typename P1, typename P2, typename P3, typename P4 > class F
    , typename T1, typename T2, typename T3, typename T4
    >
typename arity_tag<4>::type
arity_helper(type_wrapper< F< T1,T2,T3,T4 > >, arity_tag<4>);

template<
      template<
          typename P1, typename P2, typename P3, typename P4
        , typename P5
        >
      class F
    , typename T1, typename T2, typename T3, typename T4, typename T5
    >
typename arity_tag<5>::type
arity_helper(type_wrapper< F< T1,T2,T3,T4,T5 > >, arity_tag<5>);

template<
      template<
          typename P1, typename P2, typename P3, typename P4
        , typename P5, typename P6
        >
      class F
    , typename T1, typename T2, typename T3, typename T4, typename T5
    , typename T6
    >
typename arity_tag<6>::type
arity_helper(type_wrapper< F< T1,T2,T3,T4,T5,T6 > >, arity_tag<6>);
template< typename F, int N >
struct template_arity_impl
{
    BOOST_STATIC_CONSTANT(int, value =
          sizeof(::boost::mpl::aux::arity_helper(type_wrapper<F>(), arity_tag<N>())) - 1
        );
};

template< typename F >
struct template_arity
{
    BOOST_STATIC_CONSTANT(int, value  = (
          max_arity< template_arity_impl< F,1 >::value, template_arity_impl< F,2 >::value, template_arity_impl< F,3 >::value, template_arity_impl< F,4 >::value, template_arity_impl< F,5 >::value, template_arity_impl< F,6 >::value >::value
        ));
    typedef mpl::int_<value> type;
};

}}}
