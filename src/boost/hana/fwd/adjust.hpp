/*!
@file
Forward declares `boost::hana::adjust`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_ADJUST_HPP
#define BOOST_HANA_FWD_ADJUST_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Apply a function on all the elements of a structure that compare
    //! equal to some value.
    //! @ingroup group-Functor
    //!
    //!
    //! Signature
    //! ---------
    //! Given `F` a Functor and `U` a type that can be compared with `T`'s,
    //! the signature is
    //! \f$
    //!     \mathtt{adjust} : F(T) \times U \times (T \to T) \to F(T)
    //! \f$
    //!
    //! @param xs
    //! The structure to adjust with `f`.
    //!
    //! @param value
    //! An object that is compared with each element `x` of the structure.
    //! Elements of the structure that compare equal to `value` are adjusted
    //! with the `f` function.
    //!
    //! @param f
    //! A function called as `f(x)` on the element(s) of the structure that
    //! compare equal to `value`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/adjust.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto adjust = [](auto&& xs, auto&& value, auto&& f) {
        return tag-dispatched;
    };
#else
    template <typename Xs, typename = void>
    struct adjust_impl : adjust_impl<Xs, when<true>> { };

    struct adjust_t {
        template <typename Xs, typename Value, typename F>
        constexpr auto operator()(Xs&& xs, Value&& value, F&& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr adjust_t adjust{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_ADJUST_HPP
