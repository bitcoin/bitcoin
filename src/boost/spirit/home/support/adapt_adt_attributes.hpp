//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ADAPT_ADT_ATTRIBUTES_SEP_15_2010_1219PM)
#define BOOST_SPIRIT_ADAPT_ADT_ATTRIBUTES_SEP_15_2010_1219PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/attributes.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/numeric_traits.hpp>
#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/utility/enable_if.hpp>

///////////////////////////////////////////////////////////////////////////////
// customization points allowing to use adapted classes with spirit
namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, bool Const, typename Domain>
    struct not_is_variant<
            fusion::extension::adt_attribute_proxy<T, N, Const>, Domain>
      : not_is_variant<
            typename fusion::extension::adt_attribute_proxy<T, N, Const>::type
          , Domain>
    {};

    template <typename T, int N, bool Const, typename Domain>
    struct not_is_optional<
            fusion::extension::adt_attribute_proxy<T, N, Const>, Domain>
      : not_is_optional<
            typename fusion::extension::adt_attribute_proxy<T, N, Const>::type
          , Domain>
    {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, bool Const>
    struct is_container<fusion::extension::adt_attribute_proxy<T, N, Const> >
      : is_container<
            typename fusion::extension::adt_attribute_proxy<T, N, Const>::type
        >
    {};

    template <typename T, int N, bool Const>
    struct container_value<fusion::extension::adt_attribute_proxy<T, N, Const> >
      : container_value<
            typename remove_reference<
                typename fusion::extension::adt_attribute_proxy<
                    T, N, Const
                >::type
            >::type
        >
    {};

    template <typename T, int N, bool Const>
    struct container_value<
            fusion::extension::adt_attribute_proxy<T, N, Const> const>
      : container_value<
            typename add_const<
                typename remove_reference<
                    typename fusion::extension::adt_attribute_proxy<
                        T, N, Const
                    >::type
                >::type 
            >::type 
        >
    {};

    template <typename T, int N, typename Val>
    struct push_back_container<
        fusion::extension::adt_attribute_proxy<T, N, false>
      , Val
      , typename enable_if<is_reference<
            typename fusion::extension::adt_attribute_proxy<T, N, false>::type
        > >::type>
    {
        static bool call(
            fusion::extension::adt_attribute_proxy<T, N, false>& p
          , Val const& val)
        {
            typedef typename 
                fusion::extension::adt_attribute_proxy<T, N, false>::type
            type;
            return push_back(type(p), val);
        }
    };

    template <typename T, int N, bool Const>
    struct container_iterator<
            fusion::extension::adt_attribute_proxy<T, N, Const> >
      : container_iterator<
            typename remove_reference<
                typename fusion::extension::adt_attribute_proxy<
                    T, N, Const
                >::type
            >::type
        >
    {};

    template <typename T, int N, bool Const>
    struct container_iterator<
            fusion::extension::adt_attribute_proxy<T, N, Const> const>
      : container_iterator<
            typename add_const<
                typename remove_reference<
                    typename fusion::extension::adt_attribute_proxy<
                        T, N, Const
                    >::type
                >::type 
            >::type 
        >
    {};

    template <typename T, int N, bool Const>
    struct begin_container<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        typedef typename remove_reference<
            typename fusion::extension::adt_attribute_proxy<T, N, Const>::type
        >::type container_type;

        static typename container_iterator<container_type>::type 
        call(fusion::extension::adt_attribute_proxy<T, N, Const>& c)
        {
            return c.get().begin();
        }
    };

    template <typename T, int N, bool Const>
    struct begin_container<fusion::extension::adt_attribute_proxy<T, N, Const> const>
    {
        typedef typename add_const<
            typename remove_reference<
                typename fusion::extension::adt_attribute_proxy<T, N, Const>::type 
            >::type
        >::type container_type;

        static typename container_iterator<container_type>::type 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& c)
        {
            return c.get().begin();
        }
    };

    template <typename T, int N, bool Const>
    struct end_container<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        typedef typename remove_reference<
            typename fusion::extension::adt_attribute_proxy<T, N, Const>::type
        >::type container_type;

        static typename container_iterator<container_type>::type 
        call(fusion::extension::adt_attribute_proxy<T, N, Const>& c)
        {
            return c.get().end();
        }
    };

    template <typename T, int N, bool Const>
    struct end_container<fusion::extension::adt_attribute_proxy<T, N, Const> const>
    {
        typedef typename add_const<
            typename remove_reference<
                typename fusion::extension::adt_attribute_proxy<T, N, Const>::type 
            >::type
        >::type container_type;

        static typename container_iterator<container_type>::type 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& c)
        {
            return c.get().end();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, typename Val>
    struct assign_to_attribute_from_value<
        fusion::extension::adt_attribute_proxy<T, N, false>
      , Val>
    {
        static void 
        call(Val const& val
          , fusion::extension::adt_attribute_proxy<T, N, false>& attr)
        {
            attr = val;
        }
    };

    template <typename T, int N, bool Const, typename Exposed>
    struct extract_from_attribute<
        fusion::extension::adt_attribute_proxy<T, N, Const>, Exposed>
    {
        typedef
            typename fusion::extension::adt_attribute_proxy<T, N, Const>::type
        get_return_type;
        typedef typename remove_const<
            typename remove_reference<
                get_return_type
            >::type
        >::type embedded_type;
        typedef 
            typename spirit::result_of::extract_from<Exposed, embedded_type>::type
        extracted_type;

        // If adt_attribute_proxy returned a value we must pass the attribute
        // by value, otherwise we will end up with a reference to a temporary
        // that will expire out of scope of the function call.
        typedef typename mpl::if_c<is_reference<get_return_type>::value
          , extracted_type
          , typename remove_reference<extracted_type>::type
        >::type type;

        template <typename Context>
        static type 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& val, Context& ctx)
        {
            return extract_from<Exposed>(val.get(), ctx);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, bool Const>
    struct attribute_type<fusion::extension::adt_attribute_proxy<T, N, Const> >
      : fusion::extension::adt_attribute_proxy<T, N, Const>
    {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, bool Const>
    struct optional_attribute<
        fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        typedef typename result_of::optional_value<
            typename remove_reference<
                typename fusion::extension::adt_attribute_proxy<T, N, Const>::type 
            >::type
        >::type type;

        static type 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& val)
        {
            return optional_value(val.get());
        }

        static bool 
        is_valid(fusion::extension::adt_attribute_proxy<T, N, Const> const& val)
        {
            return has_optional_value(val.get());
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, int N, typename Attribute, typename Domain>
    struct transform_attribute<
        fusion::extension::adt_attribute_proxy<T, N, false>
      , Attribute
      , Domain
      , typename disable_if<is_reference<
            typename fusion::extension::adt_attribute_proxy<T, N, false>::type
        > >::type>
    {
        typedef Attribute type;

        static Attribute 
        pre(fusion::extension::adt_attribute_proxy<T, N, false>& val)
        { 
            return val; 
        }
        static void 
        post(
            fusion::extension::adt_attribute_proxy<T, N, false>& val
          , Attribute const& attr) 
        {
            val = attr;
        }
        static void 
        fail(fusion::extension::adt_attribute_proxy<T, N, false>&)
        {
        }
    };

    template <
        typename T, int N, bool Const, typename Attribute, typename Domain>
    struct transform_attribute<
        fusion::extension::adt_attribute_proxy<T, N, Const>
      , Attribute
      , Domain
      , typename enable_if<is_reference<
            typename fusion::extension::adt_attribute_proxy<
                T, N, Const
            >::type
        > >::type>
    {
        typedef Attribute& type;

        static Attribute& 
        pre(fusion::extension::adt_attribute_proxy<T, N, Const>& val)
        { 
            return val; 
        }
        static void 
        post(
            fusion::extension::adt_attribute_proxy<T, N, Const>&
          , Attribute const&)
        {
        }
        static void 
        fail(fusion::extension::adt_attribute_proxy<T, N, Const>&)
        {
        }
    };

    template <typename T, int N, bool Const>
    struct clear_value<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        static void call(
            fusion::extension::adt_attribute_proxy<T, N, Const>& val)
        {
            typedef typename 
                fusion::extension::adt_attribute_proxy<T, N, Const>::type
            type;
            clear(type(val));
        }
    };

    template <typename T, int N, bool Const>
    struct attribute_size<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        typedef typename remove_const<
            typename remove_reference<
                typename fusion::extension::adt_attribute_proxy<T, N, Const>::type 
            >::type
        >::type embedded_type;

        typedef typename attribute_size<embedded_type>::type type;

        static type 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& val)
        {
            return attribute_size<embedded_type>::call(val.get());
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // customization point specializations for numeric generators
    template <typename T, int N, bool Const>
    struct absolute_value<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        typedef typename 
            fusion::extension::adt_attribute_proxy<T, N, Const>::type
        type;

        static type 
        call (fusion::extension::adt_attribute_proxy<T, N, Const> const& val)
        {
            return get_absolute_value(val.get());
        }
    };

    template <typename T, int N, bool Const>
    struct is_negative<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        static bool 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& val) 
        { 
            return test_negative(val.get()); 
        }
    };

    template <typename T, int N, bool Const>
    struct is_zero<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        static bool 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& val) 
        { 
            return test_zero(val.get()); 
        }
    };

    template <typename T, int N, bool Const>
    struct is_nan<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        static bool 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& val) 
        { 
            return test_nan(val.get()); 
        }
    };

    template <typename T, int N, bool Const>
    struct is_infinite<fusion::extension::adt_attribute_proxy<T, N, Const> >
    {
        static bool 
        call(fusion::extension::adt_attribute_proxy<T, N, Const> const& val) 
        { 
            return test_infinite(val.get()); 
        }
    };
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace result_of
{
    template <typename T, int N, bool Const>
    struct optional_value<fusion::extension::adt_attribute_proxy<T, N, Const> >
      : result_of::optional_value<
            typename remove_const<
                typename remove_reference<
                    typename fusion::extension::adt_attribute_proxy<T, N, Const>::type 
                >::type
            >::type>
    {};
}}}

#endif
