/*!
@file
Defines operators for Monads.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_OPERATORS_MONAD_HPP
#define BOOST_HANA_DETAIL_OPERATORS_MONAD_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/tag_of.hpp>
#include <boost/hana/fwd/chain.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <typename Tag>
    struct monad_operators {
        static constexpr bool value = false;
    };

    namespace operators {
        template <typename Xs, typename F, typename = typename std::enable_if<
            detail::monad_operators<typename hana::tag_of<Xs>::type>::value
        >::type>
        constexpr auto operator|(Xs&& xs, F&& f)
        { return hana::chain(static_cast<Xs&&>(xs), static_cast<F&&>(f)); }
    } // end namespace operators
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_OPERATORS_MONAD_HPP
