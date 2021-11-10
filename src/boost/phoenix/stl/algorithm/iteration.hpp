// Copyright 2005 Daniel Wallin. 
// Copyright 2005 Joel de Guzman.
// Copyright 2005 Dan Marsden. 
//
// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Modeled after range_ex, Copyright 2004 Eric Niebler

#ifndef BOOST_PHOENIX_ALGORITHM_ITERATION_HPP
#define BOOST_PHOENIX_ALGORITHM_ITERATION_HPP

#include <algorithm>
#include <numeric>

#include <boost/phoenix/stl/algorithm/detail/begin.hpp>
#include <boost/phoenix/stl/algorithm/detail/end.hpp>

#include <boost/phoenix/function/adapt_callable.hpp>

namespace boost { namespace phoenix {
    namespace impl
    {
        struct for_each
        {
            template <typename Sig>
            struct result;

            template<typename This, class R, class F>
            struct result<This(R&, F)>
                : result<This(R&, F const &)>
            {};

            template<typename This, class R, class F>
            struct result<This(R&, F &)>
            {
                typedef F type;
            };

            template<class R, class F>
            F const operator()(R& r, F const& fn) const
            {        
                return std::for_each(detail::begin_(r), detail::end_(r), fn);
            }
        };

        struct accumulate
        {
            template <typename Sig>
            struct result;
            
            template<typename This, class R, class I>
            struct result<This(R&, I)>
                : result<This(R&, I const &)>
            {};
            
            template<typename This, class R, class I>
            struct result<This(R&, I &)>
            {
                typedef I type;
            };
            
            template<typename This, class R, class I, class C>
            struct result<This(R&, I, C)>
                : result<This(R&, I const &, C)>
            {};

            template<typename This, class R, class I, class C>
            struct result<This(R&, I &, C)>
            {
                typedef I type;
            };

            template<class R, class I>
            I
            operator()(R& r, I i) const
            {
                return std::accumulate(detail::begin_(r), detail::end_(r), i);
            }

            template<class R, class I, class C>
            I
            operator()(R& r, I i, C c) const
            {
                return std::accumulate(detail::begin_(r), detail::end_(r), i, c);
            }
        };
    }

    BOOST_PHOENIX_ADAPT_CALLABLE(for_each, impl::for_each, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(accumulate, impl::accumulate, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(accumulate, impl::accumulate, 3)

}}

#endif
