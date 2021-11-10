/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2013 Agustin Berge
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_SUBCONTEXT_APR_15_2013_0840AM)
#define BOOST_SPIRIT_X3_SUBCONTEXT_APR_15_2013_0840AM

#include <boost/spirit/home/x3/support/context.hpp>
#include <boost/spirit/home/x3/support/unused.hpp>

namespace boost { namespace spirit { namespace x3
{
    template <typename... T>
    struct subcontext;

    template <>
    struct subcontext<>
    {
        template <typename Context>
        subcontext(Context const& /*context*/)
        {}
        
        template <typename ID_>
        unused_type
        get(ID_) const
        {
            return unused;
        }
    };

    template <typename T>
    struct subcontext<T>
      : context<typename T::first_type, typename T::second_type>
    {
        typedef context<
            typename T::first_type, typename T::second_type
        > context_type;

        template <typename Context>
        subcontext(Context const& context)
          : context_type(x3::get<typename T::first_type>(context))
        {}

        using context_type::get;
    };

    template <typename T, typename... Tail>
    struct subcontext<T, Tail...>
      : subcontext<Tail...>
      , context<
            typename T::first_type, typename T::second_type
          , subcontext<Tail...>
        >
    {
        typedef subcontext<Tail...> base_type;
        typedef context<
            typename T::first_type, typename T::second_type
          , base_type
        > context_type;

        template <typename Context>
        subcontext(Context const& context)
          : base_type(context)
          , context_type(
                x3::get<typename T::first_type>(context)
              , *static_cast<base_type*>(this))
        {}

        using context_type::get;
    };

}}}

#endif
