//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_MESSAGE_HPP
#define BOOST_BEAST_HTTP_MESSAGE_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/type_traits.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/core/empty_value.hpp>
#include <boost/mp11/integer_sequence.hpp>
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/throw_exception.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace boost {
namespace beast {
namespace http {

/** A container for an HTTP request or response header.

    This container is derived from the `Fields` template type.
    To understand all of the members of this class it is necessary
    to view the declaration for the `Fields` type. When using
    the default fields container, those declarations are in
    @ref fields.

    Newly constructed header objects have version set to
    HTTP/1.1. Newly constructed response objects also have
    result code set to @ref status::ok.

    A `header` includes the start-line and header-fields.
*/
#if BOOST_BEAST_DOXYGEN
template<bool isRequest, class Fields = fields>
class header : public Fields

#else
template<bool isRequest, class Fields = fields>
class header;

template<class Fields>
class header<true, Fields> : public Fields
#endif
{
public:
    static_assert(is_fields<Fields>::value,
        "Fields type requirements not met");

    /// Indicates if the header is a request or response.
#if BOOST_BEAST_DOXYGEN
    using is_request = std::integral_constant<bool, isRequest>;
#else
    using is_request = std::true_type;
#endif

    /// The type representing the fields.
    using fields_type = Fields;

    /// Constructor
    header() = default;

    /// Constructor
    header(header&&) = default;

    /// Constructor
    header(header const&) = default;

    /// Assignment
    header& operator=(header&&) = default;

    /// Assignment
    header& operator=(header const&) = default;

    /** Return the HTTP-version.

        This holds both the major and minor version numbers,
        using these formulas:
        @code
            unsigned major = version / 10;
            unsigned minor = version % 10;
        @endcode

        Newly constructed headers will use HTTP/1.1 by default.
    */
    unsigned version() const noexcept
    {
        return version_;
    }

    /** Set the HTTP-version.

        This holds both the major and minor version numbers,
        using these formulas:
        @code
            unsigned major = version / 10;
            unsigned minor = version % 10;
        @endcode

        Newly constructed headers will use HTTP/1.1 by default.

        @param value The version number to use
    */
    void version(unsigned value) noexcept
    {
        BOOST_ASSERT(value > 0 && value < 100);
        version_ = value;
    }

    /** Return the request-method verb.

        If the request-method is not one of the recognized verbs,
        @ref verb::unknown is returned. Callers may use @ref method_string
        to retrieve the exact text.

        @note This function is only available when `isRequest == true`.

        @see method_string
    */
    verb
    method() const;

    /** Set the request-method.

        This function will set the method for requests to a known verb.

        @param v The request method verb to set.
        This may not be @ref verb::unknown.

        @throws std::invalid_argument when `v == verb::unknown`.

        @note This function is only available when `isRequest == true`.
    */
    void
    method(verb v);

    /** Return the request-method as a string.

        @note This function is only available when `isRequest == true`.

        @see method
    */
    string_view
    method_string() const;

    /** Set the request-method.

        This function will set the request-method a known verb
        if the string matches, otherwise it will store a copy of
        the passed string.

        @param s A string representing the request-method.

        @note This function is only available when `isRequest == true`.
    */
    void
    method_string(string_view s);

    /** Returns the request-target string.

        The request target string returned is the same string which
        was received from the network or stored. In particular, it will
        contain url-encoded characters and should follow the syntax
        rules for URIs used with HTTP.

        @note This function is only available when `isRequest == true`.
    */
    string_view
    target() const;

    /** Set the request-target string.

        It is the caller's responsibility to ensure that the request
        target string follows the syntax rules for URIs used with
        HTTP. In particular, reserved or special characters must be
        url-encoded. The implementation does not perform syntax checking
        on the passed string.

        @param s A string representing the request-target.

        @note This function is only available when `isRequest == true`.
    */
    void
    target(string_view s);

    // VFALCO Don't rearrange these declarations or
    //        ifdefs, or else the documentation will break.

    /** Constructor

        @param args Arguments forwarded to the `Fields`
        base class constructor.

        @note This constructor participates in overload
        resolution if and only if the first parameter is
        not convertible to @ref header, @ref verb, or
        @ref status.
    */
#if BOOST_BEAST_DOXYGEN
    template<class... Args>
    explicit
    header(Args&&... args);

#else
    template<class Arg1, class... ArgN,
        class = typename std::enable_if<
            ! std::is_convertible<typename
                std::decay<Arg1>::type, header>::value &&
            ! std::is_convertible<typename
                std::decay<Arg1>::type, verb>::value &&
            ! std::is_convertible<typename
                std::decay<Arg1>::type, status>::value
        >::type>
    explicit
    header(Arg1&& arg1, ArgN&&... argn);

private:
    template<bool, class, class>
    friend class message;

    template<class T>
    friend
    void
    swap(header<true, T>& m1, header<true, T>& m2);

    template<class... FieldsArgs>
    header(
        verb method,
        string_view target_,
        unsigned version_value,
        FieldsArgs&&... fields_args)
        : Fields(std::forward<FieldsArgs>(fields_args)...)
        , method_(method)
    {
        version(version_value);
        target(target_);
    }

    unsigned version_ = 11;
    verb method_ = verb::unknown;
};

/** A container for an HTTP request or response header.

    A `header` includes the start-line and header-fields.
*/
template<class Fields>
class header<false, Fields> : public Fields
{
public:
    static_assert(is_fields<Fields>::value,
        "Fields type requirements not met");

    /// Indicates if the header is a request or response.
    using is_request = std::false_type;

    /// The type representing the fields.
    using fields_type = Fields;

    /// Constructor.
    header() = default;

    /// Constructor
    header(header&&) = default;

    /// Constructor
    header(header const&) = default;

    /// Assignment
    header& operator=(header&&) = default;

    /// Assignment
    header& operator=(header const&) = default;

    /** Constructor

        @param args Arguments forwarded to the `Fields`
        base class constructor.

        @note This constructor participates in overload
        resolution if and only if the first parameter is
        not convertible to @ref header, @ref verb, or
        @ref status.
    */
    template<class Arg1, class... ArgN,
        class = typename std::enable_if<
            ! std::is_convertible<typename
                std::decay<Arg1>::type, header>::value &&
            ! std::is_convertible<typename
                std::decay<Arg1>::type, verb>::value &&
            ! std::is_convertible<typename
                std::decay<Arg1>::type, status>::value
        >::type>
    explicit
    header(Arg1&& arg1, ArgN&&... argn);

    /** Return the HTTP-version.

        This holds both the major and minor version numbers,
        using these formulas:
        @code
            unsigned major = version / 10;
            unsigned minor = version % 10;
        @endcode

        Newly constructed headers will use HTTP/1.1 by default.
    */
    unsigned version() const noexcept
    {
        return version_;
    }

    /** Set the HTTP-version.

        This holds both the major and minor version numbers,
        using these formulas:
        @code
            unsigned major = version / 10;
            unsigned minor = version % 10;
        @endcode

        Newly constructed headers will use HTTP/1.1 by default.

        @param value The version number to use
    */
    void version(unsigned value) noexcept
    {
        BOOST_ASSERT(value > 0 && value < 100);
        version_ = value;
    }
#endif

    /** The response status-code result.

        If the actual status code is not a known code, this
        function returns @ref status::unknown. Use @ref result_int
        to return the raw status code as a number.

        @note This member is only available when `isRequest == false`.
    */
    status
    result() const;

    /** Set the response status-code.

        @param v The code to set.

        @note This member is only available when `isRequest == false`.
    */
    void
    result(status v);

    /** Set the response status-code as an integer.

        This sets the status code to the exact number passed in.
        If the number does not correspond to one of the known
        status codes, the function @ref result will return
        @ref status::unknown. Use @ref result_int to obtain the
        original raw status-code.

        @param v The status-code integer to set.

        @throws std::invalid_argument if `v > 999`.
    */
    void
    result(unsigned v);

    /** The response status-code expressed as an integer.

        This returns the raw status code as an integer, even
        when that code is not in the list of known status codes.

        @note This member is only available when `isRequest == false`.
    */
    unsigned
    result_int() const;

    /** Return the response reason-phrase.

        The reason-phrase is obsolete as of rfc7230.

        @note This function is only available when `isRequest == false`.
    */
    string_view
    reason() const;

    /** Set the response reason-phrase (deprecated)

        This function sets a custom reason-phrase to a copy of
        the string passed in. Normally it is not necessary to set
        the reason phrase on an outgoing response object; the
        implementation will automatically use the standard reason
        text for the corresponding status code.

        To clear a previously set custom phrase, pass an empty
        string. This will restore the default standard reason text
        based on the status code used when serializing.

        The reason-phrase is obsolete as of rfc7230.

        @param s The string to use for the reason-phrase.

        @note This function is only available when `isRequest == false`.
    */
    void
    reason(string_view s);

private:
#if ! BOOST_BEAST_DOXYGEN
    template<bool, class, class>
    friend class message;

    template<class T>
    friend
    void
    swap(header<false, T>& m1, header<false, T>& m2);

    template<class... FieldsArgs>
    header(
        status result,
        unsigned version_value,
        FieldsArgs&&... fields_args)
        : Fields(std::forward<FieldsArgs>(fields_args)...)
        , result_(result)
    {
        version(version_value);
    }

    unsigned version_ = 11;
    status result_ = status::ok;
#endif
};

/// A typical HTTP request header
template<class Fields = fields>
using request_header = header<true, Fields>;

/// A typical HTTP response header
template<class Fields = fields>
using response_header = header<false, Fields>;

#if defined(BOOST_MSVC)
// Workaround for MSVC bug with private base classes
namespace detail {
template<class T>
using value_type_t = typename T::value_type;
} // detail
#endif

/** A container for a complete HTTP message.

    This container is derived from the `Fields` template type.
    To understand all of the members of this class it is necessary
    to view the declaration for the `Fields` type. When using
    the default fields container, those declarations are in
    @ref fields.

    A message can be a request or response, depending on the
    `isRequest` template argument value. Requests and responses
    have different types; functions may be overloaded based on
    the type if desired.

    The `Body` template argument type determines the model used
    to read or write the content body of the message.

    Newly constructed messages objects have version set to
    HTTP/1.1. Newly constructed response objects also have
    result code set to @ref status::ok.

    @tparam isRequest `true` if this represents a request,
    or `false` if this represents a response. Some class data
    members are conditionally present depending on this value.

    @tparam Body A type meeting the requirements of Body.

    @tparam Fields The type of container used to hold the
    field value pairs.
*/
template<bool isRequest, class Body, class Fields = fields>
class message
    : public header<isRequest, Fields>
#if ! BOOST_BEAST_DOXYGEN
    , boost::empty_value<
        typename Body::value_type>
#endif
{
public:
    /// The base class used to hold the header portion of the message.
    using header_type = header<isRequest, Fields>;

    /** The type providing the body traits.

        The @ref message::body member will be of type `body_type::value_type`.
    */
    using body_type = Body;

    /// Constructor
    message() = default;

    /// Constructor
    message(message&&) = default;

    /// Constructor
    message(message const&) = default;

    /// Assignment
    message& operator=(message&&) = default;

    /// Assignment
    message& operator=(message const&) = default;

    /** Constructor

        @param h The header to move construct from.

        @param body_args Optional arguments forwarded
        to the `body` constructor.
    */
    template<class... BodyArgs>
    explicit
    message(header_type&& h, BodyArgs&&... body_args);

    /** Constructor.

        @param h The header to copy construct from.

        @param body_args Optional arguments forwarded
        to the `body` constructor.
    */
    template<class... BodyArgs>
    explicit
    message(header_type const& h, BodyArgs&&... body_args);

    /** Constructor

        @param method The request-method to use.

        @param target The request-target.

        @param version The HTTP-version.

        @note This function is only available when `isRequest == true`.
    */
#if BOOST_BEAST_DOXYGEN
    message(verb method, string_view target, unsigned version);
#else
    template<class Version,
        class = typename std::enable_if<isRequest &&
            std::is_convertible<Version, unsigned>::value>::type>
    message(verb method, string_view target, Version version);
#endif

    /** Constructor

        @param method The request-method to use.

        @param target The request-target.

        @param version The HTTP-version.

        @param body_arg An argument forwarded to the `body` constructor.

        @note This function is only available when `isRequest == true`.
    */
#if BOOST_BEAST_DOXYGEN
    template<class BodyArg>
    message(verb method, string_view target,
        unsigned version, BodyArg&& body_arg);
#else
    template<class Version, class BodyArg,
        class = typename std::enable_if<isRequest &&
            std::is_convertible<Version, unsigned>::value>::type>
    message(verb method, string_view target,
        Version version, BodyArg&& body_arg);
#endif

    /** Constructor

        @param method The request-method to use.

        @param target The request-target.

        @param version The HTTP-version.

        @param body_arg An argument forwarded to the `body` constructor.

        @param fields_arg An argument forwarded to the `Fields` constructor.

        @note This function is only available when `isRequest == true`.
    */
#if BOOST_BEAST_DOXYGEN
    template<class BodyArg, class FieldsArg>
    message(verb method, string_view target, unsigned version,
        BodyArg&& body_arg, FieldsArg&& fields_arg);
#else
    template<class Version, class BodyArg, class FieldsArg,
        class = typename std::enable_if<isRequest &&
            std::is_convertible<Version, unsigned>::value>::type>
    message(verb method, string_view target, Version version,
        BodyArg&& body_arg, FieldsArg&& fields_arg);
#endif

    /** Constructor

        @param result The status-code for the response.

        @param version The HTTP-version.

        @note This member is only available when `isRequest == false`.
    */
#if BOOST_BEAST_DOXYGEN
    message(status result, unsigned version);
#else
    template<class Version,
        class = typename std::enable_if<! isRequest &&
           std::is_convertible<Version, unsigned>::value>::type>
    message(status result, Version version);
#endif

    /** Constructor

        @param result The status-code for the response.

        @param version The HTTP-version.

        @param body_arg An argument forwarded to the `body` constructor.

        @note This member is only available when `isRequest == false`.
    */
#if BOOST_BEAST_DOXYGEN
    template<class BodyArg>
    message(status result, unsigned version, BodyArg&& body_arg);
#else
    template<class Version, class BodyArg,
        class = typename std::enable_if<! isRequest &&
           std::is_convertible<Version, unsigned>::value>::type>
    message(status result, Version version, BodyArg&& body_arg);
#endif

    /** Constructor

        @param result The status-code for the response.

        @param version The HTTP-version.

        @param body_arg An argument forwarded to the `body` constructor.

        @param fields_arg An argument forwarded to the `Fields` base class constructor.

        @note This member is only available when `isRequest == false`.
    */
#if BOOST_BEAST_DOXYGEN
    template<class BodyArg, class FieldsArg>
    message(status result, unsigned version,
        BodyArg&& body_arg, FieldsArg&& fields_arg);
#else
    template<class Version, class BodyArg, class FieldsArg,
        class = typename std::enable_if<! isRequest &&
           std::is_convertible<Version, unsigned>::value>::type>
    message(status result, Version version,
        BodyArg&& body_arg, FieldsArg&& fields_arg);
#endif

    /** Constructor

        The header and body are default-constructed.
    */
    explicit
    message(std::piecewise_construct_t);

    /** Construct a message.

        @param body_args A tuple forwarded as a parameter
        pack to the body constructor.
    */
    template<class... BodyArgs>
    message(std::piecewise_construct_t,
        std::tuple<BodyArgs...> body_args);

    /** Construct a message.

        @param body_args A tuple forwarded as a parameter
        pack to the body constructor.

        @param fields_args A tuple forwarded as a parameter
        pack to the `Fields` constructor.
    */
    template<class... BodyArgs, class... FieldsArgs>
    message(std::piecewise_construct_t,
        std::tuple<BodyArgs...> body_args,
        std::tuple<FieldsArgs...> fields_args);

    /// Returns the header portion of the message
    header_type const&
    base() const
    {
        return *this;
    }

    /// Returns the header portion of the message
    header_type&
    base()
    {
        return *this;
    }

    /// Returns `true` if the chunked Transfer-Encoding is specified
    bool
    chunked() const
    {
        return this->get_chunked_impl();
    }

    /** Set or clear the chunked Transfer-Encoding

        This function will set or remove the "chunked" transfer
        encoding as the last item in the list of encodings in the
        field.

        If the result of removing the chunked token results in an
        empty string, the field is erased.

        The Content-Length field is erased unconditionally.
    */
    void
    chunked(bool value);

    /** Returns `true` if the Content-Length field is present.

        This function inspects the fields and returns `true` if
        the Content-Length field is present. The properties of the
        body are not checked, this only looks for the field.
    */
    bool
    has_content_length() const
    {
        return this->has_content_length_impl();
    }

    /** Set or clear the Content-Length field

        This function adjusts the Content-Length field as follows:

        @li If `value` specifies a value, the Content-Length field
          is set to the value. Otherwise

        @li The Content-Length field is erased.

        If "chunked" token appears as the last item in the
        Transfer-Encoding field it is unconditionally removed.

        @param value The value to set for Content-Length.
    */
    void
    content_length(boost::optional<std::uint64_t> const& value);

    /** Returns `true` if the message semantics indicate keep-alive

        The value depends on the version in the message, which must
        be set to the final value before this function is called or
        else the return value is unreliable.
    */
    bool
    keep_alive() const
    {
        return this->get_keep_alive_impl(this->version());
    }

    /** Set the keep-alive message semantic option

        This function adjusts the Connection field to indicate
        whether or not the connection should be kept open after
        the corresponding response. The result depends on the
        version set on the message, which must be set to the
        final value before making this call.

        @param value `true` if the connection should persist.
    */
    void
    keep_alive(bool value)
    {
        this->set_keep_alive_impl(this->version(), value);
    }

    /** Returns `true` if the message semantics require an end of file.

        For HTTP requests, this function returns the logical
        NOT of a call to @ref keep_alive.

        For HTTP responses, this function returns the logical NOT
        of a call to @ref keep_alive if any of the following are true:

        @li @ref has_content_length would return `true`

        @li @ref chunked would return `true`

        @li @ref result returns @ref status::no_content

        @li @ref result returns @ref status::not_modified

        @li @ref result returns any informational status class (100 to 199)

        Otherwise, the function returns `true`.

        @see https://tools.ietf.org/html/rfc7230#section-3.3
    */
    bool
    need_eof() const
    {
        return need_eof(typename header_type::is_request{});
    }

    /** Returns the payload size of the body in octets if possible.

        This function invokes the <em>Body</em> algorithm to measure
        the number of octets in the serialized body container. If
        there is no body, this will return zero. Otherwise, if the
        body exists but is not known ahead of time, `boost::none`
        is returned (usually indicating that a chunked Transfer-Encoding
        will be used).

        @note The value of the Content-Length field in the message
        is not inspected.
    */
    boost::optional<std::uint64_t>
    payload_size() const;

    /** Prepare the message payload fields for the body.

        This function will adjust the Content-Length and
        Transfer-Encoding field values based on the properties
        of the body.

        @par Example
        @code
        request<string_body> req{verb::post, "/"};
        req.set(field::user_agent, "Beast");
        req.body() = "Hello, world!";
        req.prepare_payload();
        @endcode
    */
    void
    prepare_payload()
    {
        prepare_payload(typename header_type::is_request{});
    }

    /// Returns the body
#if BOOST_BEAST_DOXYGEN || ! defined(BOOST_MSVC)
    typename body_type::value_type&
#else
    detail::value_type_t<Body>&
#endif
    body()& noexcept
    {
        return this->boost::empty_value<
            typename Body::value_type>::get();
    }

    /// Returns the body
#if BOOST_BEAST_DOXYGEN || ! defined(BOOST_MSVC)
    typename body_type::value_type&&
#else
    detail::value_type_t<Body>&&
#endif
    body()&& noexcept
    {
        return std::move(
            this->boost::empty_value<
                typename Body::value_type>::get());
    }

    /// Returns the body
#if BOOST_BEAST_DOXYGEN || ! defined(BOOST_MSVC)
    typename body_type::value_type const&
#else
    detail::value_type_t<Body> const&
#endif
    body() const& noexcept
    {
        return this->boost::empty_value<
            typename Body::value_type>::get();
    }

private:
    static_assert(is_body<Body>::value,
        "Body type requirements not met");

    template<
        class... BodyArgs,
        std::size_t... IBodyArgs>
    message(
        std::piecewise_construct_t,
        std::tuple<BodyArgs...>& body_args,
        mp11::index_sequence<IBodyArgs...>)
        : boost::empty_value<
            typename Body::value_type>(boost::empty_init_t(),
                std::forward<BodyArgs>(
                std::get<IBodyArgs>(body_args))...)
    {
        boost::ignore_unused(body_args);
    }

    template<
        class... BodyArgs,
        class... FieldsArgs,
        std::size_t... IBodyArgs,
        std::size_t... IFieldsArgs>
    message(
        std::piecewise_construct_t,
        std::tuple<BodyArgs...>& body_args,
        std::tuple<FieldsArgs...>& fields_args,
        mp11::index_sequence<IBodyArgs...>,
        mp11::index_sequence<IFieldsArgs...>)
        : header_type(std::forward<FieldsArgs>(
            std::get<IFieldsArgs>(fields_args))...)
        , boost::empty_value<
            typename Body::value_type>(boost::empty_init_t(),
                std::forward<BodyArgs>(
                std::get<IBodyArgs>(body_args))...)
    {
        boost::ignore_unused(body_args);
        boost::ignore_unused(fields_args);
    }

    bool
    need_eof(std::true_type) const
    {
        return ! keep_alive();
    }

    bool
    need_eof(std::false_type) const;

    boost::optional<std::uint64_t>
    payload_size(std::true_type) const
    {
        return Body::size(this->body());
    }

    boost::optional<std::uint64_t>
    payload_size(std::false_type) const
    {
        return boost::none;
    }

    void
    prepare_payload(std::true_type);

    void
    prepare_payload(std::false_type);
};

/// A typical HTTP request
template<class Body, class Fields = fields>
using request = message<true, Body, Fields>;

/// A typical HTTP response
template<class Body, class Fields = fields>
using response = message<false, Body, Fields>;

//------------------------------------------------------------------------------

#if BOOST_BEAST_DOXYGEN
/** Swap two header objects.

    @par Requirements
    `Fields` is @b Swappable.
*/
template<bool isRequest, class Fields>
void
swap(
    header<isRequest, Fields>& m1,
    header<isRequest, Fields>& m2);
#endif

/** Swap two message objects.

    @par Requirements:
    `Body::value_type` and `Fields` are @b Swappable.
*/
template<bool isRequest, class Body, class Fields>
void
swap(
    message<isRequest, Body, Fields>& m1,
    message<isRequest, Body, Fields>& m2);

} // http
} // beast
} // boost

#include <boost/beast/http/impl/message.hpp>

#endif
