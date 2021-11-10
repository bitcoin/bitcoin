//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_CHUNK_ENCODE_HPP
#define BOOST_BEAST_HTTP_CHUNK_ENCODE_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/buffers_cat.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/http/type_traits.hpp>
#include <boost/beast/http/detail/chunk_encode.hpp>
#include <boost/asio/buffer.hpp>
#include <memory>
#include <type_traits>

namespace boost {
namespace beast {
namespace http {

/** A chunked encoding crlf

    This implements a <em>ConstBufferSequence</em> holding the CRLF
    (`"\r\n"`) used as a delimiter in a @em chunk.

    To use this class, pass an instance of it to a
    stream algorithm as the buffer sequence:
    @code
        // writes "\r\n"
        net::write(stream, chunk_crlf{});
    @endcode

    @see https://tools.ietf.org/html/rfc7230#section-4.1
*/
struct chunk_crlf
{
    /// Constructor
    chunk_crlf() = default;

    //-----

    /// Required for <em>ConstBufferSequence</em>
#if BOOST_BEAST_DOXYGEN
    using value_type = __implementation_defined__;
#else
    using value_type = net::const_buffer;
#endif

    /// Required for <em>ConstBufferSequence</em>
    using const_iterator = value_type const*;

    /// Required for <em>ConstBufferSequence</em>
    chunk_crlf(chunk_crlf const&) = default;

    /// Required for <em>ConstBufferSequence</em>
    const_iterator
    begin() const
    {
        static net::const_buffer const cb{"\r\n", 2};
        return &cb;
    }

    /// Required for <em>ConstBufferSequence</em>
    const_iterator
    end() const
    {
        return begin() + 1;
    }
};

//------------------------------------------------------------------------------

/** A @em chunk header

    This implements a <em>ConstBufferSequence</em> representing the
    header of a @em chunk. The serialized format is as follows:
    @code
        chunk-header    = 1*HEXDIG chunk-ext CRLF       
        chunk-ext       = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
        chunk-ext-name  = token
        chunk-ext-val   = token / quoted-string
    @endcode
    The chunk extension is optional. After the header and
    chunk body have been serialized, it is the callers
    responsibility to also serialize the final CRLF (`"\r\n"`).

    This class allows the caller to emit piecewise chunk bodies,
    by first serializing the chunk header using this class and then
    serializing the chunk body in a series of one or more calls to
    a stream write operation.

    To use this class, pass an instance of it to a
    stream algorithm as the buffer sequence:
    @code
        // writes "400;x\r\n"
        net::write(stream, chunk_header{1024, "x"});
    @endcode

    @see https://tools.ietf.org/html/rfc7230#section-4.1
*/
class chunk_header
{
    using view_type = buffers_cat_view<
        detail::chunk_size,             // chunk-size
        net::const_buffer,   // chunk-extensions
        chunk_crlf>;                    // CRLF

    std::shared_ptr<
        detail::chunk_extensions> exts_;
    view_type view_;

public:
    /** Constructor

        This constructs a buffer sequence representing a
        @em chunked-body size and terminating CRLF (`"\r\n"`)
        with no chunk extensions.

        @param size The size of the chunk body that follows.
        The value must be greater than zero.

        @see https://tools.ietf.org/html/rfc7230#section-4.1
    */
    explicit
    chunk_header(std::size_t size);

    /** Constructor

        This constructs a buffer sequence representing a
        @em chunked-body size and terminating CRLF (`"\r\n"`)
        with provided chunk extensions.

        @param size The size of the chunk body that follows.
        The value must be greater than zero.

        @param extensions The chunk extensions string. This
        string must be formatted correctly as per rfc7230,
        using this BNF syntax:
        @code
            chunk-ext       = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
            chunk-ext-name  = token
            chunk-ext-val   = token / quoted-string
        @endcode
        The data pointed to by this string view must remain
        valid for the lifetime of any operations performed on
        the object.

        @see https://tools.ietf.org/html/rfc7230#section-4.1.1
    */
    chunk_header(
        std::size_t size,
        string_view extensions);

    /** Constructor

        This constructs a buffer sequence representing a
        @em chunked-body size and terminating CRLF (`"\r\n"`)
        with provided chunk extensions.
        The default allocator is used to provide storage for the
        extensions object.

        @param size The size of the chunk body that follows.
        The value must be greater than zero.

        @param extensions The chunk extensions object. The expression
        `extensions.str()` must be valid, and the return type must
        be convertible to @ref string_view. This object will be copied
        or moved as needed to ensure that the chunk header object retains
        ownership of the buffers provided by the chunk extensions object.

        @note This function participates in overload resolution only
        if @b ChunkExtensions meets the requirements stated above.

        @see https://tools.ietf.org/html/rfc7230#section-4.1
    */
    template<class ChunkExtensions
#if ! BOOST_BEAST_DOXYGEN
        , class = typename std::enable_if<
            detail::is_chunk_extensions<
                ChunkExtensions>::value>::type
#endif
    >
    chunk_header(
        std::size_t size,
        ChunkExtensions&& extensions);

