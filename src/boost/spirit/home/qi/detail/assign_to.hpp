/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ASSIGN_TO_APR_16_2006_0812PM)
#define BOOST_SPIRIT_ASSIGN_TO_APR_16_2006_0812PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/detail/construct.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/fusion/include/copy.hpp>
#include <boost/fusion/adapted/struct/detail/extension.hpp>
#include <boost/ref.hpp>
#include <boost/range/range_fwd.hpp>

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    //  This file contains assignment utilities. The utilities provided also
    //  accept spirit's unused_type; all no-ops. Compiler optimization will
    //  easily strip these away.
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T>
        struct is_iter_range : mpl::false_ {};

        template <typename I>
        struct is_iter_range<boost::iterator_range<I> > : mpl::true_ {};

        template <typename C>
        struct is_container_of_ranges
          : is_iter_range<typename C::value_type> {};
    }

    template <typename Attribute, typename Iterator, typename Enable>
    struct assign_to_attribute_from_iterators
    {
        // Common case
        static void
        call(Iterator const& first, Iterator const& last, Attribute& attr, mpl::false_)
        {
            if (traits::is_empty(attr))
                attr = Attribute(first, last);
            else {
                for (Iterator i = first; i != last; ++i)
                    push_back(attr, *i);
            }
        }

        // If Attribute is a container with value_type==iterator_range<T> just push the
        // iterator_range into it
        static void
        call(Iterator const& first, Iterator const& last, Attribute& attr, mpl::true_)
        {
            typename Attribute::value_type rng(first, last);
            push_back(attr, rng);
        }

        static void
        call(Iterator const& first, Iterator const& last, Attribute& attr)
        {
            call(first, last, attr, detail::is_container_of_ranges<Attribute>());
        }
    };

    template <typename Attribute, typename Iterator>
    struct assign_to_attribute_from_iterators<
        reference_wrapper<Attribute>, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last
          , reference_wrapper<Attribute> attr)
        {
            if (traits::is_empty(attr))
                attr = Attribute(first, last);
            else {
                for (Iterator i = first; i != last; ++i)
                    push_back(attr, *i);
            }
        }
    };

    template <typename Attribute, typename Iterator>
    struct assign_to_attribute_from_iterators<
        boost::optional<Attribute>, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last
          , boost::optional<Attribute>& attr)
        {
            Attribute val;
            assign_to(first, last, val);
            attr = val;
        }
    };

    template <typename Iterator>
    struct assign_to_attribute_from_iterators<
        iterator_range<Iterator>, Iterator>
    {
        static void
        call(Iterator const& first, Iterator const& last
          , iterator_range<Iterator>& attr)
        {
            attr = iterator_range<Iterator>(first, last);
        }
    };

    template <typename Iterator, typename Attribute>
    inline void
    assign_to(Iterator const& first, Iterator const& last, Attribute& attr)
    {
        assign_to_attribute_from_iterators<Attribute, Iterator>::
            call(first, last, attr);
    }

    template <typename Iterator>
    inline void
    assign_to(Iterator const&, Iterator const&, unused_type)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Attribute>
    void assign_to(T const& val, Attribute& attr);

    template <typename Attribute, typename T, typename Enable>
    struct assign_to_attribute_from_value
    {
        typedef typename traits::one_element_sequence<Attribute>::type
            is_one_element_sequence;

        typedef typename mpl::eval_if<
            is_one_element_sequence
          , fusion::result_of::at_c<Attribute, 0>
          , mpl::identity<Attribute&>
        >::type type;

        template <typename T_>
        static void
        call(T_ const& val, Attribute& attr, mpl::false_)
        {
            attr = static_cast<Attribute>(val);
        }

        // This handles the case where the attribute is a single element fusion
        // sequence. We silently assign to the only element and treat it as the
        // attribute to parse the results into.
        template <typename T_>
        static void
        call(T_ const& val, Attribute& attr, mpl::true_)
        {
            typedef typename fusion::result_of::value_at_c<Attribute, 0>::type
                element_type;
            fusion::at_c<0>(attr) = static_cast<element_type>(val);
        }

        static void
        call(T const& val, Attribute& attr)
        {
            call(val, attr, is_one_element_sequence());
        }
    };

    template <typename Attribute>
    struct assign_to_attribute_from_value<Attribute, Attribute>
    {
        static void
        call(Attribute const& val, Attribute& attr)
        {
            attr = val;
        }
    };

    template <typename Attribute, typename T>
    struct assign_to_attribute_from_value<Attribute, reference_wrapper<T>
      , typename disable_if<is_same<Attribute, reference_wrapper<T> > >::type>
    {
        static void
        call(reference_wrapper<T> const& val, Attribute& attr)
        {
            assign_to(val.get(), attr);
        }
    };

    template <typename Attribute, typename T>
    struct assign_to_attribute_from_value<Attribute, boost::optional<T>
      , typename disable_if<is_same<Attribute, boost::optional<T> > >::type>
    {
        static void
        call(boost::optional<T> const& val, Attribute& attr)
        {
            assign_to(val.get(), attr);
        }
    };

    template <typename Attribute, int N, bool Const, typename T>
    struct assign_to_attribute_from_value<fusion::extension::adt_attribute_proxy<Attribute, N, Const>, T>
    {
        static void
        call(T const& val, typename fusion::extension::adt_attribute_proxy<Attribute, N, Const>& attr)
        {
            attr = val;
        }
    };

    namespace detail
    {
        template <typename A, typename B>
        struct is_same_size_sequence
          : mpl::bool_<fusion::result_of::size<A>::value
                == fusion::result_of::size<B>::value>
        {};
    }

    template <typename Attribute, typename T>
    struct assign_to_attribute_from_value<Attribute, T,
            mpl::and_<
                fusion::traits::is_sequence<Attribute>,
                fusion::traits::is_sequence<T>,
                detail::is_same_size_sequence<Attribute, T>
            >
        >
    {
        static void
        call(T const& val, Attribute& attr)
        {
            fusion::copy(val, attr);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Attribute, typename T, typename Enable>
    struct assign_to_container_from_value
    {
        // T is not a container and not a string
        template <typename T_>
        static void call(T_ const& val, Attribute& attr, mpl::false_, mpl::false_)
        {
            traits::push_back(attr, val);
        }

        // T is a container (but not a string), and T is convertible to the 
        // value_type of the Attribute container
        template <typename T_>
        static void 
        append_to_container_not_string(T_ const& val, Attribute& attr, mpl::true_)
        {
            traits::push_back(attr, val);
        }

        // T is a container (but not a string), generic overload
        template <typename T_>
        static void 
        append_to_container_not_string(T_ const& val, Attribute& attr, mpl::false_)
        {
            typedef typename traits::container_iterator<T_ const>::type
                iterator_type;

            iterator_type end = traits::end(val);
            for (iterator_type i = traits::begin(val); i != end; traits::next(i))
                traits::push_back(attr, traits::deref(i));
        }

        // T is a container (but not a string)
        template <typename T_>
        static void call(T_ const& val, Attribute& attr,  mpl::true_, mpl::false_)
        {
            typedef typename container_value<Attribute>::type value_type;
            typedef typename is_convertible<T, value_type>::type is_value_type;

            append_to_container_not_string(val, attr, is_value_type());
        }

        ///////////////////////////////////////////////////////////////////////
        // T is a string
        template <typename Iterator>
        static void append_to_string(Attribute& attr, Iterator begin, Iterator end)
        {
            for (Iterator i = begin; i != end; ++i)
                traits::push_back(attr, *i);
        }

        // T is string, but not convertible to value_type of container
        template <typename T_>
        static void append_to_container(T_ const& val, Attribute& attr, mpl::false_)
        {
            typedef typename char_type_of<T_>::type char_type;

            append_to_string(attr, traits::get_begin<char_type>(val)
              , traits::get_end<char_type>(val));
        }

        // T is string, and convertible to value_type of container
        template <typename T_>
        static void append_to_container(T_ const& val, Attribute& attr, mpl::true_)
        {
            traits::push_back(attr, val);
        }

        template <typename T_, typename Pred>
        static void call(T_ const& val, Attribute& attr, Pred, mpl::true_)
        {
            typedef typename container_value<Attribute>::type value_type;
            typedef typename is_convertible<T, value_type>::type is_value_type;

            append_to_container(val, attr, is_value_type());
        }

        ///////////////////////////////////////////////////////////////////////
        static void call(T const& val, Attribute& attr)
        {
            typedef typename traits::is_container<T>::type is_container;
            typedef typename traits::is_string<T>::type is_string;

            call(val, attr, is_container(), is_string());
        }
    };

    template <typename Attribute>
    struct assign_to_container_from_value<Attribute, Attribute>
    {
        static void
        call(Attribute const& val, Attribute& attr)
        {
            attr = val;
        }
    };

    template <typename Attribute, typename T>
    struct assign_to_container_from_value<Attribute, boost::optional<T>
      , typename disable_if<is_same<Attribute, boost::optional<T> > >::type>
    {
        static void
        call(boost::optional<T> const& val, Attribute& attr)
        {
            assign_to(val.get(), attr);
        }
    };

    template <typename Attribute, typename T>
    struct assign_to_container_from_value<Attribute, reference_wrapper<T>
      , typename disable_if<is_same<Attribute, reference_wrapper<T> > >::type>
    {
        static void
        call(reference_wrapper<T> const& val, Attribute& attr)
        {
            assign_to(val.get(), attr);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        // overload for non-container attributes
        template <typename T, typename Attribute>
        inline void
        assign_to(T const& val, Attribute& attr, mpl::false_)
        {
            assign_to_attribute_from_value<Attribute, T>::call(val, attr);
        }

        // overload for containers (but not for variants or optionals
        // holding containers)
        template <typename T, typename Attribute>
        inline void
        assign_to(T const& val, Attribute& attr, mpl::true_)
        {
            assign_to_container_from_value<Attribute, T>::call(val, attr);
        }
    }

    template <typename T, typename Attribute>
    inline void
    assign_to(T const& val, Attribute& attr)
    {
        typedef typename mpl::and_<
            traits::is_container<Attribute>
          , traits::not_is_variant<Attribute>
          , traits::not_is_optional<Attribute>
        >::type is_not_wrapped_container;

        detail::assign_to(val, attr, is_not_wrapped_container());
    }

    template <typename T>
    inline void
    assign_to(T const&, unused_type)
    {
    }
}}}

#endif
