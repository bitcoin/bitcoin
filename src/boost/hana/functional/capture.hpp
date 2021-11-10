/*!
@file
Defines `boost::hana::capture`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FUNCTIONAL_CAPTURE_HPP
#define BOOST_HANA_FUNCTIONAL_CAPTURE_HPP

#include <boost/hana/basic_tuple.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/functional/partial.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-functional
    //! Create a function capturing the given variables.
    //!
    //! Given 0 or more variables, `capture` creates a closure that can be
    //! used to partially apply a function. This is very similar to `partial`,
    //! except that `capture` allows the partially applied function to be
    //! specified later. Specifically, `capture(vars...)` is a function object
    //! taking a function `f` and returning `f` partially applied to `vars...`.
    //! In other words,
    //! @code
    //!     capture(vars...)(f)(args...) == f(vars..., args...)
    //! @endcode
    //!
    //! @note
    //! The arity of `f` must match the total number of arguments passed to
    //! it, i.e. `sizeof...(vars) + sizeof...(args)`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/functional/capture.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto capture = [](auto&& ...variables) {
        return [perfect-capture](auto&& f) {
            return [perfect-capture](auto&& ...args) -> decltype(auto) {
                return forwarded(f)(forwarded(variables)..., forwarded(args)...);
            };
        };
    };
#else
    namespace detail {
        template <typename F, typename Closure, std::size_t ...i>
        constexpr auto apply_capture(F&& f, Closure&& closure, std::index_sequence<i...>) {
            return hana::partial(static_cast<F&&>(f),
                hana::at_c<i>(static_cast<Closure&&>(closure).storage_)...
            );
        }
    }

    template <typename ...X>
    struct capture_t;

    struct make_capture_t {
        struct secret { };
        template <typename ...X>
        constexpr capture_t<typename detail::decay<X>::type...>
        operator()(X&& ...x) const {
            return {secret{}, static_cast<X&&>(x)...};
        }
    };

    template <typename ...X>
    struct capture_t {
        template <typename ...Y>
        constexpr capture_t(make_capture_t::secret, Y&& ...y)
            : storage_{static_cast<Y&&>(y)...}
        { }

        basic_tuple<X...> storage_;

        template <typename F>
        constexpr auto operator()(F&& f) const& {
            return detail::apply_capture(
                static_cast<F&&>(f), *this,
                std::make_index_sequence<sizeof...(X)>{}
            );
        }

        template <typename F>
        constexpr auto operator()(F&& f) & {
            return detail::apply_capture(
                static_cast<F&&>(f), *this,
                std::make_index_sequence<sizeof...(X)>{}
            );
        }

        template <typename F>
        constexpr auto operator()(F&& f) && {
            return detail::apply_capture(
                static_cast<F&&>(f), static_cast<capture_t&&>(*this),
                std::make_index_sequence<sizeof...(X)>{}
            );
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr make_capture_t capture{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FUNCTIONAL_CAPTURE_HPP