    /** Constructor

        This constructs a buffer sequence representing a
        @em chunked-body size and terminating CRLF (`"\r\n"`)
        with provided chunk extensions.
        The specified allocator is used to provide storage for the
        extensions object.

        @param size The size of the chunk body that follows.
        The value be greater than zero.

        @param extensions The chunk extensions object. The expression
        `extensions.str()` must be valid, and the return type must
        be convertible to @ref string_view. This object will be copied
        or moved as needed to ensure that the chunk header object retains
        ownership of the buffers provided by the chunk extensions object.

        @param allocator The allocator to provide storage for the moved
        or copied extensions object.

        @note This function participates in overload resolution only
        if @b ChunkExtensions meets the requirements stated above.

        @see https://tools.ietf.org/html/rfc7230#section-4.1
    */
    template<class ChunkExtensions, class Allocator
#if ! BOOST_BEAST_DOXYGEN
        , class = typename std::enable_if<
            detail::is_chunk_extensions<
                ChunkExtensions>::value>::type
#endif
    >
    chunk_header(
        std::size_t size,
        ChunkExtensions&& extensions,
        Allocator const& allocator);

    //-----

    /// Required for <em>ConstBufferSequence</em>
#if BOOST_BEAST_DOXYGEN
    using value_type = __implementation_defined__;
#else
    using value_type = typename view_type::value_type;
#endif

    /// Required for <em>ConstBufferSequence</em>
#if BOOST_BEAST_DOXYGEN
    using const_iterator = __implementation_defined__;
#else
    using const_iterator = typename view_type::const_iterator;
#endif

    /// Required for <em>ConstBufferSequence</em>
    chunk_header(chunk_header const&) = default;

    /// Required for <em>ConstBufferSequence</em>
    const_iterator
    begin() const
    {
        return view_.begin();
    }

    /// Required for <em>ConstBufferSequence</em>
    const_iterator
    end() const
    {
        return view_.end();
    }
};

//------------------------------------------------------------------------------

/** A @em chunk

    This implements a <em>ConstBufferSequence</em> representing
    a @em chunk. The serialized format is as follows:
    @code
        chunk           = chunk-size [ chunk-ext ] CRLF chunk-data CRLF
        chunk-size      = 1*HEXDIG
        chunk-ext       = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
        chunk-ext-name  = token
        chunk-ext-val   = token / quoted-string
        chunk-data      = 1*OCTET ; a sequence of chunk-size octets
    @endcode
    The chunk extension is optional. 

    To use this class, pass an instance of it to a
    stream algorithm as the buffer sequence.

    @see https://tools.ietf.org/html/rfc7230#section-4.1
*/
template<class ConstBufferSequence>
class chunk_body
{
    using view_type = buffers_cat_view<
        detail::chunk_size,             // chunk-size
        net::const_buffer,   // chunk-extensions
        chunk_crlf,                     // CRLF
        ConstBufferSequence,            // chunk-body
        chunk_crlf>;                    // CRLF

    std::shared_ptr<
        detail::chunk_extensions> exts_;
    view_type view_;

public:
    /** Constructor

        This constructs buffers representing a complete @em chunk
        with no chunk extensions and having the size and contents
        of the specified buffer sequence.

        @param buffers A buffer sequence representing the chunk
        body. Although the buffers object may be copied as necessary,
        ownership of the underlying memory blocks is retained by the
        caller, which must guarantee that they remain valid while this
        object is in use.

        @see https://tools.ietf.org/html/rfc7230#section-4.1
    */
    explicit
    chunk_body(
        ConstBufferSequence const& buffers);

    /** Constructor

        This constructs buffers representing a complete @em chunk
        with the passed chunk extensions and having the size and
        contents of the specified buffer sequence.

        @param buffers A buffer sequence representing the chunk
        body. Although the buffers object may be copied as necessary,
        ownership of the underlying memory blocks is retained by the
        caller, which must guarantee that they remain valid while this
        object is in use.

        @param extensions The chunk extensions string. This
        string must be formatted correctly as per rfc7230,
        using this BNF syntax:
        @code
            chunk-ext       = *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
            chunk-ext-name  = token
            chunk-ext-val   = token / quoted-string
        @endcode
        The data pointed to by this string view must remain
        valid for the lifetime of any operations performed on
        the object.

        @see https://tools.ietf.org/html/rfc7230#section-4.1.1
    */
    chunk_body(
        ConstBufferSequence const& buffers,
        string_view extensions);

