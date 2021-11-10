
#ifndef BOOST_CONTRACT_EXCEPTION_HPP_
#define BOOST_CONTRACT_EXCEPTION_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Handle contract assertion failures.
*/

// IMPORTANT: Included by contract_macro.hpp so trivial headers only.
#include <boost/contract/core/config.hpp>
#include <boost/contract/detail/declspec.hpp> // No compile-time overhead.
#include <boost/function.hpp>
#include <boost/config.hpp>
#include <exception>
#include <string>

// NOTE: This code should not change (not even its impl) based on the
// CONTRACT_NO_... macros. For example, preconditions_failure() should still
// all the set precondition failure handler even when NO_PRECONDITIONS is
// #defined, because user code might explicitly call precondition_failure()
// (for whatever reason...). Otherwise, the public API of this lib will change.

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
// Needed for `std::` prefix to show (but removed via `EXCLUDE_SYMBOLS=std`).
namespace std {
    class exception {};
    class bad_cast {};
}
#endif

namespace boost { namespace contract {

/**
Public base class for all exceptions directly thrown by this library.

This class does not inherit from @c std::exception because exceptions deriving
from this class will do that (inheriting from @c std::exception,
@c std::bad_cast, etc.).

@see    @RefClass{boost::contract::assertion_failure},
        @RefClass{boost::contract::bad_virtual_result_cast},
        etc.
*/
class BOOST_CONTRACT_DETAIL_DECLSPEC exception {
public:
    /**
    Destruct this object.

    @b Throws: This is declared @c noexcept (or @c throw() before C++11).
    */
    virtual ~exception() /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */;
};

#ifdef BOOST_MSVC
    #pragma warning(push)
    #pragma warning(disable: 4275) // Bases w/o DLL spec (bad_cast, etc).
    #pragma warning(disable: 4251) // Members w/o DLL spec (string for what_).
#endif

/**
Exception thrown when inconsistent return values are passed to overridden
virtual public functions.

This exception is thrown when programmers pass to this library return value
parameters for public function overrides in derived classes that are not
consistent with the return type parameter passed for the virtual public function
being overridden from the base classes.
This allows this library to give more descriptive error messages in such cases
of misuse.

This exception is internally thrown by this library and programmers should not
need to throw it from user code.

@see    @RefSect{tutorial.public_function_overrides__subcontracting_,
        Public Function Overrides}
*/
class BOOST_CONTRACT_DETAIL_DECLSPEC bad_virtual_result_cast : // Copy (as str).
        public std::bad_cast, public boost::contract::exception {
public:
    /**
    Construct this object with the name of the from- and to- result types.

    @param from_type_name Name of the from-type (source of the cast).
    @param to_type_name Name of the to-type (destination of the cast).
    */
    explicit bad_virtual_result_cast(char const* from_type_name,
            char const* to_type_name);

    /**
    Destruct this object.

    @b Throws: This is declared @c noexcept (or @c throw() before C++11).
    */
    virtual ~bad_virtual_result_cast()
            /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */;

    /**
    Description for this error (containing both from- and to- type names).

    @b Throws: This is declared @c noexcept (or @c throw() before C++11).
    */
    virtual char const* what() const
            /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */;

/** @cond */
private:
    std::string what_;
/** @endcond */
};

/**
Exception typically used to report a contract assertion failure.

This exception is thrown by code expanded by @RefMacro{BOOST_CONTRACT_ASSERT}
(but it can also be thrown by user code programmed manually without that macro).
This exception is typically used to report contract assertion failures because
it contains detailed information about the file name, line number, and source
code of the asserted condition (so it can be used by this library to provide
detailed error messages when handling contract assertion failures).

However, any other exception can be used to report a contract assertion failure
(including user-defined exceptions).
This library will call the appropriate contract failure handler function
(@RefFunc{boost::contract::precondition_failure}, etc.) when this or any other
exception is thrown while checking contracts (by default, these failure handler
functions print an error message to @c std::cerr and terminate the program, but
they can be customized to take any other action).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}
*/
class BOOST_CONTRACT_DETAIL_DECLSPEC assertion_failure : // Copy (as str, etc.).
        public std::exception, public boost::contract::exception {
public:
    /**
    Construct this object with file name, line number, and source code text of
    an assertion condition (all optional).

    This constructor can also be used to specify no information (default
    constructor), or to specify only file name and line number but not source
    code text (because of the parameter default values).
    
    @param file Name of the file containing the assertion (usually set using
                <c>__FILE__</c>).
    @param line Number of the line containing the assertion (usually set using
                <c>__LINE__</c>).
    @param code Text listing the source code of the assertion condition.
    */
    explicit assertion_failure(char const* file = "", unsigned long line = 0,
            char const* code = "");

    /**
    Construct this object only with the source code text of the assertion
    condition.
    @param code Text listing the source code of the assertion condition.
    */
    explicit assertion_failure(char const* code);
    
    /**
    Destruct this object.

    @b Throws: This is declared @c noexcept (or @c throw() before C++11).
    */
    virtual ~assertion_failure()
            /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */;

    /**
    String describing the failed assertion.
    
    @b Throws: This is declared @c noexcept (or @c throw() before C++11).
    
    @return A string formatted similarly to the following:
      <c>assertion "`code()`" failed: file "`file()`", line \`line()\`</c>
            (where `` indicate execution quotes).
            File, line, and code will be omitted from this string if they were
            not specified when constructing this object.
    */
    virtual char const* what() const
            /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */;

    /**
    Name of the file containing the assertion.

    @return File name as specified at construction (or @c "" if no file was
            specified).
    */
    char const* file() const;
    
    /**
    Number of the line containing the assertion.

    @return Line number as specified at construction (or @c 0 if no line number
            was specified).
    */
    unsigned long line() const;
    
    /**
    Text listing the source code of the assertion condition.

    @return Assertion condition source code as specified at construction (or
            @c "" if no source code text was specified).
    */
    char const* code() const;

/** @cond */
private:
    void init();

    char const* file_;
    unsigned long line_;
    char const* code_;
    std::string what_;
/** @endcond */
};

#ifdef BOOST_MSVC
    #pragma warning(pop)
#endif

/**
Indicate the kind of operation where the contract assertion failed.

This is passed as a parameter to the assertion failure handler functions.
For example, it might be necessary to know in which operation an assertion
failed to make sure exceptions are never thrown from destructors, not even
when contract failure handlers are programmed by users to throw exceptions
instead of terminating the program.

@see @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure}
*/
enum from {
    /** Assertion failed when checking contracts for constructors. */
    from_constructor,
    
