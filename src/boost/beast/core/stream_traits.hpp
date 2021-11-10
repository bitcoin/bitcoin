//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_STREAM_TRAITS_HPP
#define BOOST_BEAST_STREAM_TRAITS_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/detail/static_const.hpp>
#include <boost/beast/core/detail/stream_traits.hpp>
#include <boost/asio/basic_socket.hpp>

namespace boost {
namespace beast {

/** A trait to determine the lowest layer type of a stack of stream layers.

    If `t.next_layer()` is well-defined for an object `t` of type `T`,
    then `lowest_layer_type<T>` will be an alias for
    `lowest_layer_type<decltype(t.next_layer())>`,
    otherwise it will be the type
    `std::remove_reference<T>`.

    @param T The type to determine the lowest layer type of.

    @return The type of the lowest layer.
*/
template<class T>
#if BOOST_BEAST_DOXYGEN
using lowest_layer_type = __see_below__;
#else
using lowest_layer_type = detail::lowest_layer_type<T>;
#endif

/** Return the lowest layer in a stack of stream layers.

    If `t.next_layer()` is well-defined, returns
    `get_lowest_layer(t.next_layer())`. Otherwise, it returns `t`.

    A stream layer is an object of class type which wraps another object through
    composition, and meets some or all of the named requirements of the wrapped
    type while optionally changing behavior. Examples of stream layers include
    `net::ssl::stream` or @ref beast::websocket::stream. The owner of a stream
    layer can interact directly with the wrapper, by passing it to stream
    algorithms. Or, the owner can obtain a reference to the wrapped object by
    calling `next_layer()` and accessing its members. This is necessary when it is
    desired to access functionality in the next layer which is not available
    in the wrapper. For example, @ref websocket::stream permits reading and
    writing, but in order to establish the underlying connection, members
    of the wrapped stream (such as `connect`) must be invoked directly.

    Usually the last object in the chain of composition is the concrete socket
    object (for example, a `net::basic_socket` or a class derived from it).
    The function @ref get_lowest_layer exists to easily obtain the concrete
    socket when it is desired to perform an action that is not prescribed by
    a named requirement, such as changing a socket option, cancelling all
    pending asynchronous I/O, or closing the socket (perhaps by using
    @ref close_socket).

    @par Example
    @code
    // Set non-blocking mode on a stack of stream
    // layers with a regular socket at the lowest layer.
    template <class Stream>
    void set_non_blocking (Stream& stream)
    {
        error_code ec;
        // A compile error here means your lowest layer is not the right type!
        get_lowest_layer(stream).non_blocking(true, ec);
        if(ec)
            throw system_error{ec};
    }
    @endcode

    @param t The layer in a stack of layered objects for which the lowest layer is returned.

    @see close_socket, lowest_layer_type
*/
template<class T>
lowest_layer_type<T>&
get_lowest_layer(T& t) noexcept
{
    return detail::get_lowest_layer_impl(
        t, detail::has_next_layer<T>{});
}

//------------------------------------------------------------------------------

/** A trait to determine the return type of get_executor.

    This type alias will be the type of values returned by
    by calling member `get_exector` on an object of type `T&`.

    @param T The type to query

    @return The type of values returned from `get_executor`.
*/
// Workaround for ICE on gcc 4.8
#if BOOST_BEAST_DOXYGEN
template<class T>
using executor_type = __see_below__;
#elif BOOST_WORKAROUND(BOOST_GCC, < 40900)
template<class T>
using executor_type =
    typename std::decay<T>::type::executor_type;
#else
template<class T>
using executor_type =
    decltype(std::declval<T&>().get_executor());
#endif

/** Determine if `T` has the `get_executor` member function.

    Metafunctions are used to perform compile time checking of template
    types. This type will be `std::true_type` if `T` has the member
    function with the correct signature, else type will be `std::false_type`. 

    @par Example

    Use with tag dispatching:

    @code
    template<class T>
    void maybe_hello(T const& t, std::true_type)
    {
        net::post(
            t.get_executor(),
            []
            {
                std::cout << "Hello, world!" << std::endl;
            });
    }

    template<class T>
    void maybe_hello(T const&, std::false_type)
    {
        // T does not have get_executor
    }

    template<class T>
    void maybe_hello(T const& t)
    {
        maybe_hello(t, has_get_executor<T>{});
    }
    @endcode

    Use with `static_assert`:

    @code
    struct stream
    {
        using executor_type = net::io_context::executor_type;
        executor_type get_executor() noexcept;
    };

    static_assert(has_get_executor<stream>::value, "Missing get_executor member");
    @endcode
*/
#if BOOST_BEAST_DOXYGEN
template<class T>
using has_get_executor = __see_below__;
#else
template<class T, class = void>
struct has_get_executor : std::false_type {};

template<class T>
struct has_get_executor<T, boost::void_t<decltype(
    std::declval<T&>().get_executor())>> : std::true_type {};
#endif

//------------------------------------------------------------------------------

/** Determine if at type meets the requirements of <em>SyncReadStream</em>.

    Metafunctions are used to perform compile time checking of template
    types. This type will be `std::true_type` if `T` meets the requirements,
    else the type will be `std::false_type`. 

    @par Example
    Use with `static_assert`:
    @code
    template<class SyncReadStream>
    void f(SyncReadStream& stream)
    {
        static_assert(is_sync_read_stream<SyncReadStream>::value,
            "SyncReadStream type requirements not met");
    ...
    @endcode

    Use with `std::enable_if` (SFINAE):
    @code
    template<class SyncReadStream>
    typename std::enable_if<is_sync_read_stream<SyncReadStream>::value>::type
    f(SyncReadStream& stream);
    @endcode
*/
#if BOOST_BEAST_DOXYGEN
template<class T>
using is_sync_read_stream = __see_below__;
#else
template<class T, class = void>
struct is_sync_read_stream : std::false_type {};

template<class T>
struct is_sync_read_stream<T, boost::void_t<decltype(
    std::declval<std::size_t&>() = std::declval<T&>().read_some(
        std::declval<detail::MutableBufferSequence>()),
    std::declval<std::size_t&>() = std::declval<T&>().read_some(
        std::declval<detail::MutableBufferSequence>(),
        std::declval<boost::system::error_code&>())
            )>> : std::true_type {};
#endif

/** Determine if `T` meets the requirements of <em>SyncWriteStream</em>.

    Metafunctions are used to perform compile time checking of template
    types. This type will be `std::true_type` if `T` meets the requirements,
    else the type will be `std::false_type`. 

    @par Example

    Use with `static_assert`:

    @code
    template<class SyncReadStream>
    void f(SyncReadStream& stream)
    {
        static_assert(is_sync_read_stream<SyncReadStream>::value,
            "SyncReadStream type requirements not met");
    ...
    @endcode

    Use with `std::enable_if` (SFINAE):

    @code
    template<class SyncReadStream>
    typename std::enable_if<is_sync_read_stream<SyncReadStream>::value>::type
    f(SyncReadStream& stream);
    @endcode
*/
#if BOOST_BEAST_DOXYGEN
template<class T>
using is_sync_write_stream = __see_below__;
#else
template<class T, class = void>
struct is_sync_write_stream : std::false_type {};

template<class T>
struct is_sync_write_stream<T, boost::void_t<decltype(
    (
    std::declval<std::size_t&>() = std::declval<T&>().write_some(
        std::declval<detail::ConstBufferSequence>()))
    ,std::declval<std::size_t&>() = std::declval<T&>().write_some(
        std::declval<detail::ConstBufferSequence>(),
        std::declval<boost::system::error_code&>())
            )>> : std::true_type {};
#endif

/** Determine if `T` meets the requirements of @b SyncStream.

    Metafunctions are used to perform compile time checking of template
    types. This type will be `std::true_type` if `T` meets the requirements,
    else the type will be `std::false_type`. 

    @par Example

    Use with `static_assert`:

    @code
    template<class SyncStream>
    void f(SyncStream& stream)
    {
        static_assert(is_sync_stream<SyncStream>::value,
            "SyncStream type requirements not met");
    ...
    @endcode

    Use with `std::enable_if` (SFINAE):

    @code
    template<class SyncStream>
    typename std::enable_if<is_sync_stream<SyncStream>::value>::type
    f(SyncStream& stream);
    @endcode
*/
#if BOOST_BEAST_DOXYGEN
template<class T>
using is_sync_stream = __see_below__;
#else
template<class T>
using is_sync_stream = std::integral_constant<bool,
    is_sync_read_stream<T>::value && is_sync_write_stream<T>::value>;
#endif

//------------------------------------------------------------------------------

/** Determine if `T` meets the requirements of <em>AsyncReadStream</em>.

    Metafunctions are used to perform compile time checking of template
    types. This type will be `std::true_type` if `T` meets the requirements,
    else the type will be `std::false_type`. 

    @par Example
    
    Use with `static_assert`:
    
    @code
    template<class AsyncReadStream>
    void f(AsyncReadStream& stream)
    {
        static_assert(is_async_read_stream<AsyncReadStream>::value,
            "AsyncReadStream type requirements not met");
    ...
    @endcode
    
    Use with `std::enable_if` (SFINAE):
    
    @code
        template<class AsyncReadStream>
        typename std::enable_if<is_async_read_stream<AsyncReadStream>::value>::type
        f(AsyncReadStream& stream);
    @endcode
*/
#if BOOST_BEAST_DOXYGEN
template<class T>
using is_async_read_stream = __see_below__;
#else
template<class T, class = void>
struct is_async_read_stream : std::false_type {};

template<class T>
struct is_async_read_stream<T, boost::void_t<decltype(
    std::declval<T&>().async_read_some(
        std::declval<detail::MutableBufferSequence>(),
        std::declval<detail::ReadHandler>())
            )>> : std::integral_constant<bool,
    has_get_executor<T>::value
        > {};
#endif

/** Determine if `T` meets the requirements of <em>AsyncWriteStream</em>.

    Metafunctions are used to perform compile time checking of template
    types. This type will be `std::true_type` if `T` meets the requirements,
    else the type will be `std::false_type`. 

    @par Example

    Use with `static_assert`:

    @code
    template<class AsyncWriteStream>
    void f(AsyncWriteStream& stream)
    {
        static_assert(is_async_write_stream<AsyncWriteStream>::value,
            "AsyncWriteStream type requirements not met");
    ...
    @endcode

    Use with `std::enable_if` (SFINAE):

    @code
    template<class AsyncWriteStream>
    typename std::enable_if<is_async_write_stream<AsyncWriteStream>::value>::type
    f(AsyncWriteStream& stream);
    @endcode
*/
#if BOOST_BEAST_DOXYGEN
template<class T>
using is_async_write_stream = __see_below__;
#else
template<class T, class = void>
struct is_async_write_stream : std::false_type {};

template<class T>
struct is_async_write_stream<T, boost::void_t<decltype(
    std::declval<T&>().async_write_some(
        std::declval<detail::ConstBufferSequence>(),
        std::declval<detail::WriteHandler>())
            )>> : std::integral_constant<bool,
    has_get_executor<T>::value
        > {};
#endif

/** Determine if `T` meets the requirements of @b AsyncStream.

    Metafunctions are used to perform compile time checking of template
    types. This type will be `std::true_type` if `T` meets the requirements,
    else the type will be `std::false_type`. 

    @par Example

    Use with `static_assert`:

    @code
    template<class AsyncStream>
    void f(AsyncStream& stream)
    {
        static_assert(is_async_stream<AsyncStream>::value,
            "AsyncStream type requirements not met");
    ...
    @endcode

    Use with `std::enable_if` (SFINAE):

    @code
    template<class AsyncStream>
    typename std::enable_if<is_async_stream<AsyncStream>::value>::type
    f(AsyncStream& stream);
    @endcode
*/
#if BOOST_BEAST_DOXYGEN
template<class T>
using is_async_stream = __see_below__;
#else
template<class T>
using is_async_stream = std::integral_constant<bool,
    is_async_read_stream<T>::value && is_async_write_stream<T>::value>;
#endif

//------------------------------------------------------------------------------

/** Default socket close function.

    This function is not meant to be called directly. Instead, it
    is called automatically when using @ref close_socket. To enable
    closure of user-defined types or classes derived from a particular
    user-defined type, this function should be overloaded in the
    corresponding namespace for the type in question.

    @see close_socket
*/
template<
    class Protocol,
    class Executor>
void
beast_close_socket(
    net::basic_socket<
        Protocol, Executor>& sock)
{
    boost::system::error_code ec;
    sock.close(ec);
}

namespace detail {

struct close_socket_impl
{
    template<class T>
    void
    operator()(T& t) const
    {
        using beast::beast_close_socket;
        beast_close_socket(t);
    }
};

} // detail

/** Close a socket or socket-like object.

    This function attempts to close an object representing a socket.
    In this context, a socket is an object for which an unqualified
    call to the function `void beast_close_socket(Socket&)` is
    well-defined. The function `beast_close_socket` is a
    <em>customization point</em>, allowing user-defined types to
    provide an algorithm for performing the close operation by
    overloading this function for the type in question.

    Since the customization point is a function call, the normal
    rules for finding the correct overload are applied including
    the rules for argument-dependent lookup ("ADL"). This permits
    classes derived from a type for which a customization is provided
    to inherit the customization point.

    An overload for the networking class template `net::basic_socket`
    is provided, which implements the close algorithm for all socket-like
    objects (hence the name of this customization point). When used
    in conjunction with @ref get_lowest_layer, a generic algorithm
    operating on a layered stream can perform a closure of the underlying
    socket without knowing the exact list of concrete types.

    @par Example 1
    The following generic function synchronously sends a message
    on the stream, then closes the socket.
    @code
    template <class WriteStream>
    void hello_and_close (WriteStream& stream)
    {
        net::write(stream, net::const_buffer("Hello, world!", 13));
        close_socket(get_lowest_layer(stream));
    }
    @endcode

    To enable closure of user defined types, it is necessary to provide
    an overload of the function `beast_close_socket` for the type.

    @par Example 2
    The following code declares a user-defined type which contains a
    private socket, and provides an overload of the customization
    point which closes the private socket.
    @code
    class my_socket
    {
        net::ip::tcp::socket sock_;

    public:
        my_socket(net::io_context& ioc)
            : sock_(ioc)
        {
        }

        friend void beast_close_socket(my_socket& s)
        {
            error_code ec;
            s.sock_.close(ec);
            // ignore the error
        }
    };
    @endcode

    @param sock The socket to close. If the customization point is not
    defined for the type of this object, or one of its base classes,
    then a compiler error results.

    @see beast_close_socket
*/
#if BOOST_BEAST_DOXYGEN
template<class Socket>
void
close_socket(Socket& sock);
#else
BOOST_BEAST_INLINE_VARIABLE(close_socket, detail::close_socket_impl)
#endif

} // beast
} // boost

#endif
