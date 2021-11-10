///////////////////////////////////////////////////////////////////////////////
// cons.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_UTILITY_CONS_HPP_EAN_11_19_2005
#define BOOST_XPRESSIVE_DETAIL_UTILITY_CONS_HPP_EAN_11_19_2005

#include <boost/version.hpp>

#if BOOST_VERSION >= 103300

// In Boost 1.33+, we have a cons list in Fusion, so just include it.

# if BOOST_VERSION >= 103500
#  include <boost/fusion/include/cons.hpp> // Boost 1.35+ has Fusion2
# else
#  include <boost/spirit/fusion/sequence/cons.hpp> // Fusion1
# endif

#else

// For earlier versions of Boost, put the definition of cons here
# include <boost/call_traits.hpp>
# include <boost/mpl/if.hpp>
# include <boost/mpl/eval_if.hpp>
# include <boost/mpl/identity.hpp>
# include <boost/type_traits/is_const.hpp>
# include <boost/type_traits/add_const.hpp>
# include <boost/type_traits/add_reference.hpp>
# include <boost/spirit/fusion/detail/config.hpp>
# include <boost/spirit/fusion/detail/access.hpp>
# include <boost/spirit/fusion/iterator/next.hpp>
# include <boost/spirit/fusion/iterator/equal_to.hpp>
# include <boost/spirit/fusion/iterator/as_fusion_iterator.hpp>
# include <boost/spirit/fusion/iterator/detail/iterator_base.hpp>
# include <boost/spirit/fusion/sequence/begin.hpp>
# include <boost/spirit/fusion/sequence/end.hpp>
# include <boost/spirit/fusion/sequence/as_fusion_sequence.hpp>
# include <boost/spirit/fusion/sequence/detail/sequence_base.hpp>

namespace boost { namespace fusion
{
    struct nil;

    struct cons_tag;

    template <typename Car, typename Cdr>
    struct cons;

    struct cons_iterator_tag;

    template <typename Cons>
    struct cons_iterator;

    namespace cons_detail
    {
        template <typename Iterator>
        struct deref_traits_impl
        {
            typedef typename Iterator::cons_type cons_type;
            typedef typename cons_type::car_type value_type;

            typedef typename mpl::eval_if<
                is_const<cons_type>
              , add_reference<typename add_const<value_type>::type>
              , add_reference<value_type> >::type
            type;

            static type
            call(Iterator const& i)
            {
                return detail::ref(i.cons.car);
            }
        };

        template <typename Iterator>
        struct next_traits_impl
        {
            typedef typename Iterator::cons_type cons_type;
            typedef typename cons_type::cdr_type cdr_type;

            typedef cons_iterator<
                typename mpl::eval_if<
                    is_const<cons_type>
                  , add_const<cdr_type>
                  , mpl::identity<cdr_type>
                >::type>
            type;

            static type
            call(Iterator const& i)
            {
                return type(detail::ref(i.cons.cdr));
            }
        };

        template <typename Iterator>
        struct value_traits_impl
        {
            typedef typename Iterator::cons_type cons_type;
            typedef typename cons_type::car_type type;
        };

        template <typename Cons>
        struct begin_traits_impl
        {
            typedef cons_iterator<Cons> type;

            static type
            call(Cons& t)
            {
                return type(t);
            }
        };

        template <typename Cons>
        struct end_traits_impl
        {
            typedef cons_iterator<
                typename mpl::if_<is_const<Cons>, nil const, nil>::type>
            type;

            static type
            call(Cons& t)
            {
                FUSION_RETURN_DEFAULT_CONSTRUCTED;
            }
        };
    } // namespace cons_detail

    namespace meta
    {
        template <typename Tag>
        struct deref_impl;

        template <>
        struct deref_impl<cons_iterator_tag>
        {
            template <typename Iterator>
            struct apply : cons_detail::deref_traits_impl<Iterator> {};
        };

