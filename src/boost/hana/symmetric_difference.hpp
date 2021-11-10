/*!
@file
Defines `boost::hana::symmetric_difference`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_SYMMETRIC_DIFFERENCE_HPP
#define BOOST_HANA_SYMMETRIC_DIFFERENCE_HPP

#include <boost/hana/fwd/symmetric_difference.hpp>

#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/difference.hpp>
#include <boost/hana/union.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Ys>
    constexpr auto symmetric_difference_t::operator()(Xs&& xs, Ys&& ys) const {
        using S = typename hana::tag_of<Xs>::type;
        using SymmetricDifference = BOOST_HANA_DISPATCH_IF(symmetric_difference_impl<S>,
            true
        );

        return SymmetricDifference::apply(static_cast<Xs&&>(xs), static_cast<Ys&&>(ys));
    }
    //! @endcond

    template <typename S, bool condition>
    struct symmetric_difference_impl<S, when<condition>> : default_ {
        template <typename Xs, typename Ys>
        static constexpr auto apply(Xs&& xs, Ys&& ys) {
            return hana::union_(
                hana::difference(xs, ys),
                hana::difference(ys, xs)
            );
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_SYMMETRIC_DIFFERENCE_HPP
