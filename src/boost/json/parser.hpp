//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_PARSER_HPP
#define BOOST_JSON_PARSER_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/basic_parser.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>
#include <boost/json/detail/handler.hpp>
#include <type_traits>
#include <cstddef>

BOOST_JSON_NS_BEGIN

//----------------------------------------------------------

/** A DOM parser for JSON contained in a single buffer.

    This class is used to parse a JSON contained in a
    single character buffer, into a @ref value container.

    @par Usage

    To use the parser first construct it, then optionally
    call @ref reset to specify a @ref storage_ptr to use
    for the resulting @ref value. Then call @ref write
    to parse a character buffer containing a complete
    JSON. If the parse is successful, call @ref release
    to take ownership of the value:
    @code
    parser p;                                       // construct a parser
    size_t n = p.write( "[1,2,3]" );                // parse a complete JSON
    assert( n == 7 );                               // all characters consumed
    value jv = p.release();                         // take ownership of the value
    @endcode

    @par Extra Data

    When the character buffer provided as input contains
    additional data that is not part of the complete
    JSON, an error is returned. The @ref write_some
    function is an alternative which allows the parse
    to finish early, without consuming all the characters
    in the buffer. This allows parsing of a buffer
    containing multiple individual JSONs or containing
    different protocol data:
    @code
    parser p;                                       // construct a parser
    size_t n = p.write_some( "[1,2,3] null" );      // parse a complete JSON
    assert( n == 8 );                               // only some characters consumed
    value jv = p.release();                         // take ownership of the value
    @endcode

    @par Temporary Storage

    The parser may dynamically allocate temporary
    storage as needed to accommodate the nesting level
    of the JSON being parsed. Temporary storage is
    first obtained from an optional, caller-owned
    buffer specified upon construction. When that
    is exhausted, the next allocation uses the
    @ref memory_resource passed to the constructor; if
    no such argument is specified, the default memory
    resource is used. Temporary storage is freed only
    when the parser is destroyed; The performance of
    parsing multiple JSONs may be improved by reusing
    the same parser instance.
\n
    It is important to note that the @ref memory_resource
    supplied upon construction is used for temporary
    storage only, and not for allocating the elements
    which make up the parsed value. That other memory
    resource is optionally supplied in each call
    to @ref reset.

    @par Duplicate Keys

    If there are object elements with duplicate keys;
    that is, if multiple elements in an object have
    keys that compare equal, only the last equivalent
    element will be inserted.

    @par Non-Standard JSON

    The @ref parse_options structure optionally
    provided upon construction is used to customize
    some parameters of the parser, including which
    non-standard JSON extensions should be allowed.
    A default-constructed parse options allows only
    standard JSON.

    @par Thread Safety

    Distinct instances may be accessed concurrently.
    Non-const member functions of a shared instance
    may not be called concurrently with any other
    member functions of that instance.

    @see
        @ref parse,
        @ref parse_options,
        @ref stream_parser.
*/
class parser
{
    basic_parser<detail::handler> p_;

public:
    /// Copy constructor (deleted)
    parser(
        parser const&) = delete;

    /// Copy assignment (deleted)
    parser& operator=(
        parser const&) = delete;

    /** Destructor.

        All dynamically allocated memory, including
        any incomplete parsing results, is freed.

        @par Complexity
        Linear in the size of partial results

        @par Exception Safety
        No-throw guarantee.
    */
    ~parser() = default;

    /** Constructor.

        This constructs a new parser which first uses
        the caller-owned storage pointed to by `buffer`
        for temporary storage, falling back to the memory
        resource `sp` if needed. The parser will use the
        specified parsing options.
    \n
        The parsed value will use the default memory
        resource for storage. To use a different resource,
        call @ref reset after construction.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param sp The memory resource to use for
        temporary storage after `buffer` is exhausted.

        @param opt The parsing options to use.

        @param buffer A pointer to valid memory of at least
        `size` bytes for the parser to use for temporary storage.
        Ownership is not transferred, the caller is responsible
        for ensuring the lifetime of the memory pointed to by
        `buffer` extends until the parser is destroyed.

        @param size The number of valid bytes in `buffer`.
    */
    BOOST_JSON_DECL
    parser(
        storage_ptr sp,
        parse_options const& opt,
        unsigned char* buffer,
        std::size_t size) noexcept;

