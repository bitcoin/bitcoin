/*!
@file
Forward declares `boost::hana::eval`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_EVAL_HPP
#define BOOST_HANA_FWD_EVAL_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Evaluate a lazy value and return it.
    //! @relates hana::lazy
    //!
    //! Given a lazy expression `expr`, `eval` evaluates `expr` and returns
    //! the result as a normal value. However, for convenience, `eval` can
    //! also be used with nullary and unary function objects. Specifically,
    //! if `expr` is not a `hana::lazy`, it is called with no arguments at
    //! all and the result of that call (`expr()`) is returned. Otherwise,
    //! if `expr()` is ill-formed, then `expr(hana::id)` is returned instead.
    //! If that expression is ill-formed, then a compile-time error is
    //! triggered.
    //!
    //! The reason for allowing nullary callables in `eval` is because this
    //! allows using nullary lambdas as lazy branches to `eval_if`, which
    //! is convenient. The reason for allowing unary callables and calling
    //! them with `hana::id` is because this allows deferring the
    //! compile-time evaluation of selected expressions inside the callable.
    //! How this can be achieved is documented by `hana::eval_if`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/eval.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto eval = [](auto&& see_documentation) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename T, typename = void>
    struct eval_impl : eval_impl<T, when<true>> { };

    struct eval_t {
        template <typename Expr>
        constexpr decltype(auto) operator()(Expr&& expr) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr eval_t eval{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_EVAL_HPP
