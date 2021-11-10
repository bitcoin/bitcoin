//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_MONOTONIC_RESOURCE_HPP
#define BOOST_JSON_MONOTONIC_RESOURCE_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/memory_resource.hpp>
#include <boost/json/storage_ptr.hpp>
#include <cstddef>
#include <utility>

BOOST_JSON_NS_BEGIN

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // class needs to have dll-interface to be used by clients of class
#pragma warning(disable: 4275) // non dll-interface class used as base for dll-interface class
#endif

//----------------------------------------------------------

/** A dynamically allocating resource with a trivial deallocate

    This memory resource is a special-purpose resource
    that releases allocated memory only when the resource
    is destroyed (or when @ref release is called).
    It has a trivial deallocate function; that is, the
    metafunction @ref is_deallocate_trivial returns `true`.
\n
    The resource can be constructed with an initial buffer.
    If there is no initial buffer, or if the buffer is
    exhausted, subsequent dynamic allocations are made from
    the system heap. The size of buffers obtained in this
    fashion follow a geometric progression.
\n
    The purpose of this resource is to optimize the use
    case for performing many allocations, followed by
    deallocating everything at once. This is precisely the
    pattern of memory allocation which occurs when parsing:
    allocation is performed for each parsed element, and
    when the the resulting @ref value is no longer needed,
    the entire structure is destroyed. However, it is not
    suited for modifying the value after parsing is
    complete; reallocations waste memory, since the
    older buffer is not reclaimed until the resource
    is destroyed.

    @par Example

    This parses a JSON into a value which uses a local
    stack buffer, then prints the result.

    @code

    unsigned char buf[ 4000 ];
    monotonic_resource mr( buf );

    // Parse the string, using our memory resource
    auto const jv = parse( "[1,2,3]", &mr );

    // Print the JSON
    std::cout << jv;

    @endcode

    @note The total amount of memory dynamically
    allocated is monotonically increasing; That is,
    it never decreases.

    @par Thread Safety
    Members of the same instance may not be
    called concurrently.

    @see
        https://en.wikipedia.org/wiki/Region-based_memory_management
*/
class BOOST_JSON_CLASS_DECL
    monotonic_resource final
    : public memory_resource
{
    struct block;
    struct block_base
    {
        void* p;
        std::size_t avail;
        std::size_t size;
        block_base* next;
    };

    block_base buffer_;
    block_base* head_ = &buffer_;
    std::size_t next_size_ = 1024;
    storage_ptr upstream_;

    static constexpr std::size_t min_size_ = 1024;
    inline static constexpr std::size_t max_size();
    inline static std::size_t round_pow2(
        std::size_t n) noexcept;
    inline static std::size_t next_pow2(
        std::size_t n) noexcept;

public:
    /// Copy constructor (deleted)
    monotonic_resource(
        monotonic_resource const&) = delete;

    /// Copy assignment (deleted)
    monotonic_resource& operator=(
        monotonic_resource const&) = delete;

    /** Destructor

        Deallocates all the memory owned by this resource.

        @par Effects
        @code
        this->release();
        @endcode

        @par Complexity
        Linear in the number of deallocations performed.

        @par Exception Safety
        No-throw guarantee.
    */
    ~monotonic_resource();

    /** Constructor

        This constructs the resource and indicates
        that the first internal dynamic allocation
        shall be at least `initial_size` bytes.
    \n
        This constructor is guaranteed not to perform
        any dynamic allocations.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param initial_size The size of the first
        internal dynamic allocation. If this is lower
        than the implementation-defined lower limit, then
        the lower limit is used instead.

        @param upstream An optional upstream memory resource
        to use for performing internal dynamic allocations.
        If this parameter is omitted, the default resource
        is used.
    */
    explicit
    monotonic_resource(
        std::size_t initial_size = 1024,
        storage_ptr upstream = {}) noexcept;

    /** Constructor

        This constructs the resource and indicates that
        subsequent allocations should use the specified
        caller-owned buffer.
        When this buffer is exhausted, dynamic allocations
        from the upstream resource are made.
    \n
        This constructor is guaranteed not to perform
        any dynamic allocations.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param buffer The buffer to use.
        Ownership is not transferred; the caller is
        responsible for ensuring that the lifetime of
        the buffer extends until the resource is destroyed.

        @param size The number of valid bytes pointed
        to by `buffer`.

        @param upstream An optional upstream memory resource
        to use for performing internal dynamic allocations.
        If this parameter is omitted, the default resource
        is used.
    */
    /** @{ */
    monotonic_resource(
        unsigned char* buffer,
        std::size_t size,
        storage_ptr upstream = {}) noexcept;

#if defined(__cpp_lib_byte) || defined(BOOST_JSON_DOCS)
    monotonic_resource(
        std::byte* buffer,
        std::size_t size,
        storage_ptr upstream) noexcept
        : monotonic_resource(reinterpret_cast<
            unsigned char*>(buffer), size,
                std::move(upstream))
    {
    }
#endif
    /** @} */

    /** Constructor

        This constructs the resource and indicates that
        subsequent allocations should use the specified
        caller-owned buffer.
        When this buffer is exhausted, dynamic allocations
        from the upstream resource are made.
    \n
        This constructor is guaranteed not to perform
        any dynamic allocations.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param buffer The buffer to use.
        Ownership is not transferred; the caller is
        responsible for ensuring that the lifetime of
        the buffer extends until the resource is destroyed.

        @param upstream An optional upstream memory resource
        to use for performing internal dynamic allocations.
        If this parameter is omitted, the default resource
        is used.
    */
    /** @{ */
    template<std::size_t N>
    explicit
    monotonic_resource(
        unsigned char(&buffer)[N],
        storage_ptr upstream = {}) noexcept
        : monotonic_resource(&buffer[0],
            N, std::move(upstream))
    {
    }

#if defined(__cpp_lib_byte) || defined(BOOST_JSON_DOCS)
    template<std::size_t N>
    explicit
    monotonic_resource(
        std::byte(&buffer)[N],
        storage_ptr upstream = {}) noexcept
        : monotonic_resource(&buffer[0],
            N, std::move(upstream))
    {
    }
#endif
    /** @} */

#ifndef BOOST_JSON_DOCS
    // Safety net for accidental buffer overflows
    template<std::size_t N>
    monotonic_resource(
        unsigned char(&buffer)[N],
        std::size_t n,
        storage_ptr upstream = {}) noexcept
        : monotonic_resource(&buffer[0],
            n, std::move(upstream))
    {
        // If this goes off, check your parameters
        // closely, chances are you passed an array
        // thinking it was a pointer.
        BOOST_ASSERT(n <= N);
    }

#ifdef __cpp_lib_byte
    // Safety net for accidental buffer overflows
    template<std::size_t N>
    monotonic_resource(
        std::byte(&buffer)[N],
        std::size_t n,
        storage_ptr upstream = {}) noexcept
        : monotonic_resource(&buffer[0],
            n, std::move(upstream))
    {
        // If this goes off, check your parameters
        // closely, chances are you passed an array
        // thinking it was a pointer.
        BOOST_ASSERT(n <= N);
    }
#endif
#endif

    /** Release all allocated memory.

        This function deallocates all allocated memory.
        If an initial buffer was provided upon construction,
        then all of the bytes will be available again for
        allocation. Allocated memory is deallocated even
        if deallocate has not been called for some of
        the allocated blocks.

        @par Complexity
        Linear in the number of deallocations performed.

        @par Exception Safety
        No-throw guarantee.
    */
    void
    release() noexcept;

protected:
#ifndef BOOST_JSON_DOCS
    void*
    do_allocate(
        std::size_t n,
        std::size_t align) override;

    void
    do_deallocate(
        void* p,
        std::size_t n,
        std::size_t align) override;

    bool
    do_is_equal(
        memory_resource const& mr) const noexcept override;
#endif
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

template<>
struct is_deallocate_trivial<
    monotonic_resource>
{
    static constexpr bool value = true;
};

BOOST_JSON_NS_END

#endif