    /** Constructor.

        This constructs a new parser which uses the default
        memory resource for temporary storage, and accepts
        only strict JSON.
    \n
        The parsed value will use the default memory
        resource for storage. To use a different resource,
        call @ref reset after construction.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    parser() noexcept
        : parser({}, {})
    {
    }

    /** Constructor.

        This constructs a new parser which uses the
        specified memory resource for temporary storage,
        and is configured to use the specified parsing
        options.
    \n
        The parsed value will use the default memory
        resource for storage. To use a different resource,
        call @ref reset after construction.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param sp The memory resource to use for temporary storage.

        @param opt The parsing options to use.
    */
    BOOST_JSON_DECL
    parser(
        storage_ptr sp,
        parse_options const& opt) noexcept;

    /** Constructor.

        This constructs a new parser which uses the
        specified memory resource for temporary storage,
        and accepts only strict JSON.
    \n
        The parsed value will use the default memory
        resource for storage. To use a different resource,
        call @ref reset after construction.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param sp The memory resource to use for temporary storage.
    */
    explicit
    parser(storage_ptr sp) noexcept
        : parser(std::move(sp), {})
    {
    }

    /** Constructor.

        This constructs a new parser which first uses the
        caller-owned storage `buffer` for temporary storage,
        falling back to the memory resource `sp` if needed.
        The parser will use the specified parsing options.
    \n
        The parsed value will use the default memory
        resource for storage. To use a different resource,
        call @ref reset after construction.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param sp The memory resource to use for
        temporary storage after `buffer` is exhausted.

        @param opt The parsing options to use.

        @param buffer A buffer for the parser to use for
        temporary storage. Ownership is not transferred,
        the caller is responsible for ensuring the lifetime
        of `buffer` extends until the parser is destroyed.
    */
    template<std::size_t N>
    parser(
        storage_ptr sp,
        parse_options const& opt,
        unsigned char(&buffer)[N]) noexcept
        : parser(std::move(sp),
            opt, &buffer[0], N)
    {
    }

#if defined(__cpp_lib_byte) || defined(BOOST_JSON_DOCS)
    /** Constructor.

        This constructs a new parser which first uses
        the caller-owned storage pointed to by `buffer`
        for temporary storage, falling back to the memory
        resource `sp` if needed. The parser will use the
        specified parsing options.
    \n
        The parsed value will use the default memory
        resource for storage. To use a different resource,
        call @ref reset after construction.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param sp The memory resource to use for
        temporary storage after `buffer` is exhausted.

        @param opt The parsing options to use.

        @param buffer A pointer to valid memory of at least
        `size` bytes for the parser to use for temporary storage.
        Ownership is not transferred, the caller is responsible
        for ensuring the lifetime of the memory pointed to by
        `buffer` extends until the parser is destroyed.

        @param size The number of valid bytes in `buffer`.
    */
    parser(
        storage_ptr sp,
        parse_options const& opt,
        std::byte* buffer,
        std::size_t size) noexcept
        : parser(sp, opt, reinterpret_cast<
            unsigned char*>(buffer), size)
    {
    }

    /** Constructor.

        This constructs a new parser which first uses the
        caller-owned storage `buffer` for temporary storage,
        falling back to the memory resource `sp` if needed.
        The parser will use the specified parsing options.
    \n
        The parsed value will use the default memory
        resource for storage. To use a different resource,
        call @ref reset after construction.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param sp The memory resource to use for
        temporary storage after `buffer` is exhausted.

        @param opt The parsing options to use.

        @param buffer A buffer for the parser to use for
        temporary storage. Ownership is not transferred,
        the caller is responsible for ensuring the lifetime
        of `buffer` extends until the parser is destroyed.
    */
    template<std::size_t N>
    parser(
        storage_ptr sp,
        parse_options const& opt,
        std::byte(&buffer)[N]) noexcept
        : parser(std::move(sp),
            opt, &buffer[0], N)
    {
    }
#endif

#ifndef BOOST_JSON_DOCS
    // Safety net for accidental buffer overflows
    template<std::size_t N>
    parser(
        storage_ptr sp,
        parse_options const& opt,
        unsigned char(&buffer)[N],
        std::size_t n) noexcept
        : parser(std::move(sp),
            opt, &buffer[0], n)
    {
        // If this goes off, check your parameters
        // closely, chances are you passed an array
        // thinking it was a pointer.
        BOOST_ASSERT(n <= N);
    }

#ifdef __cpp_lib_byte
    // Safety net for accidental buffer overflows
    template<std::size_t N>
    parser(
        storage_ptr sp,
        parse_options const& opt,
        std::byte(&buffer)[N], std::size_t n) noexcept
        : parser(std::move(sp),
            opt, &buffer[0], n)
    {
        // If this goes off, check your parameters
        // closely, chances are you passed an array
        // thinking it was a pointer.
        BOOST_ASSERT(n <= N);
    }
#endif
#endif

