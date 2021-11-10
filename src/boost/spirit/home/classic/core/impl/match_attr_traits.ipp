/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_MATCH_ATTR_TRAITS_IPP)
#define BOOST_SPIRIT_MATCH_ATTR_TRAITS_IPP

#include <boost/optional.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace spirit { 

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

namespace impl
{
    template <typename T>
    struct match_attr_traits
    {
        typedef typename
            boost::optional<T>::reference_const_type
        const_reference;

        //  case where src *IS* convertible to T (dest)
        template <typename T2>
        static void
        convert(boost::optional<T>& dest, T2 const& src, mpl::true_)
        { 
            dest.reset(src); 
        }

        //  case where src *IS NOT* convertible to T (dest)
        template <typename T2>
        static void
        convert(boost::optional<T>& dest, T2 const& /*src*/, mpl::false_)
        { 
            dest.reset(); 
        }

        static void
        convert(boost::optional<T>& dest, nil_t/*src*/)
        { 
            dest.reset(); 
        }
        
        template <typename T2>
        static void
        convert(boost::optional<T>& dest, T2 const& src)
        { 
            convert(dest, src, is_convertible<T2, T>());
        }

        template <typename OtherMatchT>
        static void
        copy(boost::optional<T>& dest, OtherMatchT const& src)
        {
            if (src.has_valid_attribute())
                convert(dest, src.value());
        }

        template <typename OtherMatchT>
        static void
        assign(boost::optional<T>& dest, OtherMatchT const& src)
        {
            if (src.has_valid_attribute())
                convert(dest, src.value());
            else
                dest.reset();
        }

        // T is not reference
        template <typename ValueT>
        static void
        set_value(boost::optional<T>& dest, ValueT const& val, mpl::false_)
        {
            dest.reset(val);
        }

        // T is a reference
        template <typename ValueT>
        static void
        set_value(boost::optional<T>& dest, ValueT const& val, mpl::true_)
        {
            dest.get() = val;
        }
    };

}

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit::impl

#endif

