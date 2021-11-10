//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_BASIC_PARSER_HPP
#define BOOST_JSON_BASIC_PARSER_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/error.hpp>
#include <boost/json/kind.hpp>
#include <boost/json/parse_options.hpp>
#include <boost/json/detail/stack.hpp>
#include <boost/json/detail/stream.hpp>
#include <boost/json/detail/utf8.hpp>

/*  VFALCO NOTE

    This file is in the detail namespace because it
    is not allowed to be included directly by users,
    who should be including <boost/json/basic_parser.hpp>
    instead, which provides the member function definitions.

    The source code is arranged this way to keep compile
    times down.
*/

BOOST_JSON_NS_BEGIN

/** An incremental SAX parser for serialized JSON.

    This implements a SAX-style parser, invoking a
    caller-supplied handler with each parsing event.
    To use, first declare a variable of type
    `basic_parser<T>` where `T` meets the handler
    requirements specified below. Then call
    @ref write_some one or more times with the input,
    setting `more = false` on the final buffer.
    The parsing events are realized through member
    function calls on the handler, which exists
    as a data member of the parser.
\n
    The parser may dynamically allocate intermediate
    storage as needed to accommodate the nesting level
    of the input JSON. On subsequent invocations, the
    parser can cheaply re-use this memory, improving
    performance. This storage is freed when the
    parser is destroyed

    @par Usage

    To get the declaration and function definitions
    for this class it is necessary to include this
    file instead:
    @code
    #include <boost/json/basic_parser_impl.hpp>
    @endcode

    Users who wish to parse JSON into the DOM container
    @ref value will not use this class directly; instead
    they will create an instance of @ref parser or
    @ref stream_parser and use that instead. Alternatively,
    they may call the function @ref parse. This class is
    designed for users who wish to perform custom actions
    instead of building a @ref value. For example, to
    produce a DOM from an external library.
\n
    @note

    By default, only conforming JSON using UTF-8
    encoding is accepted. However, select non-compliant
    syntax can be allowed by construction using a
    @ref parse_options set to desired values.

    @par Handler

    The handler provided must be implemented as an
    object of class type which defines each of the
    required event member functions below. The event
    functions return a `bool` where `true` indicates
    success, and `false` indicates failure. If the
    member function returns `false`, it must set
    the error code to a suitable value. This error
    code will be returned by the write function to
    the caller.
\n
    Handlers are required to declare the maximum
    limits on various elements. If these limits
    are exceeded during parsing, then parsing
    fails with an error.
\n
    The following declaration meets the parser's
    handler requirements:

    @code
    struct handler
    {
        /// The maximum number of elements allowed in an array
        static constexpr std::size_t max_array_size = -1;

        /// The maximum number of elements allowed in an object
        static constexpr std::size_t max_object_size = -1;

        /// The maximum number of characters allowed in a string
        static constexpr std::size_t max_string_size = -1;

        /// The maximum number of characters allowed in a key
        static constexpr std::size_t max_key_size = -1;

        /// Called once when the JSON parsing begins.
        ///
        /// @return `true` on success.
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_document_begin( error_code& ec );

        /// Called when the JSON parsing is done.
        ///
        /// @return `true` on success.
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_document_end( error_code& ec );

        /// Called when the beginning of an array is encountered.
        ///
        /// @return `true` on success.
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_array_begin( error_code& ec );

        /// Called when the end of the current array is encountered.
        ///
        /// @return `true` on success.
        /// @param n The number of elements in the array.
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_array_end( std::size_t n, error_code& ec );

        /// Called when the beginning of an object is encountered.
        ///
        /// @return `true` on success.
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_object_begin( error_code& ec );

        /// Called when the end of the current object is encountered.
        ///
        /// @return `true` on success.
        /// @param n The number of elements in the object.
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_object_end( std::size_t n, error_code& ec );

        /// Called with characters corresponding to part of the current string.
        ///
        /// @return `true` on success.
        /// @param s The partial characters
        /// @param n The total size of the string thus far
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_string_part( string_view s, std::size_t n, error_code& ec );

        /// Called with the last characters corresponding to the current string.
        ///
        /// @return `true` on success.
        /// @param s The remaining characters
        /// @param n The total size of the string
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_string( string_view s, std::size_t n, error_code& ec );

        /// Called with characters corresponding to part of the current key.
        ///
        /// @return `true` on success.
        /// @param s The partial characters
        /// @param n The total size of the key thus far
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_key_part( string_view s, std::size_t n, error_code& ec );

        /// Called with the last characters corresponding to the current key.
        ///
        /// @return `true` on success.
        /// @param s The remaining characters
        /// @param n The total size of the key
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_key( string_view s, std::size_t n, error_code& ec );

        /// Called with the characters corresponding to part of the current number.
        ///
        /// @return `true` on success.
        /// @param s The partial characters
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_number_part( string_view s, error_code& ec );

        /// Called when a signed integer is parsed.
        ///
        /// @return `true` on success.
        /// @param i The value
        /// @param s The remaining characters
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_int64( int64_t i, string_view s, error_code& ec );

        /// Called when an unsigend integer is parsed.
        ///
        /// @return `true` on success.
        /// @param u The value
        /// @param s The remaining characters
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_uint64( uint64_t u, string_view s, error_code& ec );

        /// Called when a double is parsed.
        ///
        /// @return `true` on success.
        /// @param d The value
        /// @param s The remaining characters
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_double( double d, string_view s, error_code& ec );

        /// Called when a boolean is parsed.
        ///
        /// @return `true` on success.
        /// @param b The value
        /// @param s The remaining characters
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_bool( bool b, error_code& ec );

        /// Called when a null is parsed.
        ///
        /// @return `true` on success.
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_null( error_code& ec );

        /// Called with characters corresponding to part of the current comment.
        ///
        /// @return `true` on success.
        /// @param s The partial characters.
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_comment_part( string_view s, error_code& ec );

        /// Called with the last characters corresponding to the current comment.
        ///
        /// @return `true` on success.
        /// @param s The remaining characters
        /// @param ec Set to the error, if any occurred.
        ///
        bool on_comment( string_view s, error_code& ec );
    };
    @endcode

    @see
        @ref parse,
        @ref stream_parser.

    @headerfile <boost/json/basic_parser.hpp>
*/
template<class Handler>
class basic_parser
{
    enum class state : char
    {
        doc1,  doc2,  doc3, doc4,
        com1,  com2,  com3, com4,
        nul1,  nul2,  nul3,
        tru1,  tru2,  tru3,
        fal1,  fal2,  fal3,  fal4,
        str1,  str2,  str3,  str4,
        str5,  str6,  str7,  str8,
        sur1,  sur2,  sur3,
        sur4,  sur5,  sur6,
        obj1,  obj2,  obj3,  obj4,
        obj5,  obj6,  obj7,  obj8,
        obj9,  obj10, obj11,
        arr1,  arr2,  arr3,
        arr4,  arr5,  arr6,
        num1,  num2,  num3,  num4,
        num5,  num6,  num7,  num8,
        exp1,  exp2,  exp3,
        val1,  val2
    };