    /** Assertion failed when checking contracts for destructors . */
    from_destructor,
    
    /**
    Assertion failed when checking contracts for functions (members or not,
    public or not).
    */
    from_function
};

/**
Type of assertion failure handler functions (with @c from parameter).

Assertion failure handler functions specified by this type must be functors
returning @c void and taking a single parameter of type
@RefEnum{boost::contract::from}.
For example, this is used to specify contract failure handlers for class
invariants, preconditions, postconditions, and exception guarantees.

@see @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure}
*/
typedef boost::function<void (from)> from_failure_handler;

/**
Type of assertion failure handler functions (without @c from parameter).

Assertion failure handler functions specified by this type must be nullary
functors returning @c void.
For example, this is used to specify contract failure handlers for
implementation checks.

@see @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure}
*/
typedef boost::function<void ()> failure_handler;

/** @cond */
namespace exception_ {
    // Check failure.

    BOOST_CONTRACT_DETAIL_DECLSPEC
    failure_handler const& set_check_failure_unlocked(failure_handler const& f)
            BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    failure_handler const& set_check_failure_locked(failure_handler const& f)
            BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    failure_handler get_check_failure_unlocked() BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    failure_handler get_check_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    void check_failure_unlocked() /* can throw */;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    void check_failure_locked() /* can throw */;
    
    // Precondition failure.

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_pre_failure_unlocked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_pre_failure_locked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_pre_failure_unlocked() BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_pre_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    void pre_failure_unlocked(from where) /* can throw */;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    void pre_failure_locked(from where) /* can throw */;
    
    // Postcondition failure.

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_post_failure_unlocked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_post_failure_locked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_post_failure_unlocked() BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_post_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    void post_failure_unlocked(from where) /* can throw */;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    void post_failure_locked(from where) /* can throw */;
    
    // Except failure.

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_except_failure_unlocked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_except_failure_locked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_except_failure_unlocked()
            BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_except_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    void except_failure_unlocked(from where) /* can throw */;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    void except_failure_locked(from where) /* can throw */;
    
