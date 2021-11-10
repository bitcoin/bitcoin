/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2002 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_SPIRIT_CLASSIC_PHOENIX_TUPLE_HELPERS_HPP
#define BOOST_SPIRIT_CLASSIC_PHOENIX_TUPLE_HELPERS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <cassert>
#include <boost/spirit/home/classic/phoenix/tuples.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace phoenix
{

///////////////////////////////////////////////////////////////////////////////
//
//  make_tuple template class
//
//      This template class is used to calculate a tuple type required to hold
//      the given template parameter type
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  normal (non-tuple types are wrapped into a tuple)
template <typename ResultT>
struct make_tuple {

    typedef tuple<ResultT> type;
};

///////////////////////////////////////////////////////////////////////////////
//  nil_t is converted to an empty tuple type
template <>
struct make_tuple<nil_t> {

    typedef tuple<> type;
};

///////////////////////////////////////////////////////////////////////////////
//  tuple types are left alone without any refactoring
template <
      typename A, typename B, typename C
#if PHOENIX_LIMIT > 3
    , typename D, typename E, typename F
#if PHOENIX_LIMIT > 6
    , typename G, typename H, typename I
#if PHOENIX_LIMIT > 9
    , typename J, typename K, typename L
#if PHOENIX_LIMIT > 12
    , typename M, typename N, typename O
#endif
#endif
#endif
#endif
>
struct make_tuple<tuple<A, B, C
#if PHOENIX_LIMIT > 3
    , D, E, F
#if PHOENIX_LIMIT > 6
    , G, H, I
#if PHOENIX_LIMIT > 9
    , J, K, L
#if PHOENIX_LIMIT > 12
    , M, N, O
#endif
#endif
#endif
#endif
    > > {

// the tuple parameter itself is the required tuple type
    typedef tuple<A, B, C
#if PHOENIX_LIMIT > 3
        , D, E, F
#if PHOENIX_LIMIT > 6
        , G, H, I
#if PHOENIX_LIMIT > 9
        , J, K, L
#if PHOENIX_LIMIT > 12
        , M, N, O
#endif
#endif
#endif
#endif
        > type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat_tuple type computer
//
//      This class returns the type of a tuple, which is constructed by
//      concatenating a tuple with a given type
//
///////////////////////////////////////////////////////////////////////////////
template <typename TupleT, typename AppendT>
struct concat_tuple;

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <0 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename AppendT>
struct concat_tuple<tuple<>, AppendT> {

    typedef tuple<AppendT> type;
};

template <>
struct concat_tuple<tuple<>, nil_t> {

    typedef tuple<> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <1 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename AppendT>
struct concat_tuple<tuple<A>, AppendT> {

    typedef tuple<A, AppendT> type;
};

template <typename A>
struct concat_tuple<tuple<A>, nil_t> {

    typedef tuple<A> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <2 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B, typename AppendT>
struct concat_tuple<tuple<A, B>, AppendT> {

    typedef tuple<A, B, AppendT> type;
};

template <typename A, typename B>
struct concat_tuple<tuple<A, B>, nil_t> {

    typedef tuple<A, B> type;
};

#if PHOENIX_LIMIT > 3
///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <3 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C>, AppendT> {

    typedef tuple<A, B, C, AppendT> type;
};

template <
    typename A, typename B, typename C
>
struct concat_tuple<tuple<A, B, C>, nil_t> {

    typedef tuple<A, B, C> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <4 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D>, AppendT> {

    typedef tuple<A, B, C, D, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D
>
struct concat_tuple<tuple<A, B, C, D>, nil_t> {

    typedef tuple<A, B, C, D> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <5 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E>, AppendT> {

    typedef tuple<A, B, C, D, E, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E
>
struct concat_tuple<tuple<A, B, C, D, E>, nil_t> {

    typedef tuple<A, B, C, D, E> type;
};

#if PHOENIX_LIMIT > 6
///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <6 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F>, AppendT> {

    typedef tuple<A, B, C, D, E, F, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F
>
struct concat_tuple<tuple<A, B, C, D, E, F>, nil_t> {

    typedef tuple<A, B, C, D, E, F> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <7 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F, G>, AppendT> {

    typedef tuple<A, B, C, D, E, F, G, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G
>
struct concat_tuple<tuple<A, B, C, D, E, F, G>, nil_t> {

    typedef tuple<A, B, C, D, E, F, G> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <8 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H>, AppendT> {

    typedef tuple<A, B, C, D, E, F, G, H, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H>, nil_t> {

    typedef tuple<A, B, C, D, E, F, G, H> type;
};

#if PHOENIX_LIMIT > 9
///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <9 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I>, AppendT> {

    typedef tuple<A, B, C, D, E, F, G, H, I, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I>, nil_t> {

    typedef tuple<A, B, C, D, E, F, G, H, I> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <10 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J>, AppendT> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J>, nil_t> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <11 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J, typename K,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J, K>, AppendT> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, K, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J, typename K
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J, K>, nil_t> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, K> type;
};

#if PHOENIX_LIMIT > 12
///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <12 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J, typename K, typename L,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J, K, L>, AppendT> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, K, L, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J, typename K, typename L
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J, K, L>, nil_t> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, K, L> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <13 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J, typename K, typename L,
    typename M,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J, K, L, M>, AppendT> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J, typename K, typename L,
    typename M
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J, K, L, M>, nil_t> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, K, L, M> type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  concat tuple <14 member> class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J, typename K, typename L,
    typename M, typename N,
    typename AppendT
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, N>, AppendT> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, N, AppendT> type;
};

template <
    typename A, typename B, typename C, typename D, typename E, typename F,
    typename G, typename H, typename I, typename J, typename K, typename L,
    typename M, typename N
>
struct concat_tuple<tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, N>, nil_t> {

    typedef tuple<A, B, C, D, E, F, G, H, I, J, K, L, M, N> type;
};

#endif
#endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  concat_tuples type computer
//
//      This template class returns the type of a tuple built from the
//      concatenation of two given tuples.
//
///////////////////////////////////////////////////////////////////////////////
template <typename TupleT1, typename TupleT2, int N, typename AppendT>
struct concat_tuple_element {

    typedef
        typename concat_tuple_element<
                typename concat_tuple<TupleT1, AppendT>::type, TupleT2, N+1,
                typename tuple_element<N+1, TupleT2>::type
            >::type
        type;
};

template <typename TupleT1, typename TupleT2, int N>
struct concat_tuple_element<TupleT1, TupleT2, N, nil_t> {

    typedef TupleT1 type;
};

template <typename TupleT1, typename TupleT2>
struct concat_tuples {

    typedef
        typename concat_tuple_element<
                TupleT1, TupleT2, 0,
                typename tuple_element<0, TupleT2>::type
            >::type
        type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  convert_actors template function
//
//      The convert_actors template functions constructs a new tuple object
//      composed of the elements returned by the actors contained in the
//      input tuple. (i.e. the given tuple type 'actor_tuple' contains a set
//      of actors to evaluate and the resulting tuple contains the results of
//      evaluating the actors.)
//
///////////////////////////////////////////////////////////////////////////////
template <typename ActorT, typename TupleT>
struct actor_result; // forward declaration

namespace impl
{
    template <unsigned N>
    struct convert_actors_ {};
}

template <typename TupleResultT, typename ActorTupleT>
TupleResultT
convert_actors(ActorTupleT const& actor_tuple)
{
    BOOST_STATIC_ASSERT(ActorTupleT::length <= TupleResultT::length);
    BOOST_STATIC_CONSTANT(int, length = TupleResultT::length);
    return impl::convert_actors_<length>
        ::template apply<TupleResultT, ActorTupleT>::do_(actor_tuple);
}

namespace impl
{
    template <int N, typename TupleResultT, typename ActorTupleT>
    struct convert_actor
    {
        typedef typename tuple_element<N, TupleResultT>::type type;

        template <bool C>
        struct is_default_t {};
        typedef is_default_t<true>  is_default;
        typedef is_default_t<false> is_not_default;

        static type
        actor_element(ActorTupleT const& /*actor_tuple*/, is_default)
        {
            return type(); // default construct
        }

        static type
        actor_element(ActorTupleT const& actor_tuple, is_not_default)
        {
            BOOST_STATIC_ASSERT(ActorTupleT::length <= TupleResultT::length);
            tuple_index<N> const idx;
            return actor_tuple[idx](); // apply the actor
        }

        static type
        do_(ActorTupleT const& actor_tuple)
        {
            return actor_element(
                actor_tuple, is_default_t<(N >= ActorTupleT::length)>());
        }
    };

    ///////////////////////////////////////
    template <>
    struct convert_actors_<1>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;

                return TupleResultT(
                    converter0::do_(actor_tuple)
                );
            }
        };
    };

    ///////////////////////////////////////
    template <>
    struct convert_actors_<2>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                );
            }
        };
    };

    ///////////////////////////////////////
    template <>
    struct convert_actors_<3>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                );
            }
        };
    };

    #if PHOENIX_LIMIT > 3

    /////////////////////////////////////
    template <>
    struct convert_actors_<4>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                );
            }
        };
    };

    /////////////////////////////////////
    template <>
    struct convert_actors_<5>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                );
            }
        };
    };

    /////////////////////////////////////
    template <>
    struct convert_actors_<6>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                );
            }
        };
    };

    #if PHOENIX_LIMIT > 6

    /////////////////////////////////////
    template <>
    struct convert_actors_<7>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                );
            }
        };
    };

    /////////////////////////////////////
    template <>
    struct convert_actors_<8>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;
                typedef impl::convert_actor<7, TupleResultT, ActorTupleT> converter7;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                    ,   converter7::do_(actor_tuple)
                );
            }
        };
    };

    /////////////////////////////////////
    template <>
    struct convert_actors_<9>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;
                typedef impl::convert_actor<7, TupleResultT, ActorTupleT> converter7;
                typedef impl::convert_actor<8, TupleResultT, ActorTupleT> converter8;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                    ,   converter7::do_(actor_tuple)
                    ,   converter8::do_(actor_tuple)
                );
            }
        };
    };

    #if PHOENIX_LIMIT > 9

    /////////////////////////////////////
    template <>
    struct convert_actors_<10>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;
                typedef impl::convert_actor<7, TupleResultT, ActorTupleT> converter7;
                typedef impl::convert_actor<8, TupleResultT, ActorTupleT> converter8;
                typedef impl::convert_actor<9, TupleResultT, ActorTupleT> converter9;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                    ,   converter7::do_(actor_tuple)
                    ,   converter8::do_(actor_tuple)
                    ,   converter9::do_(actor_tuple)
                );
            }
        };
    };

    /////////////////////////////////////
    template <>
    struct convert_actors_<11>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;
                typedef impl::convert_actor<7, TupleResultT, ActorTupleT> converter7;
                typedef impl::convert_actor<8, TupleResultT, ActorTupleT> converter8;
                typedef impl::convert_actor<9, TupleResultT, ActorTupleT> converter9;
                typedef impl::convert_actor<10, TupleResultT, ActorTupleT> converter10;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                    ,   converter7::do_(actor_tuple)
                    ,   converter8::do_(actor_tuple)
                    ,   converter9::do_(actor_tuple)
                    ,   converter10::do_(actor_tuple)
                );
            }
        };
    };

    /////////////////////////////////////
    template <>
    struct convert_actors_<12>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;
                typedef impl::convert_actor<7, TupleResultT, ActorTupleT> converter7;
                typedef impl::convert_actor<8, TupleResultT, ActorTupleT> converter8;
                typedef impl::convert_actor<9, TupleResultT, ActorTupleT> converter9;
                typedef impl::convert_actor<10, TupleResultT, ActorTupleT> converter10;
                typedef impl::convert_actor<11, TupleResultT, ActorTupleT> converter11;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                    ,   converter7::do_(actor_tuple)
                    ,   converter8::do_(actor_tuple)
                    ,   converter9::do_(actor_tuple)
                    ,   converter10::do_(actor_tuple)
                    ,   converter11::do_(actor_tuple)
                );
            }
        };
    };

    #if PHOENIX_LIMIT > 12

    /////////////////////////////////////
    template <>
    struct convert_actors_<13>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;
                typedef impl::convert_actor<7, TupleResultT, ActorTupleT> converter7;
                typedef impl::convert_actor<8, TupleResultT, ActorTupleT> converter8;
                typedef impl::convert_actor<9, TupleResultT, ActorTupleT> converter9;
                typedef impl::convert_actor<10, TupleResultT, ActorTupleT> converter10;
                typedef impl::convert_actor<11, TupleResultT, ActorTupleT> converter11;
                typedef impl::convert_actor<12, TupleResultT, ActorTupleT> converter12;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                    ,   converter7::do_(actor_tuple)
                    ,   converter8::do_(actor_tuple)
                    ,   converter9::do_(actor_tuple)
                    ,   converter10::do_(actor_tuple)
                    ,   converter11::do_(actor_tuple)
                    ,   converter12::do_(actor_tuple)
                );
            }
        };
    };

    ///////////////////////////////////////
    template <>
    struct convert_actors_<14>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;
                typedef impl::convert_actor<7, TupleResultT, ActorTupleT> converter7;
                typedef impl::convert_actor<8, TupleResultT, ActorTupleT> converter8;
                typedef impl::convert_actor<9, TupleResultT, ActorTupleT> converter9;
                typedef impl::convert_actor<10, TupleResultT, ActorTupleT> converter10;
                typedef impl::convert_actor<11, TupleResultT, ActorTupleT> converter11;
                typedef impl::convert_actor<12, TupleResultT, ActorTupleT> converter12;
                typedef impl::convert_actor<13, TupleResultT, ActorTupleT> converter13;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                    ,   converter7::do_(actor_tuple)
                    ,   converter8::do_(actor_tuple)
                    ,   converter9::do_(actor_tuple)
                    ,   converter10::do_(actor_tuple)
                    ,   converter11::do_(actor_tuple)
                    ,   converter12::do_(actor_tuple)
                    ,   converter13::do_(actor_tuple)
                );
            }
        };
    };

    ///////////////////////////////////////
    template <>
    struct convert_actors_<15>
    {
        template <typename TupleResultT, typename ActorTupleT>
        struct apply
        {
            static TupleResultT
            do_(ActorTupleT const& actor_tuple)
            {
                typedef impl::convert_actor<0, TupleResultT, ActorTupleT> converter0;
                typedef impl::convert_actor<1, TupleResultT, ActorTupleT> converter1;
                typedef impl::convert_actor<2, TupleResultT, ActorTupleT> converter2;
                typedef impl::convert_actor<3, TupleResultT, ActorTupleT> converter3;
                typedef impl::convert_actor<4, TupleResultT, ActorTupleT> converter4;
                typedef impl::convert_actor<5, TupleResultT, ActorTupleT> converter5;
                typedef impl::convert_actor<6, TupleResultT, ActorTupleT> converter6;
                typedef impl::convert_actor<7, TupleResultT, ActorTupleT> converter7;
                typedef impl::convert_actor<8, TupleResultT, ActorTupleT> converter8;
                typedef impl::convert_actor<9, TupleResultT, ActorTupleT> converter9;
                typedef impl::convert_actor<10, TupleResultT, ActorTupleT> converter10;
                typedef impl::convert_actor<11, TupleResultT, ActorTupleT> converter11;
                typedef impl::convert_actor<12, TupleResultT, ActorTupleT> converter12;
                typedef impl::convert_actor<13, TupleResultT, ActorTupleT> converter13;
                typedef impl::convert_actor<14, TupleResultT, ActorTupleT> converter14;

                using namespace tuple_index_names;
                return TupleResultT(
                        converter0::do_(actor_tuple)
                    ,   converter1::do_(actor_tuple)
                    ,   converter2::do_(actor_tuple)
                    ,   converter3::do_(actor_tuple)
                    ,   converter4::do_(actor_tuple)
                    ,   converter5::do_(actor_tuple)
                    ,   converter6::do_(actor_tuple)
                    ,   converter7::do_(actor_tuple)
                    ,   converter8::do_(actor_tuple)
                    ,   converter9::do_(actor_tuple)
                    ,   converter10::do_(actor_tuple)
                    ,   converter11::do_(actor_tuple)
                    ,   converter12::do_(actor_tuple)
                    ,   converter13::do_(actor_tuple)
                    ,   converter14::do_(actor_tuple)
                );
            }
        };
    };

    #endif
    #endif
    #endif
    #endif
}   //  namespace impl


///////////////////////////////////////////////////////////////////////////////
}   //  namespace phoenix

#endif