    struct number
    {
        uint64_t mant;
        int bias;
        int exp;
        bool frac;
        bool neg;
    };

    // optimization: must come first
    Handler h_;

    number num_;
    error_code ec_;
    detail::stack st_;
    detail::utf8_sequence seq_;
    unsigned u1_;
    unsigned u2_;
    bool more_; // false for final buffer
    bool done_ = false; // true on complete parse
    bool clean_ = true; // write_some exited cleanly
    const char* end_;
    parse_options opt_;
    // how many levels deeper the parser can go
    std::size_t depth_ = opt_.max_depth;

    inline void reserve();
    inline const char* sentinel();
    inline bool incomplete(
        const detail::const_stream_wrapper& cs);

#ifdef __INTEL_COMPILER
#pragma warning push
#pragma warning disable 2196
#endif

    BOOST_NOINLINE
    inline
    const char*
    suspend_or_fail(state st);

    BOOST_NOINLINE
    inline
    const char*
    suspend_or_fail(
        state st,
        std::size_t n);

    BOOST_NOINLINE
    inline
    const char*
    fail(const char* p) noexcept;

    BOOST_NOINLINE
    inline
    const char*
    fail(
        const char* p,
        error ev) noexcept;

    BOOST_NOINLINE
    inline
    const char*
    maybe_suspend(
        const char* p,
        state st);

