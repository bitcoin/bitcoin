/*!
@file
Defines `boost::hana::on`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FUNCTIONAL_ON_HPP
#define BOOST_HANA_FUNCTIONAL_ON_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/create.hpp>
#include <boost/hana/functional/infix.hpp>

#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-functional
    //! Invoke a function with the result of invoking another function on
    //! each argument.
    //!
    //! Specifically, `on(f, g)` is a function such that
    //! @code
    //!     on(f, g)(x...) == f(g(x)...)
    //! @endcode
    //!
    //! For convenience, `on` also supports infix application as provided
    //! by `infix`.
    //!
    //!
    //! @note
    //! `on` is associative, i.e. `on(f, on(g, h))` is equivalent to
    //! `on(on(f, g), h)`.
    //!
    //! @internal
    //! ### Proof of associativity
    //!
    //! @code
    //!     on(f, on(g, h))(xs...) == f(on(g, h)(xs)...)
    //!                            == f(g(h(xs))...)
    //!
    //!     on(on(f, g), h)(xs...) == on(f, g)(h(xs)...)
    //!                            == f(g(h(xs))...)
    //! @endcode
    //! @endinternal
    //!
    //!
    //! ### Example
    //! @include example/functional/on.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto on = infix([](auto&& f, auto&& g) {
        return [perfect-capture](auto&& ...x) -> decltype(auto) {
            return forwarded(f)(g(forwarded(x))...);
        };
    });
#else
    template <typename F, typename G>
    struct on_t {
        F f; G g;
        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) const& {
            return f(g(static_cast<X&&>(x))...);
        }

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) & {
            return f(g(static_cast<X&&>(x))...);
        }

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) && {
            return std::move(f)(g(static_cast<X&&>(x))...);
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr auto on = infix(detail::create<on_t>{});
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FUNCTIONAL_ON_HPP
