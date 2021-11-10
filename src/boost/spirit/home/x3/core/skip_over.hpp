/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_SKIP_APRIL_16_2006_0625PM)
#define BOOST_SPIRIT_X3_SKIP_APRIL_16_2006_0625PM

#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/traits/attribute_category.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/not.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/declval.hpp>

namespace boost { namespace spirit { namespace x3
{
    ///////////////////////////////////////////////////////////////////////////
    // Move the /first/ iterator to the first non-matching position
    // given a skip-parser. The function is a no-op if unused_type or
    // unused_skipper is passed as the skip-parser.
    ///////////////////////////////////////////////////////////////////////////
    template <typename Skipper>
    struct unused_skipper : unused_type
    {
        unused_skipper(Skipper const& skipper)
          : skipper(skipper) {}
        Skipper const& skipper;
    };

    namespace detail
    {
        template <typename Skipper>
        struct is_unused_skipper
          : mpl::false_ {};

        template <typename Skipper>
        struct is_unused_skipper<unused_skipper<Skipper>>
          : mpl::true_ {};

        template <> 
        struct is_unused_skipper<unused_type>
          : mpl::true_ {};

        template <typename Skipper>
        inline Skipper const&
        get_unused_skipper(Skipper const& skipper)
        {
            return skipper;
        }
        template <typename Skipper>
        inline Skipper const&
        get_unused_skipper(unused_skipper<Skipper> const& unused_skipper)
        {
            return unused_skipper.skipper;
        }

        template <typename Iterator, typename Skipper>
        inline void skip_over(
            Iterator& first, Iterator const& last, Skipper const& skipper)
        {
            while (skipper.parse(first, last, unused, unused, unused))
                /***/;
        }

        template <typename Iterator>
        inline void skip_over(Iterator&, Iterator const&, unused_type)
        {
        }

        template <typename Iterator, typename Skipper>
        inline void skip_over(
            Iterator&, Iterator const&, unused_skipper<Skipper> const&)
        {
        }
    }

    // this tag is used to find the skipper from the context
    struct skipper_tag;
    
    template <typename Context>
    struct has_skipper
      : mpl::not_<detail::is_unused_skipper<
            typename remove_cv<typename remove_reference<
                decltype(x3::get<skipper_tag>(boost::declval<Context>()))
            >::type>::type
        >> {};

    template <typename Iterator, typename Context>
    inline void skip_over(
        Iterator& first, Iterator const& last, Context const& context)
    {
        detail::skip_over(first, last, x3::get<skipper_tag>(context));
    }
}}}

#endif
