//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_VALUE_STACK_HPP
#define BOOST_JSON_VALUE_STACK_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/error.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>
#include <stddef.h>

BOOST_JSON_NS_BEGIN

//----------------------------------------------------------

/** A stack of @ref value elements, for building a document.

    This stack of @ref value allows iterative
    construction of a JSON document in memory.
    The implementation uses temporary internal
    storage to buffer elements so that arrays, objects,
    and strings in the document are constructed using a
    single memory allocation. This improves performance
    and makes efficient use of the @ref memory_resource
    used to create the resulting @ref value.

    Temporary storage used by the implementation
    initially comes from an optional memory buffer
    owned by the caller. If that storage is exhausted,
    then memory is obtained dynamically from the
    @ref memory_resource provided on construction.

    @par Usage

    Construct the stack with an optional initial
    temporary buffer, and a @ref storage_ptr to use for
    more storage when the initial buffer is exhausted.
    Then to build a @ref value, first call @ref reset
    and optionally specify the @ref memory_resource
    which will be used for the value. Then push elements
    onto the stack by calling the corresponding functions.
    After the document has been fully created, call
    @ref release to acquire ownership of the top-level
    @ref value.

    @par Performance

    The initial buffer and any dynamically allocated
    temporary buffers are retained until the stack
    is destroyed. This improves performance when using
    a single stack instance to produce multiple
    values.

    @par Example

    The following code constructs a @ref value which
    when serialized produces a JSON object with three
    elements. It uses a local buffer for the temporary
    storage, and a separate local buffer for the storage
    of the resulting value. No memory is dynamically
    allocated; this shows how to construct a value
    without using the heap.

    @code

    // This example builds a json::value without any dynamic memory allocations:

    // Construct the value stack using a local buffer
    unsigned char temp[4096];
    value_stack st( storage_ptr(), temp, sizeof(temp) );

    // Create a static resource with a local initial buffer
    unsigned char buf[4096];
    static_resource mr( buf, sizeof(buf) );

    // All values on the stack will use `mr`
    st.reset(&mr);

    // Push the key/value pair "a":1.
    st.push_key("a");
    st.push_int64(1);

    // Push "b":null
    st.push_key("b");
    st.push_null();

    // Push "c":"hello"
    st.push_key("c");
    st.push_string("hello");

    // Pop the three key/value pairs and push an object with those three values.
    st.push_object(3);

    // Pop the object from the stack and take ownership.
    value jv = st.release();

    assert( serialize(jv) == "{\"a\":1,\"b\":null,\"c\":\"hello\"}" );

    // At this point we could re-use the stack by calling reset

    @endcode

    @par Thread Safety

    Distinct instances may be accessed concurrently.
    Non-const member functions of a shared instance
    may not be called concurrently with any other
    member functions of that instance.
*/
class value_stack
{
    class stack
    {
        enum
        {
            min_size_ = 16
        };

        storage_ptr sp_;
        void* temp_;
        value* begin_;
        value* top_;
        value* end_;
        // string starts at top_+1
        std::size_t chars_ = 0;
        bool run_dtors_ = true;

    public:
        inline ~stack();
        inline stack(
            storage_ptr sp,
            void* temp, std::size_t size) noexcept;
        inline void run_dtors(bool b) noexcept;
        inline std::size_t size() const noexcept;
        inline bool has_chars();

        inline void clear() noexcept;
        inline void maybe_grow();
        inline void grow_one();
        inline void grow(std::size_t nchars);

        inline void append(string_view s);
        inline string_view release_string() noexcept;
        inline value* release(std::size_t n) noexcept;
        template<class... Args> value& push(Args&&... args);
        template<class Unchecked> void exchange(Unchecked&& u);
    };

    stack st_;
    storage_ptr sp_;

public:
    /// Copy constructor (deleted)
    value_stack(
        value_stack const&) = delete;

    /// Copy assignment (deleted)
    value_stack& operator=(
        value_stack const&) = delete;

    /** Destructor.

        All dynamically allocated memory and
        partial or complete elements is freed.

        @par Complexity
        Linear in the size of partial results.

        @par Exception Safety
        No-throw guarantee.
    */
    BOOST_JSON_DECL
    ~value_stack();

    /** Constructor.

        Constructs an empty stack. Before any
        @ref value can be built, the function
        @ref reset must be called.

        The `sp` parameter is only used to allocate
        intermediate storage; it will not be used
        for the @ref value returned by @ref release.

        @param sp A pointer to the @ref memory_resource
        to use for intermediate storage allocations. If
        this argument is omitted, the default memory
        resource is used.

        @param temp_buffer A pointer to a caller-owned
        buffer which will be used to store temporary
        data used while building the value. If this
        pointer is null, the builder will use the
        storage pointer to allocate temporary data.

        @param temp_size The number of valid bytes of
        storage pointed to by `temp_buffer`.
    */
    BOOST_JSON_DECL
    value_stack(
        storage_ptr sp = {},
        unsigned char* temp_buffer = nullptr,
        std::size_t temp_size = 0) noexcept;

    /** Prepare to build a new document.

        This function must be called before constructing
        a new top-level @ref value. Any previously existing
        partial or complete elements are destroyed, but
        internal dynamically allocated memory is preserved
        which may be reused to build new values.

        @par Exception Safety

        No-throw guarantee.

        @param sp A pointer to the @ref memory_resource
        to use for top-level @ref value and all child
        values. The stack will acquire shared ownership
        of the memory resource until @ref release or
        @ref reset is called, or when the stack is
        destroyed.
    */
    BOOST_JSON_DECL
    void
    reset(storage_ptr sp = {}) noexcept;

