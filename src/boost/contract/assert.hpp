
#ifndef BOOST_CONTRACT_ASSERT_HPP_
#define BOOST_CONTRACT_ASSERT_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Assert contract conditions.
*/

#include <boost/contract/core/config.hpp>
#include <boost/contract/detail/noop.hpp>

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Preferred way to assert contract conditions.

    Any exception thrown from within a contract (preconditions, postconditions,
    exception guarantees, old value copies at body, class invariants, etc.) is
    interpreted by this library as a contract failure.
    Therefore, users can program contract assertions manually throwing an
    exception when an asserted condition is checked to be @c false (this
    library will then call the appropriate contract failure handler
    @RefFunc{boost::contract::precondition_failure}, etc.).
    However, it is preferred to use this macro because it expands to
    code that throws @RefClass{boost::contract::assertion_failure} with the
    correct assertion file name (using <c>__FILE__</c>), line number (using
    <c>__LINE__</c>), and asserted condition code so to produce informative
    error messages (C++11 <c>__func__</c> is not used here because in most cases
    it will simply expand to the internal compiler name of the lambda function
    used to program the contract conditions adding no specificity to the error
    message).
    
    @RefMacro{BOOST_CONTRACT_ASSERT}, @RefMacro{BOOST_CONTRACT_ASSERT_AUDIT},
    and @RefMacro{BOOST_CONTRACT_ASSERT_AXIOM} are the three assertion levels
    predefined by this library.

    @see    @RefSect{tutorial.preconditions, Preconditions},
            @RefSect{tutorial.postconditions, Postconditions},
            @RefSect{tutorial.exception_guarantees, Exceptions Guarantees},
            @RefSect{tutorial.class_invariants, Class Invariants},
            @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}
    
    @param cond Boolean contract condition to check.
                (This is not a variadic macro parameter so any comma it might
                contain must be protected by round parenthesis and
                @c BOOST_CONTRACT_ASSERT((cond)) will always work.)
    */
    // This must be an expression (a trivial one so the compiler can optimize it
    // away). It cannot an empty code block `{}`, etc. otherwise code like
    // `if(...) ASSERT(...); else ASSERT(...);` won't work when NO_ALL.
    #define BOOST_CONTRACT_ASSERT(cond)
#elif !defined(BOOST_CONTRACT_NO_ALL)
    #include <boost/contract/detail/assert.hpp>
    #define BOOST_CONTRACT_ASSERT(cond) \
        BOOST_CONTRACT_DETAIL_ASSERT(cond) /* no `;`  here */
#else
    // This must be an expression (a trivial one so the compiler can optimize it
    // away). It cannot an empty code block `{}`, etc. otherwise code like
    // `if(...) ASSERT(...); else ASSERT(...);` won't work when NO_ALL.
    #define BOOST_CONTRACT_ASSERT(cond) \
        BOOST_CONTRACT_DETAIL_NOOP
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Preferred way to assert contract conditions that are computationally
    expensive, at least compared to the computational cost of executing the
    function body.

    The asserted condition will always be compiled and validated syntactically,
    but it will not be checked at run-time unless
    @RefMacro{BOOST_CONTRACT_AUDITS} is defined (undefined by default).
    This macro is defined by code equivalent to:

    @code
        #ifdef BOOST_CONTRACT_AUDITS
            #define BOOST_CONTRACT_ASSERT_AUDIT(cond) \
                BOOST_CONTRACT_ASSERT(cond)
        #else
            #define BOOST_CONTRACT_ASSERT_AUDIT(cond) \
                BOOST_CONTRACT_ASSERT(true || cond)
        #endif
    @endcode

    @RefMacro{BOOST_CONTRACT_ASSERT}, @RefMacro{BOOST_CONTRACT_ASSERT_AUDIT},
    and @RefMacro{BOOST_CONTRACT_ASSERT_AXIOM} are the three assertion levels
    predefined by this library.
    If there is a need, programmers are free to implement their own assertion
    levels defining macros similar to the one above.
    
    @see    @RefSect{extras.assertion_levels, Assertion Levels},
            @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}
    
    @param cond Boolean contract condition to check.
                (This is not a variadic macro parameter so any comma it might
                contain must be protected by round parenthesis and
                @c BOOST_CONTRACT_ASSERT_AUDIT((cond)) will always work.)
    */
    #define BOOST_CONTRACT_ASSERT_AUDIT(cond)
#elif defined(BOOST_CONTRACT_AUDITS)
    #define BOOST_CONTRACT_ASSERT_AUDIT(cond) \
        BOOST_CONTRACT_ASSERT(cond)
#else
    #define BOOST_CONTRACT_ASSERT_AUDIT(cond) \
        BOOST_CONTRACT_DETAIL_NOEVAL(cond)
#endif

/**
Preferred way to document in the code contract conditions that are
computationally prohibitive, at least compared to the computational cost of
executing the function body.

The asserted condition will always be compiled and validated syntactically, but
it will never be checked at run-time.
This macro is defined by code equivalent to:

@code
    #define BOOST_CONTRACT_ASSERT_AXIOM(cond) \
        BOOST_CONTRACT_ASSERT(true || cond)
@endcode

@RefMacro{BOOST_CONTRACT_ASSERT}, @RefMacro{BOOST_CONTRACT_ASSERT_AUDIT}, and
@RefMacro{BOOST_CONTRACT_ASSERT_AXIOM} are the three assertion levels predefined
by this library.
If there is a need, programmers are free to implement their own assertion levels
defining macros similar to the one above.

@see    @RefSect{extras.assertion_levels, Assertion Levels},
        @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}

@param cond Boolean contract condition to check.
            (This is not a variadic macro parameter so any comma it might
            contain must be protected by round parenthesis and
            @c BOOST_CONTRACT_ASSERT_AXIOM((cond)) will always work.)
*/
#define BOOST_CONTRACT_ASSERT_AXIOM(cond) \
    BOOST_CONTRACT_DETAIL_NOEVAL(cond)

#endif // #include guard

