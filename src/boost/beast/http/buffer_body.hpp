//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_BUFFER_BODY_HPP
#define BOOST_BEAST_HTTP_BUFFER_BODY_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/type_traits.hpp>
#include <boost/optional.hpp>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {
namespace http {

/** A <em>Body</em> using a caller provided buffer

    Messages using this body type may be serialized and parsed.
    To use this class, the caller must initialize the members
    of @ref buffer_body::value_type to appropriate values before
    each call to read or write during a stream operation.
*/
struct buffer_body
{
    /// The type of the body member when used in a message.
    struct value_type
    {
        /** A pointer to a contiguous area of memory of @ref size octets, else `nullptr`.

            @par When Serializing

            If this is `nullptr` and `more` is `true`, the error
            @ref error::need_buffer will be returned from @ref serializer::get
            Otherwise, the serializer will use the memory pointed to
            by `data` having `size` octets of valid storage as the
            next buffer representing the body.

            @par When Parsing

            If this is `nullptr`, the error @ref error::need_buffer
            will be returned from @ref parser::put. Otherwise, the
            parser will store body octets into the memory pointed to
            by `data` having `size` octets of valid storage. After
            octets are stored, the `data` and `size` members are
            adjusted: `data` is incremented to point to the next
            octet after the data written, while `size` is decremented
            to reflect the remaining space at the memory location
            pointed to by `data`.
        */
        void* data = nullptr;

        /** The number of octets in the buffer pointed to by @ref data.

            @par When Serializing

            If `data` is `nullptr` during serialization, this value
            is ignored. Otherwise, it represents the number of valid
            body octets pointed to by `data`.

            @par When Parsing

            The value of this field will be decremented during parsing
            to indicate the number of remaining free octets in the
            buffer pointed to by `data`. When it reaches zero, the
            parser will return @ref error::need_buffer, indicating to
            the caller that the values of `data` and `size` should be
            updated to point to a new memory buffer.
        */
        std::size_t size = 0;

        /** `true` if this is not the last buffer.

            @par When Serializing
            
            If this is `true` and `data` is `nullptr`, the error
            @ref error::need_buffer will be returned from @ref serializer::get

            @par When Parsing

            This field is not used during parsing.
        */
        bool more = true;
    };

    /** The algorithm for parsing the body

        Meets the requirements of <em>BodyReader</em>.
    */
#if BOOST_BEAST_DOXYGEN
    using reader = __implementation_defined__;
#else
    class reader
    {
        value_type& body_;

    public:
        template<bool isRequest, class Fields>
        explicit
        reader(header<isRequest, Fields>&, value_type& b)
            : body_(b)
        {
        }

        void
        init(boost::optional<std::uint64_t> const&, error_code& ec)
        {
            ec = {};
        }

        template<class ConstBufferSequence>
        std::size_t
        put(ConstBufferSequence const& buffers,
            error_code& ec)
        {
            if(! body_.data)
            {
                ec = error::need_buffer;
                return 0;
            }
            auto const bytes_transferred =
                net::buffer_copy(net::buffer(
                    body_.data, body_.size), buffers);
            body_.data = static_cast<char*>(
                body_.data) + bytes_transferred;
            body_.size -= bytes_transferred;
            if(bytes_transferred == buffer_bytes(buffers))
                ec = {};
            else
                ec = error::need_buffer;
            return bytes_transferred;
        }

        void
        finish(error_code& ec)
        {
            ec = {};
        }
    };
#endif

    /** The algorithm for serializing the body

        Meets the requirements of <em>BodyWriter</em>.
    */
#if BOOST_BEAST_DOXYGEN
    using writer = __implementation_defined__;
#else
    class writer
    {
        bool toggle_ = false;
        value_type const& body_;

    public:
        using const_buffers_type =
            net::const_buffer;

        template<bool isRequest, class Fields>
        explicit
        writer(header<isRequest, Fields> const&, value_type const& b)
            : body_(b)
        {
        }

        void
        init(error_code& ec)
        {
            ec = {};
        }

        boost::optional<
            std::pair<const_buffers_type, bool>>
        get(error_code& ec)
        {
            if(toggle_)
            {
                if(body_.more)
                {
                    toggle_ = false;
                    ec = error::need_buffer;
                }
                else
                {
                    ec = {};
                }
                return boost::none;
            }
            if(body_.data)
            {
                ec = {};
                toggle_ = true;
                return {{const_buffers_type{
                    body_.data, body_.size}, body_.more}};
            }
            if(body_.more)
                ec = error::need_buffer;
            else
                ec = {};
            return boost::none;
        }
    };
#endif
};

#if ! BOOST_BEAST_DOXYGEN
// operator<< is not supported for buffer_body
template<bool isRequest, class Fields>
std::ostream&
operator<<(std::ostream& os, message<isRequest,
    buffer_body, Fields> const& msg) = delete;
#endif

} // http
} // beast
} // boost

#endif