    // Old-copy failure.

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_old_failure_unlocked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_old_failure_locked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_old_failure_unlocked() BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_old_failure_locked() BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    void old_failure_unlocked(from where) /* can throw */;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    void old_failure_locked(from where) /* can throw */;
    
    // Entry invariant failure.

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_entry_inv_failure_unlocked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_entry_inv_failure_locked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_entry_inv_failure_unlocked()
            BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_entry_inv_failure_locked()
            BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    void entry_inv_failure_unlocked(from where) /* can throw */;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    void entry_inv_failure_locked(from where) /* can throw */;
    
    // Exit invariant failure.

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const& set_exit_inv_failure_unlocked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler const&set_exit_inv_failure_locked(
            from_failure_handler const& f) BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_exit_inv_failure_unlocked()
            BOOST_NOEXCEPT_OR_NOTHROW;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    from_failure_handler get_exit_inv_failure_locked()
            BOOST_NOEXCEPT_OR_NOTHROW;

    BOOST_CONTRACT_DETAIL_DECLSPEC
    void exit_inv_failure_unlocked(from where) /* can throw */;
    BOOST_CONTRACT_DETAIL_DECLSPEC
    void exit_inv_failure_locked(from where) /* can throw */;
}
/** @endcond */

} } // namespace

/** @cond */
#ifdef BOOST_CONTRACT_HEADER_ONLY
    // NOTE: This header must be included in the middle of this file (because
    // its impl depends on both from and assert_failure types). This is not
    // ideal, but it is better than splitting this file into multiple
    // independent ones because all content in this file is logically related
    // from the user prospective.
    #include <boost/contract/detail/inlined/core/exception.hpp>
#endif
/** @endcond */

namespace boost { namespace contract {
    
// Following must be inline for static linkage (no DYN_LINK and no HEADER_ONLY).

/**
Set failure handler for implementation checks.

Set a new failure handler and returns it.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@param f New failure handler functor to set.

@return Same failure handler functor @p f passed as parameter (e.g., for
        concatenating function calls).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{advanced.implementation_checks, Implementation Checks}
*/
inline failure_handler const& set_check_failure(failure_handler const& f)
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::set_check_failure_locked(f);
    #else
        return exception_::set_check_failure_unlocked(f);
    #endif
}

/**
Return failure handler currently set for implementation checks.

This is often called only internally by this library.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@return A copy of the failure handler currently set.

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{advanced.implementation_checks, Implementation Checks}
*/
inline failure_handler get_check_failure()
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::get_check_failure_locked();
    #else
        return exception_::get_check_failure_unlocked();
    #endif
}

/**
Call failure handler for implementation checks.

This is often called only internally by this library.

@b Throws:  This can throw in case programmers specify a failure handler that
            throws exceptions on implementation check failures (not the
            default).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{advanced.implementation_checks, Implementation Checks}
*/
inline void check_failure() /* can throw */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        exception_::check_failure_locked();
    #else
        exception_::check_failure_unlocked();
    #endif
}

/**
Set failure handler for preconditions.

Set a new failure handler and returns it.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@param f New failure handler functor to set.

@return Same failure handler functor @p f passed as parameter (e.g., for
        concatenating function calls).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.preconditions, Preconditions}
*/
inline from_failure_handler const& set_precondition_failure(from_failure_handler
        const& f) /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::set_pre_failure_locked(f);
    #else
        return exception_::set_pre_failure_unlocked(f);
    #endif
}

/**
Return failure handler currently set for preconditions.

This is often called only internally by this library.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@return A copy of the failure handler currently set.

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.preconditions, Preconditions}
*/
inline from_failure_handler get_precondition_failure()
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::get_pre_failure_locked();
    #else
        return exception_::get_pre_failure_unlocked();
    #endif
}

/**
Call failure handler for preconditions.

This is often called only internally by this library.

@b Throws:  This can throw in case programmers specify a failure handler that
            throws exceptions on contract assertion failures (not the default).

@param where    Operation that failed the contract assertion (when this function
                is called by this library, this parameter will never be
                @c from_destructor because destructors do not have
                preconditions).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.preconditions, Preconditions}
*/
inline void precondition_failure(from where) /* can throw */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        exception_::pre_failure_locked(where);
    #else
        exception_::pre_failure_unlocked(where);
    #endif
}