    BOOST_NOINLINE
    inline
    const char*
    maybe_suspend(
        const char* p,
        state st,
        std::size_t n);

    BOOST_NOINLINE
    inline
    const char*
    maybe_suspend(
        const char* p,
        state st,
        const number& num);

    BOOST_NOINLINE
    inline
    const char*
    suspend(
        const char* p,
        state st);

    BOOST_NOINLINE
    inline
    const char*
    suspend(
        const char* p,
        state st,
        const number& num);

#ifdef __INTEL_COMPILER
#pragma warning pop
#endif

    template<bool StackEmpty_/*, bool Terminal_*/>
    const char* parse_comment(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        /*std::integral_constant<bool, Terminal_>*/ bool terminal);

    template<bool StackEmpty_>
    const char* parse_document(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty);

    template<bool StackEmpty_, bool AllowComments_/*,
        bool AllowTrailing_, bool AllowBadUTF8_*/>
    const char* parse_value(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        std::integral_constant<bool, AllowComments_> allow_comments,
        /*std::integral_constant<bool, AllowTrailing_>*/ bool allow_trailing,
        /*std::integral_constant<bool, AllowBadUTF8_>*/ bool allow_bad_utf8);

    template<bool StackEmpty_, bool AllowComments_/*,
        bool AllowTrailing_, bool AllowBadUTF8_*/>
    const char* resume_value(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        std::integral_constant<bool, AllowComments_> allow_comments,
        /*std::integral_constant<bool, AllowTrailing_>*/ bool allow_trailing,
        /*std::integral_constant<bool, AllowBadUTF8_>*/ bool allow_bad_utf8);

    template<bool StackEmpty_, bool AllowComments_/*,
        bool AllowTrailing_, bool AllowBadUTF8_*/>
    const char* parse_object(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        std::integral_constant<bool, AllowComments_> allow_comments,
        /*std::integral_constant<bool, AllowTrailing_>*/ bool allow_trailing,
        /*std::integral_constant<bool, AllowBadUTF8_>*/ bool allow_bad_utf8);

    template<bool StackEmpty_, bool AllowComments_/*,
        bool AllowTrailing_, bool AllowBadUTF8_*/>
    const char* parse_array(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        std::integral_constant<bool, AllowComments_> allow_comments,
        /*std::integral_constant<bool, AllowTrailing_>*/ bool allow_trailing,
        /*std::integral_constant<bool, AllowBadUTF8_>*/ bool allow_bad_utf8);

    template<bool StackEmpty_>
    const char* parse_null(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty);

    template<bool StackEmpty_>
    const char* parse_true(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty);

    template<bool StackEmpty_>
    const char* parse_false(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty);

    template<bool StackEmpty_, bool IsKey_/*,
        bool AllowBadUTF8_*/>
    const char* parse_string(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        std::integral_constant<bool, IsKey_> is_key,
        /*std::integral_constant<bool, AllowBadUTF8_>*/ bool allow_bad_utf8);

    template<bool StackEmpty_, char First_>
    const char* parse_number(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        std::integral_constant<char, First_> first);

    template<bool StackEmpty_, bool IsKey_/*,
        bool AllowBadUTF8_*/>
    const char* parse_unescaped(const char* p,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        std::integral_constant<bool, IsKey_> is_key,
        /*std::integral_constant<bool, AllowBadUTF8_>*/ bool allow_bad_utf8);

    template<bool StackEmpty_/*, bool IsKey_,
        bool AllowBadUTF8_*/>
    const char* parse_escaped(
        const char* p,
        std::size_t total,
        std::integral_constant<bool, StackEmpty_> stack_empty,
        /*std::integral_constant<bool, IsKey_>*/ bool is_key,
        /*std::integral_constant<bool, AllowBadUTF8_>*/ bool allow_bad_utf8);

    // intentionally private
    std::size_t
    depth() const noexcept
    {
        return opt_.max_depth - depth_;
    }

public:
    /// Copy constructor (deleted)
    basic_parser(
        basic_parser const&) = delete;

