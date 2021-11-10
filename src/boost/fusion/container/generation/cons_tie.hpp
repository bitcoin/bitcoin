/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONS_TIE_07182005_0854)
#define FUSION_CONS_TIE_07182005_0854

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/list/cons.hpp>

namespace boost { namespace fusion
{
    struct nil_;

    namespace result_of
    {
        template <typename Car, typename Cdr = nil_>
        struct cons_tie
        {
            typedef cons<Car&, Cdr> type;
        };
    }

    // $$$ do we really want a cons_tie? $$$
    template <typename Car>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline cons<Car&>
    cons_tie(Car& car)
    {
        return cons<Car&>(car);
    }

    // $$$ do we really want a cons_tie? $$$
    template <typename Car, typename Cdr>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline cons<Car&, Cdr>
    cons_tie(Car& car, Cdr const& cdr)
    {
        return cons<Car&, Cdr>(car, cdr);
    }
}}

#endif