    /** Reset the parser for a new JSON.

        This function is used to reset the parser to
        prepare it for parsing a new complete JSON.
        Any previous partial results are destroyed.

        @par Complexity
        Constant or linear in the size of any previous
        partial parsing results.

        @par Exception Safety
        No-throw guarantee.

        @param sp A pointer to the @ref memory_resource
        to use for the resulting @ref value. The parser
        will acquire shared ownership.
    */
    BOOST_JSON_DECL
    void
    reset(storage_ptr sp = {}) noexcept;

    /** Parse a buffer containing a complete JSON.

        This function parses a complete JSON contained
        in the specified character buffer. Additional
        characters past the end of the complete JSON
        are ignored. The function returns the actual
        number of characters parsed, which may be less
        than the size of the input. This allows parsing
        of a buffer containing multiple individual JSONs
        or containing different protocol data:

        @par Example
        @code
        parser p;                                       // construct a parser
        size_t n = p.write_some( "[1,2,3] null" );      // parse a complete JSON
        assert( n == 8 );                               // only some characters consumed
        value jv = p.release();                         // take ownership of the value
        @endcode

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters consumed from
        the buffer.

        @param data A pointer to a buffer of `size`
        characters to parse.

        @param size The number of characters pointed to
        by `data`.

        @param ec Set to the error, if any occurred.
    */
    BOOST_JSON_DECL
    std::size_t
    write_some(
        char const* data,
        std::size_t size,
        error_code& ec);

    /** Parse a buffer containing a complete JSON.

        This function parses a complete JSON contained
        in the specified character buffer. Additional
        characters past the end of the complete JSON
        are ignored. The function returns the actual
        number of characters parsed, which may be less
        than the size of the input. This allows parsing
        of a buffer containing multiple individual JSONs
        or containing different protocol data:

        @par Example
        @code
        parser p;                                       // construct a parser
        size_t n = p.write_some( "[1,2,3] null" );      // parse a complete JSON
        assert( n == 8 );                               // only some characters consumed
        value jv = p.release();                         // take ownership of the value
        @endcode

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters consumed from
        the buffer.

        @param data A pointer to a buffer of `size`
        characters to parse.

        @param size The number of characters pointed to
        by `data`.

        @throw system_error Thrown on error.
    */
    BOOST_JSON_DECL
    std::size_t
    write_some(
        char const* data,
        std::size_t size);

    /** Parse a buffer containing a complete JSON.

        This function parses a complete JSON contained
        in the specified character buffer. Additional
        characters past the end of the complete JSON
        are ignored. The function returns the actual
        number of characters parsed, which may be less
        than the size of the input. This allows parsing
        of a buffer containing multiple individual JSONs
        or containing different protocol data:

        @par Example
        @code
        parser p;                                       // construct a parser
        size_t n = p.write_some( "[1,2,3] null" );      // parse a complete JSON
        assert( n == 8 );                               // only some characters consumed
        value jv = p.release();                         // take ownership of the value
        @endcode

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters consumed from
        the buffer.

        @param s The character string to parse.

        @param ec Set to the error, if any occurred.
    */
    std::size_t
    write_some(
        string_view s,
        error_code& ec)
    {
        return write_some(
            s.data(), s.size(), ec);
    }