/**
Set failure handler for postconditions.

Set a new failure handler and returns it.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@param f New failure handler functor to set.

@return Same failure handler functor @p f passed as parameter (e.g., for
        concatenating function calls).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.postconditions, Postconditions}
*/
inline from_failure_handler const& set_postcondition_failure(
    from_failure_handler const& f
) /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::set_post_failure_locked(f);
    #else
        return exception_::set_post_failure_unlocked(f);
    #endif
}

/**
Return failure handler currently set for postconditions.

This is often called only internally by this library.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@return A copy of the failure handler currently set.

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.postconditions, Postconditions}
*/
inline from_failure_handler get_postcondition_failure()
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::get_post_failure_locked();
    #else
        return exception_::get_post_failure_unlocked();
    #endif
}

/**
Call failure handler for postconditions.

This is often called only internally by this library.

@b Throws:  This can throw in case programmers specify a failure handler that
            throws exceptions on contract assertion failures (not the default).

@param where    Operation that failed the contract assertion (e.g., this might
                be useful to program failure handler functors that never throw
                from destructors, not even when they are programmed by users to
                throw exceptions instead of terminating the program).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.postconditions, Postconditions}
*/
inline void postcondition_failure(from where) /* can throw */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        exception_::post_failure_locked(where);
    #else
        exception_::post_failure_unlocked(where);
    #endif
}

/**
Set failure handler for exception guarantees.

Set a new failure handler and returns it.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@param f New failure handler functor to set.

@return Same failure handler functor @p f passed as parameter (e.g., for
        concatenating function calls).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.exception_guarantees, Exception Guarantees}
*/
inline from_failure_handler const& set_except_failure(from_failure_handler
        const& f) /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::set_except_failure_locked(f);
    #else
        return exception_::set_except_failure_unlocked(f);
    #endif
}

/**
Return failure handler currently set for exception guarantees.

This is often called only internally by this library.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@return A copy of the failure handler currently set.

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.exception_guarantees, Exception Guarantees}
*/
inline from_failure_handler get_except_failure()
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::get_except_failure_locked();
    #else
        return exception_::get_except_failure_unlocked();
    #endif
}

/**
Call failure handler for exception guarantees.

This is often called only internally by this library.

@b Throws:  This can throw in case programmers specify a failure handler that
            throws exceptions on contract assertion failures (not the default),
            however:

@warning    When this failure handler is called there is already an active
            exception (the one that caused the exception guarantees to be
            checked in the first place).
            Therefore, programming this failure handler to throw yet another
            exception will force C++ to automatically terminate the program.

@param where Operation that failed the contract assertion.

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.exception_guarantees, Exception Guarantees}
*/
inline void except_failure(from where) /* can throw */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        exception_::except_failure_locked(where);
    #else
        exception_::except_failure_unlocked(where);
    #endif
}

/**
Set failure handler for old values copied at body.

Set a new failure handler and returns it.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@param f New failure handler functor to set.

@return Same failure handler functor @p f passed as parameter (e.g., for
        concatenating function calls).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{advanced.old_values_copied_at_body, Old Values Copied at Body}
*/
inline from_failure_handler const& set_old_failure(from_failure_handler const&
        f) /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::set_old_failure_locked(f);
    #else
        return exception_::set_old_failure_unlocked(f);
    #endif
}

/**
Return failure handler currently set for old values copied at body.

This is often called only internally by this library.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@return A copy of the failure handler currently set.

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{advanced.old_values_copied_at_body, Old Values Copied at Body}
*/
inline from_failure_handler get_old_failure()
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::get_old_failure_locked();
    #else
        return exception_::get_old_failure_unlocked();
    #endif
}

/**
Call failure handler for old values copied at body.

This is often called only internally by this library.

@b Throws:  This can throw in case programmers specify a failure handler that
            throws exceptions on contract assertion failures (not the default).

@param where    Operation that failed the old value copy (e.g., this might
                be useful to program failure handler functors that never throw
                from destructors, not even when they are programmed by users to
                throw exceptions instead of terminating the program).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{advanced.old_values_copied_at_body, Old Values Copied at Body}
*/
inline void old_failure(from where) /* can throw */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        exception_::old_failure_locked(where);
    #else
        exception_::old_failure_unlocked(where);
    #endif
}

