//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_PILFER_HPP
#define BOOST_JSON_PILFER_HPP

#include <boost/json/detail/config.hpp>
#include <type_traits>
#include <utility>

/*
    Implements "pilfering" from P0308R0

    @see
        http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0308r0.html
*/

namespace boost {
namespace json {

/** Tag wrapper to specify pilfer-construction.

    This wrapper is used to specify a pilfer constructor
    overload.

    @par Example

    A pilfer constructor accepts a single argument
    of type @ref pilfered and throws nothing:

    @code
    struct T
    {
        T( pilfered<T> ) noexcept;
    };
    @endcode

    @note

    The constructor should not be marked explicit.

    @see @ref pilfer, @ref is_pilfer_constructible,
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0308r0.html">
        Valueless Variants Considered Harmful</a>
*/
template<class T>
class pilfered
{
    T& t_;

public:
    /** Constructor

        Construct the wrapper from `t`.

        @param t The pilferable object. Ownership
        is not transferred.
    */
    explicit
    constexpr
    pilfered(T&& t) noexcept
        : t_(t)
    {
    }

    /** Return a reference to the pilferable object.

        This returns a reference to the wrapped object.
    */
    constexpr T&
    get() const noexcept
    {
        return t_;
    }

    /** Return a pointer to the pilferable object.

        This returns a pointer to the wrapped object.
    */
    constexpr T*
    operator->() const noexcept
    {
        //return std::addressof(t_);
        return reinterpret_cast<T*>(
            const_cast<char *>(
                &reinterpret_cast<
                    const volatile char &>(t_)));
    }
};

#ifndef BOOST_JSON_DOCS
// VFALCO Renamed this to work around an msvc bug
namespace detail_pilfer {
template<class>
struct not_pilfered
{
};
} // detail_pilfer
#endif

/** Metafunction returning `true` if `T` is <em>PilferConstructible</em>

    If `T` can be pilfer constructed, this metafunction is
    equal to `std::true_type`. Otherwise it is equal to
    `std::false_type`.

    @see @ref pilfer, @ref pilfered,
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0308r0.html">
        Valueless Variants Considered Harmful</a>
*/
template<class T>
struct is_pilfer_constructible
#ifndef BOOST_JSON_DOCS
    : std::integral_constant<bool,
        std::is_nothrow_move_constructible<T>::value ||
        (
            std::is_nothrow_constructible<
                T, pilfered<T> >::value &&
            ! std::is_nothrow_constructible<
                T, detail_pilfer::not_pilfered<T> >::value
        )>
#endif
{
};

/** Indicate that an object `t` may be pilfered from.

    A <em>pilfer</em> operation is the construction
    of a new object of type `T` from an existing
    object `t`. After the construction, the only
    valid operation on the pilfered-from object is
    destruction. This permits optimizations beyond
    those available for a move-construction, as the
    pilfered-from object is not required to be in
    a "usable" state.
\n
    This is used similarly to `std::move`.

    @par Example

    A pilfer constructor accepts a single argument
    of type @ref pilfered and throws nothing:

    @code
    struct T
    {
        T( pilfered<T> ) noexcept;
    };
    @endcode

    Pilfer construction is performed using @ref pilfer :

    @code
    {
        T t1;                       // default construction
        T t2( pilfer( t1 ) );       // pilfer-construct from t1

        // At this point, t1 may only be destroyed
    }
    @endcode

    @see @ref pilfered, @ref is_pilfer_constructible,
    <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0308r0.html">
        Valueless Variants Considered Harmful</a>
*/
template<class T>
auto
pilfer(T&& t) noexcept ->
    typename std::conditional<
        std::is_nothrow_constructible<
            typename std::remove_reference<T>::type,
            pilfered<typename
                std::remove_reference<T>::type> >::value &&
        ! std::is_nothrow_constructible<
            typename std::remove_reference<T>::type,
            detail_pilfer::not_pilfered<typename
                std::remove_reference<T>::type> >::value,
        pilfered<typename std::remove_reference<T>::type>,
        typename std::remove_reference<T>::type&&
            >::type
{
    using U =
        typename std::remove_reference<T>::type;
    static_assert(
        is_pilfer_constructible<U>::value, "");
    return typename std::conditional<
        std::is_nothrow_constructible<
            U, pilfered<U> >::value &&
        ! std::is_nothrow_constructible<
            U, detail_pilfer::not_pilfered<U> >::value,
        pilfered<U>, U&&
            >::type(std::move(t));
}

/*
template<class T>
void
relocate(T* dest, T& src) noexcept
{
    static_assert(
        is_pilfer_constructible<T>::value, "");
    ::new(dest) T(pilfer(src));
    src.~T();
}
*/

} // json
} // boost


#endif
