//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_EXTRACT_FROM_SEP_30_2009_0732AM)
#define BOOST_SPIRIT_KARMA_EXTRACT_FROM_SEP_30_2009_0732AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/attributes_fwd.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/container.hpp>

#include <boost/ref.hpp>
#include <boost/optional.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  This file contains attribute extraction utilities. The utilities
    //  provided also accept spirit's unused_type; all no-ops. Compiler
    //  optimization will easily strip these away.
    ///////////////////////////////////////////////////////////////////////////

    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        // extract first and second element of a fusion sequence
        template <typename T>
        struct add_const_ref
          : add_reference<typename add_const<T>::type>
        {};

        template <typename T, int N>
        struct value_at_c
          : add_const_ref<typename fusion::result_of::value_at_c<T, N>::type>
        {};
    }

    // This is the default case: the plain attribute values
    template <typename Attribute, typename Exposed
      , bool IsOneElemSeq = traits::one_element_sequence<Attribute>::value>
    struct extract_from_attribute_base
    {
        typedef Attribute const& type;

        template <typename Context>
        static type call(Attribute const& attr, Context&)
        {
            return attr;
        }
    };

    // This handles the case where the attribute is a single element fusion
    // sequence. We silently extract the only element and treat it as the
    // attribute to generate output from.
    template <typename Attribute, typename Exposed>
    struct extract_from_attribute_base<Attribute, Exposed, true>
    {
        typedef typename remove_const<
            typename remove_reference<
                typename fusion::result_of::at_c<Attribute, 0>::type
            >::type
        >::type elem_type;

        typedef typename result_of::extract_from<Exposed, elem_type>::type type;

        template <typename Context>
        static type call(Attribute const& attr, Context& ctx)
        {
            return extract_from<Exposed>(fusion::at_c<0>(attr), ctx);
        }
    };

    template <typename Attribute, typename Exposed, typename Enable/*= void*/>
    struct extract_from_attribute
      : extract_from_attribute_base<Attribute, Exposed>
    {};

    // This handles optional attributes.
    template <typename Attribute, typename Exposed>
    struct extract_from_attribute<boost::optional<Attribute>, Exposed>
    {
        typedef Attribute const& type;

        template <typename Context>
        static type call(boost::optional<Attribute> const& attr, Context& ctx)
        {
            return extract_from<Exposed>(boost::get<Attribute>(attr), ctx);
        }
    };

    template <typename Attribute, typename Exposed>
    struct extract_from_attribute<boost::optional<Attribute const>, Exposed>
    {
        typedef Attribute const& type;

        template <typename Context>
        static type call(boost::optional<Attribute const> const& attr, Context& ctx)
        {
            return extract_from<Exposed>(boost::get<Attribute const>(attr), ctx);
        }
    };

    // This handles attributes wrapped inside a boost::ref().
    template <typename Attribute, typename Exposed>
    struct extract_from_attribute<reference_wrapper<Attribute>, Exposed>
    {
        typedef Attribute const& type;

        template <typename Context>
        static type call(reference_wrapper<Attribute> const& attr, Context& ctx)
        {
            return extract_from<Exposed>(attr.get(), ctx);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Attribute, typename Exposed, typename Enable>
    struct extract_from_container
    {
        typedef typename traits::container_value<Attribute const>::type
            value_type;
        typedef typename is_convertible<value_type, Exposed>::type
            is_convertible_to_value_type;

        typedef typename mpl::if_<
           mpl::or_<
                is_same<value_type, Exposed>, is_same<Attribute, Exposed> >
          , Exposed const&, Exposed
        >::type type;

        // handle case where container value type is convertible to result type
        // we simply return the front element of the container
        template <typename Context, typename Pred>
        static type call(Attribute const& attr, Context&, mpl::true_, Pred)
        {
            // return first element from container
            typedef typename traits::container_iterator<Attribute const>::type
                iterator_type;

            iterator_type it = traits::begin(attr);
            type result = *it;
            ++it;
            return result;
        }

        // handle strings
        template <typename Iterator>
        static void append_to_string(Exposed& result, Iterator begin, Iterator end)
        {
            for (Iterator i = begin; i != end; ++i)
                push_back(result, *i);
        }

        template <typename Context>
        static type call(Attribute const& attr, Context&, mpl::false_, mpl::true_)
        {
            typedef typename char_type_of<Attribute>::type char_type;

            Exposed result;
            append_to_string(result, traits::get_begin<char_type>(attr)
              , traits::get_end<char_type>(attr));
            return result;
        }

        // everything else gets just passed through
        template <typename Context>
        static type call(Attribute const& attr, Context&, mpl::false_, mpl::false_)
        {
            return type(attr);
        }

        template <typename Context>
        static type call(Attribute const& attr, Context& ctx)
        {
            typedef typename mpl::and_<
                traits::is_string<Exposed>, traits::is_string<Attribute>
            >::type handle_strings;

            // return first element from container
            return call(attr, ctx, is_convertible_to_value_type()
              , handle_strings());
        }
    };

    template <typename Attribute>
    struct extract_from_container<Attribute, Attribute>
    {
        typedef Attribute const& type;

        template <typename Context>
        static type call(Attribute const& attr, Context&)
        {
            return attr;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        // overload for non-container attributes
        template <typename Exposed, typename Attribute, typename Context>
        inline typename spirit::result_of::extract_from<Exposed, Attribute>::type
        extract_from(Attribute const& attr, Context& ctx, mpl::false_)
        {
            return extract_from_attribute<Attribute, Exposed>::call(attr, ctx);
        }

        // overload for containers (but not for variants or optionals
        // holding containers)
        template <typename Exposed, typename Attribute, typename Context>
        inline typename spirit::result_of::extract_from<Exposed, Attribute>::type
        extract_from(Attribute const& attr, Context& ctx, mpl::true_)
        {
            return extract_from_container<Attribute, Exposed>::call(attr, ctx);
        }
    }

    template <typename Exposed, typename Attribute, typename Context>
    inline typename spirit::result_of::extract_from<Exposed, Attribute>::type
    extract_from(Attribute const& attr, Context& ctx
#if (defined(__GNUC__) && (__GNUC__ < 4)) || \
    (defined(__APPLE__) && defined(__INTEL_COMPILER))
      , typename enable_if<traits::not_is_unused<Attribute> >::type*
#endif
    )
    {
        typedef typename mpl::and_<
            traits::is_container<Attribute>
          , traits::not_is_variant<Attribute>
          , traits::not_is_optional<Attribute>
        >::type is_not_wrapped_container;

        return detail::extract_from<Exposed>(attr, ctx
          , is_not_wrapped_container());
    }

    template <typename Exposed, typename Context>
    inline unused_type extract_from(unused_type, Context&)
    {
        return unused;
    }
}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace result_of
{
    template <typename Exposed, typename Attribute>
    struct extract_from
      : mpl::if_<
            mpl::and_<
                traits::is_container<Attribute>
              , traits::not_is_variant<Attribute>
              , traits::not_is_optional<Attribute> >
          , traits::extract_from_container<Attribute, Exposed>
          , traits::extract_from_attribute<Attribute, Exposed> >::type
    {};

    template <typename Exposed>
    struct extract_from<Exposed, unused_type>
    {
        typedef unused_type type;
    };

    template <typename Exposed>
    struct extract_from<Exposed, unused_type const>
    {
        typedef unused_type type;
    };
}}}

#endif