    /** Constructor

        This constructs buffers representing a complete @em chunk
        with the passed chunk extensions and having the size and
        contents of the specified buffer sequence.
        The default allocator is used to provide storage for the
        extensions object.

        @param buffers A buffer sequence representing the chunk
        body. Although the buffers object may be copied as necessary,
        ownership of the underlying memory blocks is retained by the
        caller, which must guarantee that they remain valid while this
        object is in use.

        @param extensions The chunk extensions object. The expression
        `extensions.str()` must be valid, and the return type must
        be convertible to @ref string_view. This object will be copied
        or moved as needed to ensure that the chunk header object retains
        ownership of the buffers provided by the chunk extensions object.

        @note This function participates in overload resolution only
        if @b ChunkExtensions meets the requirements stated above.

        @see https://tools.ietf.org/html/rfc7230#section-4.1
    */
    template<class ChunkExtensions
#if ! BOOST_BEAST_DOXYGEN
        , class = typename std::enable_if<
            ! std::is_convertible<typename std::decay<
                ChunkExtensions>::type, string_view>::value>::type
#endif
    >
    chunk_body(
        ConstBufferSequence const& buffers,
        ChunkExtensions&& extensions);

    /** Constructor

        This constructs buffers representing a complete @em chunk
        with the passed chunk extensions and having the size and
        contents of the specified buffer sequence.
        The specified allocator is used to provide storage for the
        extensions object.

        @param buffers A buffer sequence representing the chunk
        body. Although the buffers object may be copied as necessary,
        ownership of the underlying memory blocks is retained by the
        caller, which must guarantee that they remain valid while this
        object is in use.

        @param extensions The chunk extensions object. The expression
        `extensions.str()` must be valid, and the return type must
        be convertible to @ref string_view. This object will be copied
        or moved as needed to ensure that the chunk header object retains
        ownership of the buffers provided by the chunk extensions object.

        @param allocator The allocator to provide storage for the moved
        or copied extensions object.

        @note This function participates in overload resolution only
        if @b ChunkExtensions meets the requirements stated above.

        @see https://tools.ietf.org/html/rfc7230#section-4.1
    */
    template<class ChunkExtensions, class Allocator
#if ! BOOST_BEAST_DOXYGEN
        , class = typename std::enable_if<
            ! std::is_convertible<typename std::decay<
                ChunkExtensions>::type, string_view>::value>::type
#endif
    >
    chunk_body(
        ConstBufferSequence const& buffers,
        ChunkExtensions&& extensions,
        Allocator const& allocator);

    //-----

    /// Required for <em>ConstBufferSequence</em>
#if BOOST_BEAST_DOXYGEN
    using value_type = __implementation_defined__;
#else
    using value_type = typename view_type::value_type;
#endif

    /// Required for <em>ConstBufferSequence</em>
#if BOOST_BEAST_DOXYGEN
    using const_iterator = __implementation_defined__;
#else
    using const_iterator = typename view_type::const_iterator;
#endif

    /// Required for <em>ConstBufferSequence</em>
    const_iterator
    begin() const
    {
        return view_.begin();
    }

    /// Required for <em>ConstBufferSequence</em>
    const_iterator
    end() const
    {
        return view_.end();
    }
};

//------------------------------------------------------------------------------

/** A chunked-encoding last chunk
*/
template<class Trailer = chunk_crlf>
class chunk_last
{
    static_assert(
        is_fields<Trailer>::value ||
        net::is_const_buffer_sequence<Trailer>::value,
        "Trailer requirements not met");

    using buffers_type = typename
        detail::buffers_or_fields<Trailer>::type;

    using view_type =
        buffers_cat_view<
            detail::chunk_size0,    // "0\r\n"
            buffers_type>;          // Trailer (includes CRLF)

    template<class Allocator>
    buffers_type
    prepare(Trailer const& trailer, Allocator const& alloc);

    buffers_type
    prepare(Trailer const& trailer, std::true_type);

    buffers_type
    prepare(Trailer const& trailer, std::false_type);

    std::shared_ptr<void> sp_;
    view_type view_;

public:
    /** Constructor

        The last chunk will have an empty trailer
    */
    chunk_last();

    /** Constructor

        @param trailer The trailer to use. This may be
        a type meeting the requirements of either Fields
        or ConstBufferSequence. If it is a ConstBufferSequence,
        the trailer must be formatted correctly as per rfc7230
        including a CRLF on its own line to denote the end
        of the trailer.
    */
    explicit
    chunk_last(Trailer const& trailer);

    /** Constructor

        @param trailer The trailer to use. This type must
        meet the requirements of Fields.

        @param allocator The allocator to use for storing temporary
        data associated with the serialized trailer buffers.
    */
#if BOOST_BEAST_DOXYGEN
    template<class Allocator>
    chunk_last(Trailer const& trailer, Allocator const& allocator);
#else
    template<class DeducedTrailer, class Allocator,
        class = typename std::enable_if<
            is_fields<DeducedTrailer>::value>::type>
    chunk_last(
        DeducedTrailer const& trailer, Allocator const& allocator);
#endif