/**
Set failure handler for class invariants at entry.

Set a new failure handler and returns it.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@param f New failure handler functor to set.

@return Same failure handler functor @p f passed as parameter (e.g., for
        concatenating function calls).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.class_invariants, Class Invariants},
        @RefSect{extras.volatile_public_functions,
        Volatile Public Functions}
*/
inline from_failure_handler const& set_entry_invariant_failure(
    from_failure_handler const& f
)/** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::set_entry_inv_failure_locked(f);
    #else
        return exception_::set_entry_inv_failure_unlocked(f);
    #endif
}

/**
Return failure handler currently set for class invariants at entry.

This is often called only internally by this library.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@return A copy of the failure handler currently set.

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.class_invariants, Class Invariants},
        @RefSect{extras.volatile_public_functions,
        Volatile Public Functions}
*/
inline from_failure_handler get_entry_invariant_failure()
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::get_entry_inv_failure_locked();
    #else
        return exception_::get_entry_inv_failure_unlocked();
    #endif
}

/**
Call failure handler for class invariants at entry.

This is often called only internally by this library.

@b Throws:  This can throw in case programmers specify a failure handler that
            throws exceptions on contract assertion failures (not the default).

@param where    Operation that failed the contract assertion (e.g., this might
                be useful to program failure handler functors that never throw
                from destructors, not even when they are programmed by users to
                throw exceptions instead of terminating the program).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.class_invariants, Class Invariants},
        @RefSect{extras.volatile_public_functions,
        Volatile Public Functions}
*/
inline void entry_invariant_failure(from where) /* can throw */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::entry_inv_failure_locked(where);
    #else
        return exception_::entry_inv_failure_unlocked(where);
    #endif
}

/**
Set failure handler for class invariants at exit.

Set a new failure handler and returns it.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@param f New failure handler functor to set.

@return Same failure handler functor @p f passed as parameter (e.g., for
        concatenating function calls).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.class_invariants, Class Invariants},
        @RefSect{extras.volatile_public_functions,
        Volatile Public Functions}
*/
inline from_failure_handler const& set_exit_invariant_failure(
    from_failure_handler const& f
) /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::set_exit_inv_failure_locked(f);
    #else
        return exception_::set_exit_inv_failure_unlocked(f);
    #endif
}

/**
Return failure handler currently set for class invariants at exit.

This is often called only internally by this library.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@return A copy of the failure handler currently set.

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.class_invariants, Class Invariants},
        @RefSect{extras.volatile_public_functions,
        Volatile Public Functions}
*/
inline from_failure_handler get_exit_invariant_failure()
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        return exception_::get_exit_inv_failure_locked();
    #else
        return exception_::get_exit_inv_failure_unlocked();
    #endif
}

/**
Call failure handler for class invariants at exit.

This is often called only internally by this library.

@b Throws:  This can throw in case programmers specify a failure handler that
            throws exceptions on contract assertion failures (not the default).

@param where    Operation that failed the contract assertion (e.g., this might
                be useful to program failure handler functors that never throw
                from destructors, not even when they are programmed by users to
                throw exceptions instead of terminating the program).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.class_invariants, Class Invariants},
        @RefSect{extras.volatile_public_functions,
        Volatile Public Functions}
*/
inline void exit_invariant_failure(from where) /* can throw */ {
    #ifndef BOOST_CONTRACT_DISABLE_THREADS
        exception_::exit_inv_failure_locked(where);
    #else
        exception_::exit_inv_failure_unlocked(where);
    #endif
}

/**
Set failure handler for class invariants (at both entry and exit).

This is provided for convenience and it is equivalent to call both
@RefFunc{boost::contract::set_entry_invariant_failure} and
@RefFunc{boost::contract::set_exit_invariant_failure} with the same functor
parameter @p f.

@b Throws: This is declared @c noexcept (or @c throw() before C++11).

@param f New failure handler functor to set for both entry and exit invariants.

@return Same failure handler functor @p f passed as parameter (e.g., for
        concatenating function calls).

@see    @RefSect{advanced.throw_on_failures__and__noexcept__, Throw on Failure},
        @RefSect{tutorial.class_invariants, Class Invariants},
        @RefSect{extras.volatile_public_functions,
        Volatile Public Functions}
*/
/** @cond */ BOOST_CONTRACT_DETAIL_DECLSPEC /** @endcond */
from_failure_handler const& set_invariant_failure(from_failure_handler const& f)
        /** @cond */ BOOST_NOEXCEPT_OR_NOTHROW /** @endcond */;

} } // namespace

#endif // #include guard