    /** Parse a buffer containing a complete JSON.

        This function parses a complete JSON contained
        in the specified character buffer. Additional
        characters past the end of the complete JSON
        are ignored. The function returns the actual
        number of characters parsed, which may be less
        than the size of the input. This allows parsing
        of a buffer containing multiple individual JSONs
        or containing different protocol data:

        @par Example
        @code
        parser p;                                       // construct a parser
        size_t n = p.write_some( "[1,2,3] null" );      // parse a complete JSON
        assert( n == 8 );                               // only some characters consumed
        value jv = p.release();                         // take ownership of the value
        @endcode

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters consumed from
        the buffer.

        @param s The character string to parse.

        @throw system_error Thrown on error.
    */
    std::size_t
    write_some(
        string_view s)
    {
        return write_some(
            s.data(), s.size());
    }

    /** Parse a buffer containing a complete JSON.

        This function parses a complete JSON contained
        in the specified character buffer. The entire
        buffer must be consumed; if there are additional
        characters past the end of the complete JSON,
        the parse fails and an error is returned.

        @par Example
        @code
        parser p;                                       // construct a parser
        size_t n = p.write( "[1,2,3]" );                // parse a complete JSON
        assert( n == 7 );                               // all characters consumed
        value jv = p.release();                         // take ownership of the value
        @endcode

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters consumed from
        the buffer.

        @param data A pointer to a buffer of `size`
        characters to parse.

        @param size The number of characters pointed to
        by `data`.

        @param ec Set to the error, if any occurred.
    */
    BOOST_JSON_DECL
    std::size_t
    write(
        char const* data,
        std::size_t size,
        error_code& ec);

    /** Parse a buffer containing a complete JSON.

        This function parses a complete JSON contained
        in the specified character buffer. The entire
        buffer must be consumed; if there are additional
        characters past the end of the complete JSON,
        the parse fails and an error is returned.

        @par Example
        @code
        parser p;                                       // construct a parser
        size_t n = p.write( "[1,2,3]" );                // parse a complete JSON
        assert( n == 7 );                               // all characters consumed
        value jv = p.release();                         // take ownership of the value
        @endcode

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters consumed from
        the buffer.

        @param data A pointer to a buffer of `size`
        characters to parse.

        @param size The number of characters pointed to
        by `data`.

        @throw system_error Thrown on error.
    */
    BOOST_JSON_DECL
    std::size_t
    write(
        char const* data,
        std::size_t size);

    /** Parse a buffer containing a complete JSON.

        This function parses a complete JSON contained
        in the specified character buffer. The entire
        buffer must be consumed; if there are additional
        characters past the end of the complete JSON,
        the parse fails and an error is returned.

        @par Example
        @code
        parser p;                                       // construct a parser
        size_t n = p.write( "[1,2,3]" );                // parse a complete JSON
        assert( n == 7 );                               // all characters consumed
        value jv = p.release();                         // take ownership of the value
        @endcode

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters consumed from
        the buffer.

        @param s The character string to parse.

        @param ec Set to the error, if any occurred.
    */
    std::size_t
    write(
        string_view s,
        error_code& ec)
    {
        return write(
            s.data(), s.size(), ec);
    }

    /** Parse a buffer containing a complete JSON.

        This function parses a complete JSON contained
        in the specified character buffer. The entire
        buffer must be consumed; if there are additional
        characters past the end of the complete JSON,
        the parse fails and an error is returned.

        @par Example
        @code
        parser p;                                       // construct a parser
        size_t n = p.write( "[1,2,3]" );                // parse a complete JSON
        assert( n == 7 );                               // all characters consumed
        value jv = p.release();                         // take ownership of the value
        @endcode

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters consumed from
        the buffer.

        @param s The character string to parse.

        @throw system_error Thrown on error.
    */
    std::size_t
    write(
        string_view s)
    {
        return write(
            s.data(), s.size());
    }

    /** Return the parsed JSON as a @ref value.

        This returns the parsed value, or throws
        an exception if the parsing is incomplete or
        failed. It is necessary to call @ref reset
        after calling this function in order to parse
        another JSON.

        @par Complexity
        Constant.

        @return The parsed value. Ownership of this
        value is transferred to the caller.

        @throw system_error Thrown on failure.
    */
    BOOST_JSON_DECL
    value
    release();
};

BOOST_JSON_NS_END

#endif
