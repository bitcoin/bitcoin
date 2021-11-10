//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_BASIC_PARSER_HPP
#define BOOST_BEAST_HTTP_BASIC_PARSER_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/detail/basic_parser.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/optional.hpp>
#include <boost/assert.hpp>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {
namespace http {

/** A parser for decoding HTTP/1 wire format messages.

    This parser is designed to efficiently parse messages in the
    HTTP/1 wire format. It allocates no memory when input is
    presented as a single contiguous buffer, and uses minimal
    state. It will handle chunked encoding and it understands
    the semantics of the Connection, Content-Length, and Upgrade
    fields.
    The parser is optimized for the case where the input buffer
    sequence consists of a single contiguous buffer. The
    @ref beast::basic_flat_buffer class is provided, which guarantees
    that the input sequence of the stream buffer will be represented
    by exactly one contiguous buffer. To ensure the optimum performance
    of the parser, use @ref beast::basic_flat_buffer with HTTP algorithms
    such as @ref read, @ref read_some, @ref async_read, and @ref async_read_some.
    Alternatively, the caller may use custom techniques to ensure that
    the structured portion of the HTTP message (header or chunk header)
    is contained in a linear buffer.

    The interface to the parser uses virtual member functions.
    To use this class, derive your type from @ref basic_parser. When
    bytes are presented, the implementation will make a series of zero
    or more calls to virtual functions, which the derived class must
    implement.

    Every virtual function must be provided by the derived class,
    or else a compilation error will be generated. The implementation
    will make sure that `ec` is clear before each virtual function
    is invoked. If a virtual function sets an error, it is propagated
    out of the parser to the caller.

    @tparam isRequest A `bool` indicating whether the parser will be
    presented with request or response message.

    @note If the parser encounters a field value with obs-fold
    longer than 4 kilobytes in length, an error is generated.
*/
template<bool isRequest>
class basic_parser
    : private detail::basic_parser_base
{
    boost::optional<std::uint64_t>
        body_limit_ =
            boost::optional<std::uint64_t>(
                default_body_limit(is_request{}));   // max payload body
    std::uint64_t len_ = 0;                 // size of chunk or body
    std::uint64_t len0_ = 0;                // content length if known
    std::unique_ptr<char[]> buf_;           // temp storage
    std::size_t buf_len_ = 0;               // size of buf_
    std::size_t skip_ = 0;                  // resume search here
    std::uint32_t header_limit_ = 8192;     // max header size
    unsigned short status_ = 0;             // response status
    state state_ = state::nothing_yet;      // initial state
    unsigned f_ = 0;                        // flags

    // limit on the size of the stack flat buffer
    static std::size_t constexpr max_stack_buffer = 8192;

    // Message will be complete after reading header
    static unsigned constexpr flagSkipBody              = 1<<  0;

    // Consume input buffers across semantic boundaries
    static unsigned constexpr flagEager                 = 1<<  1;

    // The parser has read at least one byte
    static unsigned constexpr flagGotSome               = 1<<  2;

    // Message semantics indicate a body is expected.
    // cleared if flagSkipBody set
    //
    static unsigned constexpr flagHasBody               = 1<<  3;

    static unsigned constexpr flagHTTP11                = 1<<  4;
    static unsigned constexpr flagNeedEOF               = 1<<  5;
    static unsigned constexpr flagExpectCRLF            = 1<<  6;
    static unsigned constexpr flagConnectionClose       = 1<<  7;
    static unsigned constexpr flagConnectionUpgrade     = 1<<  8;
    static unsigned constexpr flagConnectionKeepAlive   = 1<<  9;
    static unsigned constexpr flagContentLength         = 1<< 10;
    static unsigned constexpr flagChunked               = 1<< 11;
    static unsigned constexpr flagUpgrade               = 1<< 12;
    static unsigned constexpr flagFinalChunk            = 1<< 13;

    static constexpr
    std::uint64_t
    default_body_limit(std::true_type)
    {
        // limit for requests
        return 1 * 1024 * 1024; // 1MB
    }

    static constexpr
    std::uint64_t
    default_body_limit(std::false_type)
    {
        // limit for responses
        return 8 * 1024 * 1024; // 8MB
    }

    template<bool OtherIsRequest>
    friend class basic_parser;

    friend class basic_parser_test;

protected:
    /// Default constructor
    basic_parser() = default;

    /** Move constructor

        @note

        After the move, the only valid operation on the
        moved-from object is destruction.
    */
    basic_parser(basic_parser &&) = default;

    /// Move assignment
    basic_parser& operator=(basic_parser &&) = default;

public:
    /// `true` if this parser parses requests, `false` for responses.
    using is_request =
        std::integral_constant<bool, isRequest>;

    /// Destructor
    virtual ~basic_parser() = default;

    /// Copy constructor
    basic_parser(basic_parser const&) = delete;

    /// Copy assignment
    basic_parser& operator=(basic_parser const&) = delete;

    /// Returns `true` if the parser has received at least one byte of input.
    bool
    got_some() const
    {
        return state_ != state::nothing_yet;
    }

    /** Returns `true` if the message is complete.

        The message is complete after the full header is prduced
        and one of the following is true:

        @li The skip body option was set.

        @li The semantics of the message indicate there is no body.

        @li The semantics of the message indicate a body is expected,
        and the entire body was parsed.
    */
    bool
    is_done() const
    {
        return state_ == state::complete;
    }

    /** Returns `true` if a the parser has produced the full header.
    */
    bool
    is_header_done() const
    {
        return state_ > state::fields;
    }

    /** Returns `true` if the message is an upgrade message.

        @note The return value is undefined unless
        @ref is_header_done would return `true`.
    */
    bool
    upgrade() const
    {
        return (f_ & flagConnectionUpgrade) != 0;
    }

    /** Returns `true` if the last value for Transfer-Encoding is "chunked".

        @note The return value is undefined unless
        @ref is_header_done would return `true`.
    */
    bool
    chunked() const
    {
        return (f_ & flagChunked) != 0;
    }

    /** Returns `true` if the message has keep-alive connection semantics.

        This function always returns `false` if @ref need_eof would return
        `false`.

        @note The return value is undefined unless
        @ref is_header_done would return `true`.
    */
    bool
    keep_alive() const;

    /** Returns the optional value of Content-Length if known.

        @note The return value is undefined unless
        @ref is_header_done would return `true`.
    */
    boost::optional<std::uint64_t>
    content_length() const;

    /** Returns the remaining content length if known

        If the message header specifies a Content-Length,
        the return value will be the number of bytes remaining
        in the payload body have not yet been parsed.

        @note The return value is undefined unless
              @ref is_header_done would return `true`.
    */
    boost::optional<std::uint64_t>
    content_length_remaining() const;

    /** Returns `true` if the message semantics require an end of file.

        Depending on the contents of the header, the parser may
        require and end of file notification to know where the end
        of the body lies. If this function returns `true` it will be
        necessary to call @ref put_eof when there will never be additional
        data from the input.
    */
    bool
    need_eof() const
    {
        return (f_ & flagNeedEOF) != 0;
    }

    /** Set the limit on the payload body.

        This function sets the maximum allowed size of the payload body,
        before any encodings except chunked have been removed. Depending
        on the message semantics, one of these cases will apply:

        @li The Content-Length is specified and exceeds the limit. In
        this case the result @ref error::body_limit is returned
        immediately after the header is parsed.

        @li The Content-Length is unspecified and the chunked encoding
        is not specified as the last encoding. In this case the end of
        message is determined by the end of file indicator on the
        associated stream or input source. If a sufficient number of
        body payload octets are presented to the parser to exceed the
        configured limit, the parse fails with the result
        @ref error::body_limit

        @li The Transfer-Encoding specifies the chunked encoding as the
        last encoding. In this case, when the number of payload body
        octets produced by removing the chunked encoding  exceeds
        the configured limit, the parse fails with the result
        @ref error::body_limit.
        
        Setting the limit after any body octets have been parsed
        results in undefined behavior.

        The default limit is 1MB for requests and 8MB for responses.

        @param v An optional integral value representing the body limit.
        If this is equal to `boost::none`, then the body limit is disabled.
    */
    void
    body_limit(boost::optional<std::uint64_t> v)
    {
        body_limit_ = v;
    }

    /** Set a limit on the total size of the header.

        This function sets the maximum allowed size of the header
        including all field name, value, and delimiter characters
        and also including the CRLF sequences in the serialized
        input. If the end of the header is not found within the
        limit of the header size, the error @ref error::header_limit
        is returned by @ref put.

        Setting the limit after any header octets have been parsed
        results in undefined behavior.
    */
    void
    header_limit(std::uint32_t v)
    {
        header_limit_ = v;
    }

    /// Returns `true` if the eager parse option is set.
    bool
    eager() const
    {
        return (f_ & flagEager) != 0;
    }

    /** Set the eager parse option.

        Normally the parser returns after successfully parsing a structured
        element (header, chunk header, or chunk body) even if there are octets
        remaining in the input. This is necessary when attempting to parse the
        header first, or when the caller wants to inspect information which may
        be invalidated by subsequent parsing, such as a chunk extension. The
        `eager` option controls whether the parser keeps going after parsing
        structured element if there are octets remaining in the buffer and no
        error occurs. This option is automatically set or cleared during certain
        stream operations to improve performance with no change in functionality.

        The default setting is `false`.

        @param v `true` to set the eager parse option or `false` to disable it.
    */
    void
    eager(bool v)
    {
        if(v)
            f_ |= flagEager;
        else
            f_ &= ~flagEager;
    }

    /// Returns `true` if the skip parse option is set.
    bool
    skip() const
    {
        return (f_ & flagSkipBody) != 0;
    }

    /** Set the skip parse option.

        This option controls whether or not the parser expects to see an HTTP
        body, regardless of the presence or absence of certain fields such as
        Content-Length or a chunked Transfer-Encoding. Depending on the request,
        some responses do not carry a body. For example, a 200 response to a
        CONNECT request from a tunneling proxy, or a response to a HEAD request.
        In these cases, callers may use this function inform the parser that
        no body is expected. The parser will consider the message complete
        after the header has been received.

        @param v `true` to set the skip body option or `false` to disable it.

        @note This function must called before any bytes are processed.
    */
    void
    skip(bool v);

    /** Write a buffer sequence to the parser.

        This function attempts to incrementally parse the HTTP
        message data stored in the caller provided buffers. Upon
        success, a positive return value indicates that the parser
        made forward progress, consuming that number of
        bytes.

        In some cases there may be an insufficient number of octets
        in the input buffer in order to make forward progress. This
        is indicated by the code @ref error::need_more. When
        this happens, the caller should place additional bytes into
        the buffer sequence and call @ref put again.

        The error code @ref error::need_more is special. When this
        error is returned, a subsequent call to @ref put may succeed
        if the buffers have been updated. Otherwise, upon error
        the parser may not be restarted.

        @param buffers An object meeting the requirements of
        <em>ConstBufferSequence</em> that represents the next chunk of
        message data. If the length of this buffer sequence is
        one, the implementation will not allocate additional memory.
        The class @ref beast::basic_flat_buffer is provided as one way to
        meet this requirement

        @param ec Set to the error, if any occurred.

        @return The number of octets consumed in the buffer
        sequence. The caller should remove these octets even if the
        error is set.
    */
    template<class ConstBufferSequence>
    std::size_t
    put(ConstBufferSequence const& buffers, error_code& ec);

#if ! BOOST_BEAST_DOXYGEN
    std::size_t
    put(net::const_buffer buffer,
        error_code& ec);
#endif

    /** Inform the parser that the end of stream was reached.

        In certain cases, HTTP needs to know where the end of
        the stream is. For example, sometimes servers send
        responses without Content-Length and expect the client
        to consume input (for the body) until EOF. Callbacks
        and errors will still be processed as usual.

        This is typically called when a read from the
        underlying stream object sets the error code to
        `net::error::eof`.

        @note Only valid after parsing a complete header.

        @param ec Set to the error, if any occurred. 
    */
    void
    put_eof(error_code& ec);

protected:
    /** Called after receiving the request-line.

        This virtual function is invoked after receiving a request-line
        when parsing HTTP requests.
        It can only be called when `isRequest == true`.

        @param method The verb enumeration. If the method string is not
        one of the predefined strings, this value will be @ref verb::unknown.

        @param method_str The unmodified string representing the verb.

        @param target The request-target.

        @param version The HTTP-version. This will be 10 for HTTP/1.0,
        and 11 for HTTP/1.1.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.
    */
    virtual
    void
    on_request_impl(
        verb method,
        string_view method_str,
        string_view target,
        int version,
        error_code& ec) = 0;

    /** Called after receiving the status-line.

        This virtual function is invoked after receiving a status-line
        when parsing HTTP responses.
        It can only be called when `isRequest == false`.

        @param code The numeric status code.

        @param reason The reason-phrase. Note that this value is
        now obsolete, and only provided for historical or diagnostic
        purposes.

        @param version The HTTP-version. This will be 10 for HTTP/1.0,
        and 11 for HTTP/1.1.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.
    */
    virtual
    void
    on_response_impl(
        int code,
        string_view reason,
        int version,
        error_code& ec) = 0;

    /** Called once for each complete field in the HTTP header.

        This virtual function is invoked for each field that is received
        while parsing an HTTP message.

        @param name The known field enum value. If the name of the field
        is not recognized, this value will be @ref field::unknown.

        @param name_string The exact name of the field as received from
        the input, represented as a string.

        @param value A string holding the value of the field.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.
    */
    virtual
    void
    on_field_impl(
        field name,
        string_view name_string,
        string_view value,
        error_code& ec) = 0;

    /** Called once after the complete HTTP header is received.

        This virtual function is invoked once, after the complete HTTP
        header is received while parsing a message.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.
    */
    virtual
    void
    on_header_impl(error_code& ec) = 0;

    /** Called once before the body is processed.

        This virtual function is invoked once, before the content body is
        processed (but after the complete header is received).

        @param content_length A value representing the content length in
        bytes if the length is known (this can include a zero length).
        Otherwise, the value will be `boost::none`.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.
    */
    virtual
    void
    on_body_init_impl(
        boost::optional<std::uint64_t> const& content_length,
        error_code& ec) = 0;

    /** Called each time additional data is received representing the content body.

        This virtual function is invoked for each piece of the body which is
        received while parsing of a message. This function is only used when
        no chunked transfer encoding is present.

        @param body A string holding the additional body contents. This may
        contain nulls or unprintable characters.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.

        @see on_chunk_body_impl
    */
    virtual
    std::size_t
    on_body_impl(
        string_view body,
        error_code& ec) = 0;

    /** Called each time a new chunk header of a chunk encoded body is received.

        This function is invoked each time a new chunk header is received.
        The function is only used when the chunked transfer encoding is present.

        @param size The size of this chunk, in bytes.

        @param extensions A string containing the entire chunk extensions.
        This may be empty, indicating no extensions are present.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.
    */
    virtual
    void
    on_chunk_header_impl(
        std::uint64_t size,
        string_view extensions,
        error_code& ec) = 0;

    /** Called each time additional data is received representing part of a body chunk.

        This virtual function is invoked for each piece of the body which is
        received while parsing of a message. This function is only used when
        no chunked transfer encoding is present.

        @param remain The number of bytes remaining in this chunk. This includes
        the contents of passed `body`. If this value is zero, then this represents
        the final chunk.

        @param body A string holding the additional body contents. This may
        contain nulls or unprintable characters.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.

        @return This function should return the number of bytes actually consumed
        from the `body` value. Any bytes that are not consumed on this call
        will be presented in a subsequent call.

        @see on_body_impl
    */
    virtual
    std::size_t
    on_chunk_body_impl(
        std::uint64_t remain,
        string_view body,
        error_code& ec) = 0;

    /** Called once when the complete message is received.

        This virtual function is invoked once, after successfully parsing
        a complete HTTP message.

        @param ec An output parameter which the function may set to indicate
        an error. The error will be clear before this function is invoked.
    */
    virtual
    void
    on_finish_impl(error_code& ec) = 0;

private:

    boost::optional<std::uint64_t>
    content_length_unchecked() const;

    template<class ConstBufferSequence>
    std::size_t
    put_from_stack(
        std::size_t size,
        ConstBufferSequence const& buffers,
        error_code& ec);

    void
    maybe_need_more(
        char const* p, std::size_t n,
            error_code& ec);

    void
    parse_start_line(
        char const*& p, char const* last,
            error_code& ec, std::true_type);

    void
    parse_start_line(
        char const*& p, char const* last,
            error_code& ec, std::false_type);

    void
    parse_fields(
        char const*& p, char const* last,
            error_code& ec);

    void
    finish_header(
        error_code& ec, std::true_type);

    void
    finish_header(
        error_code& ec, std::false_type);

    void
    parse_body(char const*& p,
        std::size_t n, error_code& ec);

    void
    parse_body_to_eof(char const*& p,
        std::size_t n, error_code& ec);

    void
    parse_chunk_header(char const*& p,
        std::size_t n, error_code& ec);

    void
    parse_chunk_body(char const*& p,
        std::size_t n, error_code& ec);

    void
    do_field(field f,
        string_view value, error_code& ec);
};

} // http
} // beast
} // boost

#include <boost/beast/http/impl/basic_parser.hpp>
#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/http/impl/basic_parser.ipp>
#endif

#endif
