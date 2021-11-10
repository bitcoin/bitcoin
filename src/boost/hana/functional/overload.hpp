/*!
@file
Defines `boost::hana::overload`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FUNCTIONAL_OVERLOAD_HPP
#define BOOST_HANA_FUNCTIONAL_OVERLOAD_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-functional
    //! Pick one of several functions to call based on overload resolution.
    //!
    //! Specifically, `overload(f1, f2, ..., fn)` is a function object such
    //! that
    //! @code
    //!     overload(f1, f2, ..., fn)(x...) == fk(x...)
    //! @endcode
    //!
    //! where `fk` is the function of `f1, ..., fn` that would be called if
    //! overload resolution was performed amongst that set of functions only.
    //! If more than one function `fk` would be picked by overload resolution,
    //! then the call is ambiguous.
    //!
    //! ### Example
    //! @include example/functional/overload.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto overload = [](auto&& f1, auto&& f2, ..., auto&& fn) {
        return [perfect-capture](auto&& ...x) -> decltype(auto) {
            return forwarded(fk)(forwarded(x)...);
        };
    };
#else
    template <typename F, typename ...G>
    struct overload_t
        : overload_t<F>::type
        , overload_t<G...>::type
    {
        using type = overload_t;
        using overload_t<F>::type::operator();
        using overload_t<G...>::type::operator();

        template <typename F_, typename ...G_>
        constexpr explicit overload_t(F_&& f, G_&& ...g)
            : overload_t<F>::type(static_cast<F_&&>(f))
            , overload_t<G...>::type(static_cast<G_&&>(g)...)
        { }
    };

    template <typename F>
    struct overload_t<F> { using type = F; };

    template <typename R, typename ...Args>
    struct overload_t<R(*)(Args...)> {
        using type = overload_t;
        R (*fptr_)(Args...);

        explicit constexpr overload_t(R (*fp)(Args...))
            : fptr_(fp)
        { }

        constexpr R operator()(Args ...args) const
        { return fptr_(static_cast<Args&&>(args)...); }
    };

    struct make_overload_t {
        template <typename ...F,
            typename Overload = typename overload_t<
                typename detail::decay<F>::type...
            >::type
        >
        constexpr Overload operator()(F&& ...f) const {
            return Overload(static_cast<F&&>(f)...);
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr make_overload_t overload{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FUNCTIONAL_OVERLOAD_HPP
