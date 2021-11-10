//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_STREAM_BASE_HPP
#define BOOST_BEAST_WEBSOCKET_STREAM_BASE_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/websocket/detail/decorator.hpp>
#include <boost/beast/core/role.hpp>
#include <chrono>
#include <type_traits>

namespace boost {
namespace beast {
namespace websocket {

/** This class is used as a base for the @ref websocket::stream class template to group common types and constants.
*/
struct stream_base
{
    /// The type used to represent durations
    using duration =
        std::chrono::steady_clock::duration;

    /// The type used to represent time points
    using time_point =
        std::chrono::steady_clock::time_point;

    /// Returns the special time_point value meaning "never"
    static
    time_point
    never() noexcept
    {
        return (time_point::max)();
    }

    /// Returns the special duration value meaning "none"
    static
    duration
    none() noexcept
    {
        return (duration::max)();
    }

    /** Stream option used to adjust HTTP fields of WebSocket upgrade request and responses.
    */
    class decorator
    {
        detail::decorator d_;

        template<class, bool>
        friend class stream;

    public:
        // Move Constructor
        decorator(decorator&&) = default;

        /** Construct a decorator option.
            
            @param f An invocable function object. Ownership of
            the function object is transferred by decay-copy.
        */
        template<class Decorator
#ifndef BOOST_BEAST_DOXYGEN
            ,class = typename std::enable_if<
                detail::is_decorator<
                    Decorator>::value>::type
#endif
        >
        explicit
        decorator(Decorator&& f)
            : d_(std::forward<Decorator>(f))
        {
        }
    };

    /** Stream option to control the behavior of websocket timeouts.

        Timeout features are available for asynchronous operations only.
    */
    struct timeout
    {
        /** Time limit on handshake, accept, and close operations:

            This value whether or not there is a time limit, and the
            duration of that time limit, for asynchronous handshake,
            accept, and close operations. If this is equal to the
            value @ref none then there will be no time limit. Otherwise,
            if any of the applicable operations takes longer than this
            amount of time, the operation will be canceled and a
            timeout error delivered to the completion handler.
        */
        duration handshake_timeout;

        /** The time limit after which a connection is considered idle.
        */
        duration idle_timeout;

        /** Automatic ping setting.

            If the idle interval is set, this setting affects the
            behavior of the stream when no data is received for the
            timeout interval as follows:

            @li When `keep_alive_pings` is `true`, an idle ping will be
            sent automatically. If another timeout interval elapses
            with no received data then the connection will be closed.
            An outstanding read operation must be pending, which will
            complete immediately the error @ref beast::error::timeout.

            @li When `keep_alive_pings` is `false`, the connection will be closed.
            An outstanding read operation must be pending, which will
            complete immediately the error @ref beast::error::timeout.
        */
        bool keep_alive_pings;

        /** Construct timeout settings with suggested values for a role.

            This constructs the timeout settings with a predefined set
            of values which varies depending on the desired role. The
            values are selected upon construction, regardless of the
            current or actual role in use on the stream.

            @par Example
            This statement sets the timeout settings of the stream to
            the suggested values for the server role:
            @code
            @endcode

            @param role The role of the websocket stream
            (@ref role_type::client or @ref role_type::server).
        */
        static
        timeout
        suggested(role_type role) noexcept
        {
            timeout opt{};
            switch(role)
            {
            case role_type::client:
                opt.handshake_timeout = std::chrono::seconds(30);
                opt.idle_timeout = none();
                opt.keep_alive_pings = false;
                break;

            case role_type::server:
                opt.handshake_timeout = std::chrono::seconds(30);
                opt.idle_timeout = std::chrono::seconds(300);
                opt.keep_alive_pings = true;
                break;
            }
            return opt;
        }
    };

protected:
    enum class status
    {
        //none,
        handshake,
        open,
        closing,
        closed,
        failed // VFALCO Is this needed?
    };
};

} // websocket
} // beast
} // boost

#endif