    /** Return the top-level @ref value.

        This function transfers ownership of the
        constructed top-level value to the caller.
        The behavior is undefined if there is not
        a single, top-level element.

        @par Exception Safety

        No-throw guarantee.

        @return A __value__ holding the result.
        Ownership of this value is transferred
        to the caller. Ownership of the memory
        resource used in the last call to @ref reset
        is released.
    */
    BOOST_JSON_DECL
    value
    release() noexcept;

    //--------------------------------------------

    /** Push an array formed by popping `n` values from the stack.

        This function pushes an @ref array value
        onto the stack. The array is formed by first
        popping the top `n` values from the stack.
        If the stack contains fewer than `n` values,
        or if any of the top `n` values on the stack
        is a key, the behavior is undefined.

        @par Example

        The following statements produce an array
        with the contents 1, 2, 3:

        @code

        value_stack st;

        // reset must be called first or else the behavior is undefined
        st.reset();

        // Place three values on the stack
        st.push_int64( 1 );
        st.push_int64( 2 );
        st.push_int64( 3 );

        // Remove the 3 values, and push an array with those 3 elements on the stack
        st.push_array( 3 );

        // Pop the object from the stack and take ownership.
        value jv = st.release();

        assert( serialize(jv) == "[1,2,3]" );

        // At this point, reset must be called again to use the stack

        @endcode

        @param n The number of values to pop from the
        top of the stack to form the array.
    */
    BOOST_JSON_DECL
    void
    push_array(std::size_t n);

    /** Push an object formed by popping `n` key/value pairs from the stack.

        This function pushes an @ref object value
        onto the stack. The object is formed by first
        popping the top `n` key/value pairs from the
        stack. If the stack contains fewer than `n`
        key/value pairs, or if any of the top `n` key/value
        pairs on the stack does not consist of exactly one
        key followed by one value, the behavior is undefined.

        @note

        A key/value pair is formed by pushing a key, and then
        pushing a value.

        @par Example

        The following code creates an object on the stack
        with a single element, where key is "x" and value
        is true:

        @code

        value_stack st;

        // reset must be called first or else the behavior is undefined
        st.reset();

        // Place a key/value pair onto the stack
        st.push_key( "x" );
        st.push_bool( true );

        // Replace the key/value pair with an object containing a single element
        st.push_object( 1 );

        // Pop the object from the stack and take ownership.
        value jv = st.release();

        assert( serialize(jv) == "{\"x\",true}" );

        // At this point, reset must be called again to use the stack

        @endcode

        @par Duplicate Keys

        If there are object elements with duplicate keys;
        that is, if multiple elements in an object have
        keys that compare equal, only the last equivalent
        element will be inserted.

        @param n The number of key/value pairs to pop from the
        top of the stack to form the array.
    */
    BOOST_JSON_DECL
    void
    push_object(std::size_t n);

    /** Push part of a key or string onto the stack.

        This function pushes the characters in `s` onto
        the stack, appending to any existing characters
        or creating new characters as needed. Once a
        string part is placed onto the stack, the only
        valid stack operations are:

        @li @ref push_chars to append additional
        characters to the key or string being built,

        @li @ref push_key or @ref push_string to
        finish building the key or string and place
        the value onto the stack.

        @par Exception Safety

        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param s The characters to append. This may be empty.
    */
    BOOST_JSON_DECL
    void
    push_chars(
        string_view s);

    /** Push a key onto the stack.

        This function notionally removes all the
        characters currently on the stack, then
        pushes a @ref value containing a key onto
        the stack formed by appending `s` to the
        removed characters.

        @par Exception Safety

        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param s The characters to append. This may be empty.
    */
    BOOST_JSON_DECL
    void
    push_key(
        string_view s);

    /** Place a string value onto the stack.

        This function notionally removes all the
        characters currently on the stack, then
        pushes a @ref value containing a @ref string
        onto the stack formed by appending `s` to the
        removed characters.

        @par Exception Safety

        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param s The characters to append. This may be empty.
    */
    BOOST_JSON_DECL
    void
    push_string(
        string_view s);

    /** Push a number onto the stack

        This function pushes a number value onto the stack.

        @par Exception Safety

        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param i The number to insert.
    */
    BOOST_JSON_DECL
    void
    push_int64(
        int64_t i);

    /** Push a number onto the stack

        This function pushes a number value onto the stack.

        @par Exception Safety

        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param u The number to insert.
    */
    BOOST_JSON_DECL
    void
    push_uint64(
        uint64_t u);

    /** Push a number onto the stack

        This function pushes a number value onto the stack.

        @par Exception Safety

        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param d The number to insert.
    */
    BOOST_JSON_DECL
    void
    push_double(
        double d);

    /** Push a `bool` onto the stack

        This function pushes a boolean value onto the stack.

        @par Exception Safety

        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.

        @param b The boolean to insert.
    */
    BOOST_JSON_DECL
    void
    push_bool(
        bool b);

    /** Push a null onto the stack

        This function pushes a boolean value onto the stack.

        @par Exception Safety

        Basic guarantee.
        Calls to `memory_resource::allocate` may throw.
    */
    BOOST_JSON_DECL
    void
    push_null();
};

BOOST_JSON_NS_END

#endif