        template <typename Tag>
        struct next_impl;

        template <>
        struct next_impl<cons_iterator_tag>
        {
            template <typename Iterator>
            struct apply : cons_detail::next_traits_impl<Iterator> {};
        };

        template <typename Tag>
        struct value_impl;

        template <>
        struct value_impl<cons_iterator_tag>
        {
            template <typename Iterator>
            struct apply : cons_detail::value_traits_impl<Iterator> {};
        };

        template <typename Tag>
        struct begin_impl;

        template <>
        struct begin_impl<cons_tag>
        {
            template <typename Sequence>
            struct apply : cons_detail::begin_traits_impl<Sequence>
            {};
        };

        template <typename Tag>
        struct end_impl;

        template <>
        struct end_impl<cons_tag>
        {
            template <typename Sequence>
            struct apply : cons_detail::end_traits_impl<Sequence>
            {};
        };
    } // namespace meta

    template <typename Cons = nil>
    struct cons_iterator : iterator_base<cons_iterator<Cons> >
    {
        typedef cons_iterator_tag tag;
        typedef Cons cons_type;

        explicit cons_iterator(cons_type& cons_)
            : cons(cons_) {}

        cons_type& cons;
    };

    template <>
    struct cons_iterator<nil> : iterator_base<cons_iterator<nil> >
    {
        typedef cons_iterator_tag tag;
        typedef nil cons_type;
        cons_iterator() {}
        explicit cons_iterator(nil const&) {}
    };

    template <>
    struct cons_iterator<nil const> : iterator_base<cons_iterator<nil const> >
    {
        typedef cons_iterator_tag tag;
        typedef nil const cons_type;
        cons_iterator() {}
        explicit cons_iterator(nil const&) {}
    };

    struct nil : sequence_base<nil>
    {
        typedef cons_tag tag;
        typedef void_t car_type;
        typedef void_t cdr_type;
    };

    template <typename Car, typename Cdr = nil>
    struct cons : sequence_base<cons<Car,Cdr> >
    {
        typedef cons_tag tag;
        typedef typename call_traits<Car>::value_type car_type;
        typedef Cdr cdr_type;

        cons()
          : car(), cdr() {}

        explicit cons(
            typename call_traits<Car>::param_type car_
          , typename call_traits<Cdr>::param_type cdr_ = Cdr())
          : car(car_), cdr(cdr_) {}

        car_type car;
        cdr_type cdr;
    };

    template <typename Car>
    inline cons<Car>
    make_cons(Car const& car)
    {
        return cons<Car>(car);
    }

    template <typename Car, typename Cdr>
    inline cons<Car, Cdr>
    make_cons(Car const& car, Cdr const& cdr)
    {
        return cons<Car, Cdr>(car, cdr);
    }
}} // namespace boost::fusion

namespace boost { namespace mpl
{
    template <typename Tag>
    struct begin_impl;

    template <typename Tag>
    struct end_impl;

    template <>
    struct begin_impl<fusion::cons_tag>
      : fusion::meta::begin_impl<fusion::cons_tag>
    {
    };

    template <>
    struct end_impl<fusion::cons_tag>
      : fusion::meta::end_impl<fusion::cons_tag>
    {
    };

}} // namespace boost::mpl

#endif

// Before Boost v1.33.1, Fusion cons lists were not valid MPL sequences.
#if BOOST_VERSION < 103301
namespace boost { namespace mpl
{
    template<typename Iterator>
    struct next;

    template<typename Cons>
    struct next<fusion::cons_iterator<Cons> >
      : fusion::cons_detail::next_traits_impl<fusion::cons_iterator<Cons> >
    {
    };

    template<typename Iterator>
    struct deref;

    template<typename Cons>
    struct deref<fusion::cons_iterator<Cons> >
      : fusion::cons_detail::value_traits_impl<fusion::cons_iterator<Cons> >
    {
    };

}} // namespace boost::mpl
#endif

#endif
