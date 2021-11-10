/*!
@file
Forward declares `boost::hana::lazy`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_LAZY_HPP
#define BOOST_HANA_FWD_LAZY_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/core/make.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-datatypes
    //! `hana::lazy` implements superficial laziness via a monadic interface.
    //!
    //! It is important to understand that the laziness implemented by `lazy`
    //! is only superficial; only function applications made inside the `lazy`
    //! monad can be made lazy, not all their subexpressions.
    //!
    //!
    //! @note
    //! The actual representation of `hana::lazy` is completely
    //! implementation-defined. Lazy values may only be created through
    //! `hana::make_lazy`, and they can be stored in variables using
    //! `auto`, but any other assumption about the representation of
    //! `hana::lazy<...>` should be avoided. In particular, one should
    //! not rely on the fact that `hana::lazy<...>` can be pattern-matched
    //! on, because it may be a dependent type.
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! 1. `Functor`\n
    //! Applying a function over a lazy value with `transform` returns the
    //! result of applying the function, as a lazy value.
    //! @include example/lazy/functor.cpp
    //!
    //! 2. `Applicative`\n
    //! A normal value can be lifted into a lazy value by using `lift<lazy_tag>`.
    //! A lazy function can be lazily applied to a lazy value by using `ap`.
    //!
    //! 3. `Monad`\n
    //! The `lazy` monad allows combining lazy computations into larger
    //! lazy computations. Note that the `|` operator can be used in place
    //! of the `chain` function.
    //! @include example/lazy/monad.cpp
    //!
    //! 4. `Comonad`\n
    //! The `lazy` comonad allows evaluating a lazy computation to get its
    //! result and lazily applying functions taking lazy inputs to lazy
    //! values. This [blog post][1]  goes into more details about lazy
    //! evaluation and comonads.
    //! @include example/lazy/comonad.cpp
    //!
    //!
    //! @note
    //! `hana::lazy` only models a few concepts because providing more
    //! functionality would require evaluating the lazy values in most cases.
    //! Since this raises some issues such as side effects and memoization,
    //! the interface is kept minimal.
    //!
    //!
    //! [1]: http://ldionne.com/2015/03/16/laziness-as-a-comonad
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename implementation_defined>
    struct lazy {
        //! Equivalent to `hana::chain`.
        template <typename ...T, typename F>
        friend constexpr auto operator|(lazy<T...>, F);
    };
#else
    // We do not _actually_ define the lazy<...> type. Per the documentation,
    // users can't rely on it being anything, and so they should never use
    // it explicitly. The implementation in <boost/hana/lazy.hpp> is much
    // simpler if we use different types for lazy calls and lazy values.
#endif

    //! Tag representing `hana::lazy`.
    //! @relates hana::lazy
    struct lazy_tag { };

    //! Lifts a normal value to a lazy one.
    //! @relates hana::lazy
    //!
    //! `make<lazy_tag>` can be used to lift a normal value or a function call
    //! into a lazy expression. Precisely, `make<lazy_tag>(x)` is a lazy value
    //! equal to `x`, and `make<lazy_tag>(f)(x1, ..., xN)` is a lazy function
    //! call that is equal to `f(x1, ..., xN)` when it is `eval`uated.
    //!
    //! @note
    //! It is interesting to note that `make<lazy_tag>(f)(x1, ..., xN)` is
    //! equivalent to
    //! @code
    //!     ap(make<lazy_tag>(f), lift<lazy_tag>(x1), ..., lift<lazy_tag>(xN))
    //! @endcode
    //! which in turn is equivalent to `make<lazy_tag>(f(x1, ..., xN))`, except
    //! for the fact that the inner call to `f` is evaluated lazily.
    //!
    //!
    //! Example
    //! -------
    //! @include example/lazy/make.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <>
    constexpr auto make<lazy_tag> = [](auto&& x) {
        return lazy<implementation_defined>{forwarded(x)};
    };
#endif

    //! Alias to `make<lazy_tag>`; provided for convenience.
    //! @relates hana::lazy
    //!
    //! Example
    //! -------
    //! @include example/lazy/make.cpp
    BOOST_HANA_INLINE_VARIABLE constexpr auto make_lazy = make<lazy_tag>;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_LAZY_HPP