    //-----

    /// Required for <em>ConstBufferSequence</em>
    chunk_last(chunk_last const&) = default;

    /// Required for <em>ConstBufferSequence</em>
#if BOOST_BEAST_DOXYGEN
    using value_type = __implementation_defined__;
#else
    using value_type =
        typename view_type::value_type;
#endif

    /// Required for <em>ConstBufferSequence</em>
#if BOOST_BEAST_DOXYGEN
    using const_iterator = __implementation_defined__;
#else
    using const_iterator =
        typename view_type::const_iterator;
#endif

    /// Required for <em>ConstBufferSequence</em>
    const_iterator
    begin() const
    {
        return view_.begin();
    }

    /// Required for <em>ConstBufferSequence</em>
    const_iterator
    end() const
    {
        return view_.end();
    }
};

//------------------------------------------------------------------------------

/** A set of chunk extensions

    This container stores a set of chunk extensions suited for use with
    @ref chunk_header and @ref chunk_body. The container may be iterated
    to access the extensions in their structured form.

    Meets the requirements of ChunkExtensions
*/
template<class Allocator>
class basic_chunk_extensions
{
    std::basic_string<char,
        std::char_traits<char>, Allocator> s_;

    std::basic_string<char,
        std::char_traits<char>, Allocator> range_;

    template<class FwdIt>
    FwdIt
    do_parse(FwdIt it, FwdIt last, error_code& ec);

    void
    do_insert(string_view name, string_view value);

public:
    /** The type of value when iterating.

        The first element of the pair is the name, and the second
        element is the value which may be empty. The value is
        stored in its raw representation, without quotes or escapes.
    */
    using value_type = std::pair<string_view, string_view>;

    class const_iterator;

    /// Constructor
    basic_chunk_extensions() = default;

    /// Constructor
    basic_chunk_extensions(basic_chunk_extensions&&) = default;

    /// Constructor
    basic_chunk_extensions(basic_chunk_extensions const&) = default;

    /** Constructor

        @param allocator The allocator to use for storing the serialized extension
    */
    explicit
    basic_chunk_extensions(Allocator const& allocator)
        : s_(allocator)
    {
    }

    /** Clear the chunk extensions

        This preserves the capacity of the internal string
        used to hold the serialized representation.
    */
    void
    clear()
    {
        s_.clear();
    }

    /** Parse a set of chunk extensions

        Any previous extensions will be cleared
    */
    void
    parse(string_view s, error_code& ec);

    /** Insert an extension name with an empty value

        @param name The name of the extension
    */
    void
    insert(string_view name);

    /** Insert an extension value

        @param name The name of the extension

        @param value The value to insert. Depending on the
        contents, the serialized extension may use a quoted string.
    */
    void
    insert(string_view name, string_view value);

    /// Return the serialized representation of the chunk extension
    string_view
    str() const
    {
        return s_;
    }

    const_iterator
    begin() const;

    const_iterator
    end() const;
};

//------------------------------------------------------------------------------

/// A set of chunk extensions
using chunk_extensions =
    basic_chunk_extensions<std::allocator<char>>;

/** Returns a @ref chunk_body

    This functions constructs and returns a complete
    @ref chunk_body for a chunk body represented by the
    specified buffer sequence.

    @param buffers The buffers representing the chunk body.

    @param args Optional arguments passed to the @ref chunk_body constructor.

    @note This function is provided as a notational convenience
    to omit specification of the class template arguments.
*/
template<class ConstBufferSequence, class... Args>
auto
make_chunk(
    ConstBufferSequence const& buffers,
    Args&&... args) ->
    chunk_body<ConstBufferSequence>
{
    return chunk_body<ConstBufferSequence>(
        buffers, std::forward<Args>(args)...);
}

/** Returns a @ref chunk_last

    @note This function is provided as a notational convenience
    to omit specification of the class template arguments.
*/
inline
chunk_last<chunk_crlf>
make_chunk_last()
{
    return chunk_last<chunk_crlf>{};
}

/** Returns a @ref chunk_last

    This function construct and returns a complete
    @ref chunk_last for a last chunk containing the
    specified trailers.

    @param trailer A ConstBufferSequence or 
    @note This function is provided as a notational convenience
    to omit specification of the class template arguments.

    @param args Optional arguments passed to the @ref chunk_last
    constructor.
*/
template<class Trailer, class... Args>
chunk_last<Trailer>
make_chunk_last(
    Trailer const& trailer,
    Args&&... args)
{
    return chunk_last<Trailer>{
        trailer, std::forward<Args>(args)...};
}

} // http
} // beast
} // boost

#include <boost/beast/http/impl/chunk_encode.hpp>

#endif