    /// Copy assignment (deleted)
    basic_parser& operator=(
        basic_parser const&) = delete;

    /** Destructor.

        All dynamically allocated internal memory is freed.

        @par Effects
        @code
        this->handler().~Handler()
        @endcode

        @par Complexity
        Same as `~Handler()`.

        @par Exception Safety
        Same as `~Handler()`.
    */
    ~basic_parser() = default;

    /** Constructor.

        This function constructs the parser with
        the specified options, with any additional
        arguments forwarded to the handler's constructor.

        @par Complexity
        Same as `Handler( std::forward< Args >( args )... )`.

        @par Exception Safety
        Same as `Handler( std::forward< Args >( args )... )`.

        @param opt Configuration settings for the parser.
        If this structure is default constructed, the
        parser will accept only standard JSON.

        @param args Optional additional arguments
        forwarded to the handler's constructor.

        @see parse_options
    */
    template<class... Args>
    explicit
    basic_parser(
        parse_options const& opt,
        Args&&... args);

    /** Return a reference to the handler.

        This function provides access to the constructed
        instance of the handler owned by the parser.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    Handler&
    handler() noexcept
    {
        return h_;
    }

    /** Return a reference to the handler.

        This function provides access to the constructed
        instance of the handler owned by the parser.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    Handler const&
    handler() const noexcept
    {
        return h_;
    }

    /** Return the last error.

        This returns the last error code which
        was generated in the most recent call
        to @ref write_some.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    error_code
    last_error() const noexcept
    {
        return ec_;
    }

    /** Return true if a complete JSON has been parsed.

        This function returns `true` when all of these
        conditions are met:

        @li A complete serialized JSON has been
            presented to the parser, and

        @li No error or exception has occurred since the
            parser was constructed, or since the last call
            to @ref reset,

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    bool
    done() const noexcept
    {
        return done_;
    }

    /** Reset the state, to parse a new document.

        This function discards the current parsing
        state, to prepare for parsing a new document.
        Dynamically allocated temporary memory used
        by the implementation is not deallocated.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.
    */
    void
    reset() noexcept;

    /** Indicate a parsing failure.

        This changes the state of the parser to indicate
        that the parse has failed. A parser implementation
        can use this to fail the parser if needed due to
        external inputs.

        @note

        If `!ec`, the stored error code is unspecified.

        @par Complexity
        Constant.

        @par Exception Safety
        No-throw guarantee.

        @param ec The error code to set. If the code does
        not indicate failure, an implementation-defined
        error code that indicates failure will be stored
        instead.
    */
    void
    fail(error_code ec) noexcept;

    /** Parse some of an input string as JSON, incrementally.

        This function parses the JSON in the specified
        buffer, calling the handler to emit each SAX
        parsing event. The parse proceeds from the
        current state, which is at the beginning of a
        new JSON or in the middle of the current JSON
        if any characters were already parsed.
    \n
        The characters in the buffer are processed
        starting from the beginning, until one of the
        following conditions is met:

        @li All of the characters in the buffer
        have been parsed, or

        @li Some of the characters in the buffer
        have been parsed and the JSON is complete, or

        @li A parsing error occurs.

        The supplied buffer does not need to contain the
        entire JSON. Subsequent calls can provide more
        serialized data, allowing JSON to be processed
        incrementally. The end of the serialized JSON
        can be indicated by passing `more = false`.

        @par Complexity
        Linear in `size`.

        @par Exception Safety
        Basic guarantee.
        Calls to the handler may throw.
        Upon error or exception, subsequent calls will
        fail until @ref reset is called to parse a new JSON.

        @return The number of characters successfully
        parsed, which may be smaller than `size`.

        @param more `true` if there are possibly more
        buffers in the current JSON, otherwise `false`.

        @param data A pointer to a buffer of `size`
        characters to parse.

        @param size The number of characters pointed to
        by `data`.

        @param ec Set to the error, if any occurred.
    */
    std::size_t
    write_some(
        bool more,
        char const* data,
        std::size_t size,
        error_code& ec);
};

BOOST_JSON_NS_END

#endif
