/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_BOOST_TUPLE_ITERATOR_09262006_1851)
#define FUSION_BOOST_TUPLE_ITERATOR_09262006_1851

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/iterator/iterator_facade.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/tuple/tuple.hpp>

namespace boost { namespace fusion
{
    struct forward_traversal_tag;

    namespace detail
    {
        template <typename T>
        struct boost_tuple_is_empty : mpl::false_ {};

        template <>
        struct boost_tuple_is_empty<tuples::null_type> : mpl::true_ {};

        template <>
        struct boost_tuple_is_empty<tuples::null_type const> : mpl::true_ {};

        template <>
        struct boost_tuple_is_empty<tuples::tuple<> > : mpl::true_ {};

        template <>
        struct boost_tuple_is_empty<tuples::tuple<> const> : mpl::true_ {};
    }

    template <typename Cons>
    struct boost_tuple_iterator_identity;

    template <typename Cons = tuples::null_type>
    struct boost_tuple_iterator
        : iterator_facade<boost_tuple_iterator<Cons>, forward_traversal_tag>
    {
        typedef Cons cons_type;

        typedef boost_tuple_iterator_identity<
            typename add_const<Cons>::type> identity;

        BOOST_FUSION_GPU_ENABLED
        explicit boost_tuple_iterator(Cons& in_cons)
            : cons(in_cons) {}
        Cons& cons;

        template <typename Iterator>
        struct value_of : mpl::identity<typename Iterator::cons_type::head_type> {};

        template <typename Iterator>
        struct deref
        {
            typedef typename value_of<Iterator>::type element;

            typedef typename
                mpl::if_<
                    is_const<typename Iterator::cons_type>
                  , typename tuples::access_traits<element>::const_type
                  , typename tuples::access_traits<element>::non_const_type
                >::type
            type;

            BOOST_FUSION_GPU_ENABLED
            static type
            call(Iterator const& iter)
            {
                return iter.cons.get_head();
            }
        };

        template <typename Iterator>
        struct next
        {
            typedef typename Iterator::cons_type cons_type;
            typedef typename cons_type::tail_type tail_type;

            typedef boost_tuple_iterator<
                typename mpl::eval_if<
                    is_const<cons_type>
                  , add_const<tail_type>
                  , mpl::identity<tail_type>
                >::type>
            type;

            BOOST_FUSION_GPU_ENABLED
            static type
            call(Iterator const& iter)
            {
                return type(iter.cons.get_tail());
            }
        };

        template <typename I1, typename I2>
        struct distance;

        // detail
        template <typename I1, typename I2>
        struct lazy_next_distance
        {
            typedef
                typename mpl::plus<
                    mpl::int_<1>,
                    typename distance<
                        typename next<I1>::type,
                        I2
                    >::type
                >::type type;
        };

        template <typename I1, typename I2>
        struct distance
        {
            typedef typename mpl::eval_if<
                boost::is_same<I1, I2>,
                mpl::int_<0>,
                lazy_next_distance<I1, I2>
            >::type type;

            BOOST_FUSION_GPU_ENABLED
            static type
            call(I1 const&, I2 const&)
            {
                return type();
            }
        };

        template <typename I1, typename I2>
        struct equal_to
            : is_same<typename I1::identity, typename I2::identity>
        {};

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(boost_tuple_iterator& operator= (boost_tuple_iterator const&))
    };

    template <typename Null>
    struct boost_tuple_null_iterator
        : iterator_facade<boost_tuple_iterator<Null>, forward_traversal_tag>
    {
        typedef Null cons_type;

        typedef boost_tuple_iterator_identity<
            typename add_const<Null>::type> identity;

        template <typename I1, typename I2>
        struct equal_to
            : mpl::or_<
                is_same<I1, I2>
              , mpl::and_<
                    detail::boost_tuple_is_empty<typename I1::cons_type>
                  , detail::boost_tuple_is_empty<typename I2::cons_type>
                >
            >
        {};
    };

    template <>
    struct boost_tuple_iterator<tuples::null_type>
        : boost_tuple_null_iterator<tuples::null_type>
    {
        template <typename Cons>
        BOOST_FUSION_GPU_ENABLED
        explicit boost_tuple_iterator(Cons const&) {}
    };

    template <>
    struct boost_tuple_iterator<tuples::null_type const>
        : boost_tuple_null_iterator<tuples::null_type const>
    {
        template <typename Cons>
        BOOST_FUSION_GPU_ENABLED
        explicit boost_tuple_iterator(Cons const&) {}
    };

    template <>
    struct boost_tuple_iterator<tuples::tuple<> >
        : boost_tuple_null_iterator<tuples::tuple<> >
    {
        template <typename Cons>
        BOOST_FUSION_GPU_ENABLED
        explicit boost_tuple_iterator(Cons const&) {}
    };

    template <>
    struct boost_tuple_iterator<tuples::tuple<> const>
        : boost_tuple_null_iterator<tuples::tuple<> const>
    {
        template <typename Cons>
        BOOST_FUSION_GPU_ENABLED
        explicit boost_tuple_iterator(Cons const&) {}
    };
}}

#ifdef BOOST_FUSION_WORKAROUND_FOR_LWG_2408
namespace std
{
    template <typename Cons>
    struct iterator_traits< ::boost::fusion::boost_tuple_iterator<Cons> >
    { };
}
#endif

#endif


