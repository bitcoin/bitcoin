//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_SAVED_HANDLER_HPP
#define BOOST_BEAST_CORE_SAVED_HANDLER_HPP

#include <boost/beast/core/detail/config.hpp>

namespace boost {
namespace beast {

/** An invocable, nullary function object which holds a completion handler.

    This container can hold a type-erased instance of any completion
    handler, or it can be empty. When the container holds a value,
    the implementation maintains an instance of `net::executor_work_guard`
    for the handler's associated executor. Memory is dynamically allocated
    to store the completion handler, and the allocator may optionally
    be specified. Otherwise, the implementation uses the handler's
    associated allocator.
*/
class saved_handler
{
    class base;

    template<class, class>
    class impl;

    base* p_ = nullptr;

public:
    /// Default Constructor
    saved_handler() = default;

    /// Copy Constructor (deleted)
    saved_handler(saved_handler const&) = delete;

    /// Copy Assignment (deleted)
    saved_handler& operator=(saved_handler const&) = delete;

    /// Destructor
    BOOST_BEAST_DECL
    ~saved_handler();

    /// Move Constructor
    BOOST_BEAST_DECL
    saved_handler(saved_handler&& other) noexcept;

    /// Move Assignment
    BOOST_BEAST_DECL
    saved_handler&
    operator=(saved_handler&& other) noexcept;

    /// Returns `true` if `*this` contains a completion handler.
    bool
    has_value() const noexcept
    {
        return p_ != nullptr;
    }

    /** Store a completion handler in the container.

        Requires `this->has_value() == false`.

        @param handler The completion handler to store.
        The implementation takes ownership of the handler by performing a decay-copy.

        @param alloc The allocator to use.
    */
    template<class Handler, class Allocator>
    void
    emplace(Handler&& handler, Allocator const& alloc);

    /** Store a completion handler in the container.

        Requires `this->has_value() == false`. The
        implementation will use the handler's associated
        allocator to obtian storage.

        @param handler The completion handler to store.
        The implementation takes ownership of the handler by performing a decay-copy.
    */
    template<class Handler>
    void
    emplace(Handler&& handler);

    /** Discard the saved handler, if one exists.

        If `*this` contains an object, it is destroyed.

        @returns `true` if an object was destroyed.
    */
    BOOST_BEAST_DECL
    bool
    reset() noexcept;

    /** Unconditionally invoke the stored completion handler.

        Requires `this->has_value() == true`. Any dynamic memory
        used is deallocated before the stored completion handler
        is invoked. The executor work guard is also reset before
        the invocation.
    */
    BOOST_BEAST_DECL
    void
    invoke();

    /** Conditionally invoke the stored completion handler.

        Invokes the stored completion handler if
        `this->has_value() == true`, otherwise does nothing. Any
        dynamic memory used is deallocated before the stored completion
        handler is invoked. The executor work guard is also reset before
        the invocation.

        @return `true` if the invocation took place.
    */
    BOOST_BEAST_DECL
    bool
    maybe_invoke();
};

} // beast
} // boost

#include <boost/beast/core/impl/saved_handler.hpp>
#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/core/impl/saved_handler.ipp>
#endif

#endif
