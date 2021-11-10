//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_MEMORY_RESOURCE_HPP
#define BOOST_JSON_MEMORY_RESOURCE_HPP

#include <boost/json/detail/config.hpp>

#ifdef BOOST_JSON_STANDALONE
# if __has_include(<memory_resource>)
#  include <memory_resource>
#  ifndef __cpp_lib_memory_resource
#   error Support for std::memory_resource is required to use Boost.JSON standalone
#  endif
# elif __has_include(<experimental/memory_resource>)
#  include <experimental/memory_resource>
#  warning Support for std::memory_resource is required to use Boost.JSON standalone, using std::experimental::memory_resource as fallback
# else
#  error Header <memory_resource> is required to use Boost.JSON standalone
# endif
#else
# include <boost/container/pmr/memory_resource.hpp>
# include <boost/container/pmr/polymorphic_allocator.hpp>
#endif

BOOST_JSON_NS_BEGIN

#ifdef BOOST_JSON_DOCS

/** The type of memory resource used by the library.

    This type alias is set depending
    on how the library is configured:

    @par Use with Boost

    If the macro `BOOST_JSON_STANDALONE` is
    not defined, this type will be an alias
    for `boost::container::pmr::memory_resource`.
    Compiling a program using the library will
    require Boost, and a compiler conforming
    to C++11 or later.

    @par Use without Boost

    If the macro `BOOST_JSON_STANDALONE` is
    defined, this type will be an alias
    for `std::pmr::memory_resource`.
    Compiling a program using the library will
    require only a compiler conforming to C++17
    or later.

    @see https://en.cppreference.com/w/cpp/memory/memory_resource
*/
class memory_resource
{
};

/** The type of polymorphic allocator used by the library.

    This type alias is set depending
    on how the library is configured:

    @par Use with Boost

    If the macro `BOOST_JSON_STANDALONE` is
    not defined, this type will be an alias template
    for `boost::container::pmr::polymorphic_allocator`.
    Compiling a program using the library will
    require Boost, and a compiler conforming
    to C++11 or later.

    @par Use without Boost

    If the macro `BOOST_JSON_STANDALONE` is
    defined, this type will be an alias template
    for `std::pmr::polymorphic_allocator`.
    Compiling a program using the library will
    require only a compiler conforming to C++17
    or later.

    @see https://en.cppreference.com/w/cpp/memory/polymorphic_allocator
*/
template<class T>
class polymorphic_allocator;

// VFALCO Bug: doc toolchain won't make this a ref
//using memory_resource = __see_below__;

#elif defined(BOOST_JSON_STANDALONE)

# if __has_include(<memory_resource>)
using memory_resource = std::pmr::memory_resource;
template<class T>
using polymorphic_allocator =
    std::pmr::polymorphic_allocator<T>;

# else
using memory_resource = std::experimental::pmr::memory_resource;
template<class T>
using polymorphic_allocator =
    std::experimental::pmr::polymorphic_allocator<T>;

# endif

#else
using memory_resource = boost::container::pmr::memory_resource;

template<class T>
using polymorphic_allocator =
    boost::container::pmr::polymorphic_allocator<T>;
#endif

/** Return true if a memory resource's deallocate function has no effect.

    This metafunction may be specialized to indicate to
    the library that calls to the `deallocate` function of
    a @ref memory_resource have no effect. The implementation
    will elide such calls when it is safe to do so. By default,
    the implementation assumes that all memory resources
    require a call to `deallocate` for each memory region
    obtained by calling `allocate`.

    @par Example

    This example specializes the metafuction for `my_resource`,
    to indicate that calls to deallocate have no effect:

    @code

    // Forward-declaration for a user-defined memory resource
    struct my_resource;

    // It is necessary to specialize the template from
    // inside the namespace in which it is declared:

    namespace boost {
    namespace json {

    template<>
    struct is_deallocate_trivial< my_resource >
    {
        static constexpr bool value = true;
    };

    } // namespace json
    } // namespace boost

    @endcode

    It is usually not necessary for users to check this trait.
    Instead, they can call @ref storage_ptr::is_deallocate_trivial
    to determine if the pointed-to memory resource has a trivial
    deallocate function.

    @see
        @ref memory_resource,
        @ref storage_ptr
*/
template<class T>
struct is_deallocate_trivial
{
    /** A bool equal to true if calls to `T::do_deallocate` have no effect.

        The primary template sets `value` to false.
    */
    static constexpr bool value = false;
};

BOOST_JSON_NS_END

#endif
