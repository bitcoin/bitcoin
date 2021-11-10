/*!
@file
Forward declares `boost::hana::union_`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_UNION_HPP
#define BOOST_HANA_FWD_UNION_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    // Note: This function is documented per datatype/concept only.
    //! @cond
    template <typename T, typename = void>
    struct union_impl : union_impl<T, when<true>> { };
    //! @endcond

    struct union_t {
        template <typename Xs, typename Ys>
        constexpr auto operator()(Xs&&, Ys&&) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr union_t union_{};
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_UNION_HPP
