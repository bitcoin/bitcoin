/*!
@file
Defines a replacement for `std::decay`, which is sometimes too slow at
compile-time.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_DECAY_HPP
#define BOOST_HANA_DETAIL_DECAY_HPP

#include <boost/hana/config.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    //! @ingroup group-details
    //! Equivalent to `std::decay`, except faster.
    //!
    //! `std::decay` in libc++ is implemented in a suboptimal way. Since
    //! this is used literally everywhere by the `make<...>` functions, it
    //! is very important to keep this as efficient as possible.
    //!
    //! @note
    //! `std::decay` is still being used in some places in the library.
    //! Indeed, this is a peephole optimization and it would not be wise
    //! to clutter the code with our own implementation of `std::decay`,
    //! except when this actually makes a difference in compile-times.
    template <typename T, typename U = typename std::remove_reference<T>::type>
    struct decay {
        using type = typename std::remove_cv<U>::type;
    };

    template <typename T, typename U>
    struct decay<T, U[]> { using type = U*; };
    template <typename T, typename U, std::size_t N>
    struct decay<T, U[N]> { using type = U*; };

    template <typename T, typename R, typename ...A>
    struct decay<T, R(A...)> { using type = R(*)(A...); };
    template <typename T, typename R, typename ...A>
    struct decay<T, R(A..., ...)> { using type = R(*)(A..., ...); };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_DECAY_HPP
