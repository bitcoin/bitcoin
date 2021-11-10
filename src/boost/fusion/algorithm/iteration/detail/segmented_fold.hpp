/*=============================================================================
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_FOLD_S_HPP_INCLUDED)
#define BOOST_FUSION_FOLD_S_HPP_INCLUDED

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/algorithm/iteration/fold_fwd.hpp>
#include <boost/fusion/support/segmented_fold_until.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename Fun>
    struct segmented_fold_fun
    {
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        explicit segmented_fold_fun(Fun const& f)
          : fun(f)
        {}

        Fun const& fun;

        template <typename Sequence, typename State, typename Context>
        struct apply
        {
            typedef typename result_of::fold<Sequence, State, Fun>::type type;
            typedef mpl::true_ continue_type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static type call(Sequence& seq, State const& state, Context const&, segmented_fold_fun const& fun)
            {
                return fusion::fold(seq, state, fun.fun);
            }
        };
    };

    // The default implementation of this lives in detail/fold.hpp
    template <typename Sequence, typename State, typename Fun, bool IsSequence, bool IsSegmented>
    struct result_of_fold;

    template <typename Sequence, typename State, typename Fun>
    struct result_of_fold<Sequence, State, Fun, true, true>
    {
        typedef
            typename result_of::segmented_fold_until<
                Sequence,
                State,
                segmented_fold_fun<Fun>
            >::type
        type;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        static type call(Sequence& seq, State& state, Fun& fun)
        {
            return fusion::segmented_fold_until(seq, state, segmented_fold_fun<Fun>(fun));
        }
    };
}}}

#endif
