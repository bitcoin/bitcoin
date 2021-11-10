
#ifndef BOOST_CONTRACT_CONFIG_HPP_
#define BOOST_CONTRACT_CONFIG_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Configure this library compile-time and run-time behaviours.
*/

// IMPORTANT: This header MUST NOT #include any other header of this lib.
// That way users can #include this header and not #include any of this lib
// headers after that depending on the contract 0/1 macros below ensuring no
// compilation overhead.

// Export symbols when compiling as shared lib (for internal use only). (Named
// after similar macros in all Boost libs.)
// BOOST_CONTRACT_SOURCE

// Disable automatic library selection for linking. (Named after similar macros
// in all Boost libs.)
// BOOST_CONTRACT_NO_LIB
// BOOST_ALL_NO_LIB

#if (!defined(BOOST_CONTRACT_DYN_LINK) && defined(BOOST_ALL_DYN_LINK)) || \
        defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
    /**
    Define this macro to compile this library as a shared library (recommended).
    
    If this macro is defined, this library is compiled so it can be linked
    as a shared library (a.k.a., Dynamically Linked Library or DLL) to user
    code.
    This library will automatically define this macro when Boost libraries are
    built as shared libraries (e.g., defining @c BOOST_ALL_DYN_LINK or using
    <c>bjam link=shared ...</c>).
    
    @warning    In general this library will correctly check contracts at
                run-time only when compiled as a shared library, unless  user
                code checks contracts in a single program unit (e.g., a single
                program with only statically linked libraries).
                Therefore, it is recommended to build and use this library as
                a shared library by defining this macro (or equivalently by
                building all Boost libraries as shared libraries).
                
    @see    @RefSect{getting_started, Getting Started}
    */
    #define BOOST_CONTRACT_DYN_LINK
#elif defined(BOOST_CONTRACT_DYN_LINK) && defined(BOOST_CONTRACT_STATIC_LINK)
    #error "DYN_LINK defined with STATIC_LINK"
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Define this macro to compile this library as a static library (not
    recommended).

    If this macro is defined, this library is compiled so it can be linked
    statically to user code.
    This library will automatically define this macro when Boost libraries
    are built as static libraries.

    @warning    This library is not guaranteed to always work correctly at
                run-time when this macro is defined (define
                @RefMacro{BOOST_CONTRACT_DYN_LINK} or @c BOOST_ALL_DYN_LINK
                instead).
                However, this macro can be defined and this library can be
                safely used as a static library for user code that checks
                contracts in a single program unit (e.g., a single program with
                only statically linked libraries).
                
    @see    @RefSect{getting_started, Getting Started}
    */
    #define BOOST_CONTRACT_STATIC_LINK
#elif defined(BOOST_CONTRACT_STATIC_LINK) && defined(BOOST_CONTRACT_DYN_LINK)
    #error "STATIC_LINK defined with DYN_LINK"
#endif

#ifdef BOOST_CONTRACT_HEADER_ONLY
    #error "leave DYN_LINK and STATIC_LINK undefined instead"
#elif   (!defined(BOOST_CONTRACT_DYN_LINK) && \
        !defined(BOOST_CONTRACT_STATIC_LINK)) || \
        defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
    /**
    Automatically defined by this library when it is being used as a header-only
    library (not recommended).

    This macro is not a configuration macro and this library will generate a
    compile-time error if users try to define it directly.
    This library will automatically define this macro when users do not define
    @RefMacro{BOOST_CONTRACT_DYN_LINK} (or @c BOOST_ALL_DYN_LINK) and
    @RefMacro{BOOST_CONTRACT_STATIC_LINK}.
    When used as a header-only library, this library code does not have to be
    compiled separately from user code, this library headers are simply included
    and compiled as part of the user program.
    
    @warning    This library is not guaranteed to always work correctly at
                run-time when this macro is defined (define
                @RefMacro{BOOST_CONTRACT_DYN_LINK} or @c BOOST_ALL_DYN_LINK
                instead).
                However, this macro can be defined and this library can be
                safely used as a header-only library for user code that checks
                contracts in a single program unit (e.g., a single program with
                only statically linked libraries).
                
    @see    @RefSect{getting_started, Getting Started}
    */
    #define BOOST_CONTRACT_HEADER_ONLY
#endif

#if     (!defined(BOOST_CONTRACT_DISABLE_THREADS) && \
        defined(BOOST_DISABLE_THREADS)) || \
        defined(BOOST_CONTRACT_DETAIL_DOXYGEN)
    /**
    Define this macro to not lock internal library data for thread safety
    (undefined by default).

    Defining this macro will make the library implementation code not thread
    safe so this macro should not be defined unless the library is being used by
    single-threaded applications only.
    This library will automatically define this macro when Boost libraries are
    built without threads (e.g., defining @c BOOST_DISABLE_THREADS).

    @note   When this macro is left undefined this library needs to internally
            use some sort of global lock (to ensure contract checking is
            globally disabled when other contracts are being checked and also to
            safely access failure handler functors).
            That could introduce an undesired amount of synchronization in some
            multi-threaded applications.
    
    @see @RefSect{contract_programming_overview.assertions, Assertions}
    */
    #define BOOST_CONTRACT_DISABLE_THREADS
#endif

#ifndef BOOST_CONTRACT_MAX_ARGS
    /**
    Maximum number of arguments for public function overrides on compilers that
    do not support variadic templates (default to @c 10).

    On compilers that do not support C++11 variadic templates, this macro is
    defined to the maximum number of arguments that public function overrides
    can have and pass to @RefFunc{boost::contract::public_function} (users can
    redefine this macro to a different value).
    On compilers that support variadic templates, this macro has no effect.

    @note   Regardless of the value of this macro and of compiler support for
            variadic templates, there might be an intrinsic limit of about 18
            arguments for public function overrides (because of similar limits
            in Boost.MPL and Boost.FunctionTypes internally used by this
            library).

    @see @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}
    */
    #define BOOST_CONTRACT_MAX_ARGS 10
#endif

#ifndef BOOST_CONTRACT_BASES_TYPEDEF
    /**
    Define the name of the base type @c typedef (@c base_types by default).

    This macro expands to the name of the @c typedef that lists the base
    classes for subcontracting via @RefMacro{BOOST_CONTRACT_BASE_TYPES}:

    @code
        class u
            #define BASES public b, private w
            : BASES
        {
            friend class boost::contract:access;

            typedef BOOST_CONTRACT_BASE_TYPES(BASES) BOOST_CONTRACT_TYPEDEF;
            #undef BASES

            ...
        };
    @endcode

    When used this way, users can redefine this macro if the @c typedef must
    have a name different from @c base_types (because of name clashes in user
    code, etc.).
    
    @see @RefSect{tutorial.base_classes__subcontracting_, Base Classes}
    */
    #define BOOST_CONTRACT_BASES_TYPEDEF base_types
#endif

#ifndef BOOST_CONTRACT_INVARIANT_FUNC
    /**
    Define the name of the class invariant member function (@c invariant by
    default).

    This macro expands to the name of the @c const and <c>const volatile</c>
    member functions that check class invariants and volatile class invariants
    respectively:

    @code
        class u {
            friend class boost::contract::access;

            void BOOST_CONTRACT_INVARIANT_FUNC() const {
                BOOST_CONTRACT_ASSERT(...);
                ...
            }
            
            void BOOST_CONTRACT_INVARIANT_FUNC() const volatile {
                BOOST_CONTRACT_ASSERT(...);
                ...
            }

            ...
        };
    @endcode

    When used this way, users can redefine this macro if the invariant functions
    must have a name different from @c invariant (because of name clashes in
    user code, etc.).
    
    @note   C++ does not allow to overload member functions based on the
            @c static classifier, so this macro must always be defined to be
            different than the function name defined for
            @RefMacro{BOOST_CONTRACT_STATIC_INVARIANT_FUNC}.
    
    @see    @RefSect{tutorial.class_invariants, Class Invariants},
            @RefSect{extras.volatile_public_functions,
            Volatile Public Functions}
    */
    #define BOOST_CONTRACT_INVARIANT_FUNC invariant
#endif

#ifndef BOOST_CONTRACT_STATIC_INVARIANT_FUNC
    /**
    Define the name of the static invariant member function (@c static_invariant
    by default).

    This macro expands to the name of the @c static member function that checks
    static class invariants:

    @code
        class u {
            friend class boost::contract::access;

            static void BOOST_CONTRACT_STATIC_INVARIANT_FUNC() {
                BOOST_CONTRACT_ASSERT(...);
                ...
            }

            ...
        };
    @endcode

    When used this way, users can redefine this macro if the static invariant
    function must have a name different from @c static_invariant (because of
    name clashes in user code, etc.).

    @note   C++ does not allow to overload member functions based on the
            @c static classifier, so this macro must always be defined to be
            different than the function name defined for
            @RefMacro{BOOST_CONTRACT_INVARIANT_FUNC}.
    
    @see    @RefSect{tutorial.class_invariants, Class Invariants}
    */
    #define BOOST_CONTRACT_STATIC_INVARIANT_FUNC static_invariant
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Disable some compile-time errors generated by this library (undefined by
    default).

    Defining this macro disables a number of static checks and related
    compile-time errors generated by this library, for example:

    @li The static invariant member function named as
        @c BOOST_CONTRACT_STATIC_INVARIANT_FUNC must be declared @c static.
    @li Non-static invariant member functions named as
        @c BOOST_CONTRACT_INVARIANT_FUNC must be declared either @c const,
        <c>const volatile</c>, or <c>volatile const</c>.
    @li Derived classes that program contracts for one or more public function
        overrides via @RefFunc{boost::contract::public_function} must also
        define the @RefMacro{BOOST_CONTRACT_BASE_TYPES} @c typedef.

    In general, it is not recommended to define this macro because these
    compile-time checks can guard against misuses of this library.
        
    @see    @RefSect{tutorial.class_invariants, Class Invariants},
            @RefSect{tutorial.base_classes__subcontracting_, Base Classes}
    */
    #define BOOST_CONTRACT_PERMISSIVE
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Code block to execute if contracts are not assigned to a
    @RefClass{boost::contract::check} variable (undefined and executes
    @c BOOST_ASSERT(false) by default).

    In general, there is a logic error in the program when contracts are not
    explicitly assigned to a local variable of type
    @RefClass{boost::contract::check} and without using C++11 @c auto
    declarations (because that is a misuse of this library).
    Therefore, by default (i.e., when this macro is not defined) this library
    calls <c>BOOST_ASSERT(false)</c> in those cases.
    If this macro is defined, this library will execute the code expanded by
    this macro instead of calling @c BOOST_ASSERT(false) (if programmers prefer
    to throw an exception, etc.).

    This macro can also be defined to be any block of code (and use empty curly
    brackets @c {} to generate no error, not recommended), for example (on GCC):
    @code
        gcc -DBOOST_CONTRACT_ON_MISSING_CHECK_DECL='{ throw std::logic_error("missing contract check declaration"); }' ...
    @endcode

    @see @RefSect{tutorial, Tutorial}
    */
    #define BOOST_CONTRACT_ON_MISSING_CHECK_DECL
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Define this macro to not disable other assertions while checking
    preconditions (undefined by default).
    
    Not disabling other assertions while checking preconditions can lead to
    infinite recursion in user code so by default this macro is not defined.

    However, the @RefSect{bibliography, [N1962]} proposal does not disable
    assertions while checking preconditions because arguments can reach the
    function body unchecked if assertions are disabled while checking
    preconditions (e.g., when these same functions bodies are called to check
    the preconditions in question).
    This macro can be defined to obtain the behaviour specified in
    @RefSect{bibliography, [N1962]} (at the risk of infinite recursion).

    @see    @RefSect{contract_programming_overview.feature_summary,
            Feature Summary}
    */
    #define BOOST_CONTRACT_PRECONDITIONS_DISABLE_NO_ASSERTION
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Define this macro to not disable any assertion while checking other
    assertions (undefined by default).
    
    Not disabling assertions while checking other assertions can lead to
    infinite recursion in user code so by default this macro is not defined.
    (Defining this macro automatically implies that other assertion checking is
    disabled while checking preconditions as if
    @RefMacro{BOOST_CONTRACT_PRECONDITIONS_DISABLE_NO_ASSERTION} was also
    defined.)

    @see    @RefSect{contract_programming_overview.feature_summary,
            Feature Summary}
    */
    #define BOOST_CONTRACT_ALL_DISABLE_NO_ASSERTION
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Define this macro to evaluate and check audit assertions at run-time
    (undefined by default).

    Audit assertions and implementation checks programmed via
    @RefMacro{BOOST_CONTRACT_ASSERT_AUDIT} and
    @RefMacro{BOOST_CONTRACT_CHECK_AUDIT} are always compiled and validated
    syntactically.
    However, they are not evaluated and checked at run-time unless
    this macro is defined (because these conditions can be computationally
    expensive, at least compared to the computational cost of executing the
    function body).

    @see @RefSect{extras.assertion_levels, Assertion Levels}
    */
    #define BOOST_CONTRACT_AUDITS
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    If defined, this library disables implementation checks (undefined by
    default).

    If this macro is defined, this library internal code is also optimized to
    reduce compile-time (not just run-time) overhead associated with
    implementation checks.
    In addition, users can manually program @c \#ifndef statements in their code
    using this macro to completely disable compilation of implementation checks
    or use @RefMacro{BOOST_CONTRACT_CHECK} (recommended).
    
    @see    @RefSect{advanced.implementation_checks,
            Implementation Checks},
            @RefSect{extras.disable_contract_checking,
            Disable Contract Checking},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_CHECKS
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    If defined, this library does not check preconditions (undefined by
    default).

    If this macro is defined, this library internal code is also optimized to
    reduce compile-time (not just run-time) overhead associated with
    checking preconditions.
    In addition, users can manually program @c \#ifndef statements in their code
    using this macro to completely disable compilation of preconditions or use
    the macros defined in @c boost/contract_macro.hpp (recommended only for
    applications where it is truly necessary to completely remove contract code
    compilation from production code).

    @see    @RefSect{tutorial.preconditions, Preconditions},
            @RefSect{extras.disable_contract_checking,
            Disable Contract Checking},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_PRECONDITIONS
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    If defined, this library does not check postconditions (undefined by
    default).

    If this macro is defined, this library internal code is also optimized to
    reduce compile-time (not just run-time) overhead associated with
    checking postconditions.
    In addition, users can manually program @c \#ifndef statements in their code
    using this macro to completely disable compilation of postconditions or use
    the macros defined in @c boost/contract_macro.hpp (recommended only for
    applications where it is truly necessary to completely remove contract code
    compilation from production code).

    It is necessary to disable both postconditions and exception guarantees
    defining @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS} and
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS} in order to disable old value copies
    (see @RefMacro{BOOST_CONTRACT_NO_OLDS}).

    @see    @RefSect{tutorial.postconditions, Postconditions},
            @RefSect{extras.disable_contract_checking,
            Disable Contract Checking},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_POSTCONDITIONS
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    If defined, this library does not check exception guarantees (undefined by
    default).

    If this macro is defined, this library internal code is also optimized to
    reduce compile-time (not just run-time) overhead associated with
    checking exception guarantees.
    In addition, users can manually program @c \#ifndef statements in their code
    using this macro to completely disable compilation of exception guarantees
    or use the macros defined in @c boost/contract_macro.hpp (recommended only
    for applications where it is truly necessary to completely remove contract
    code compilation from production code).
    
    It is necessary to disable both postconditions and exception guarantees
    defining @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS} and
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS} in order to disable old value copies
    (see @RefMacro{BOOST_CONTRACT_NO_OLDS}).
    
    @see    @RefSect{tutorial.exception_guarantees, Exception Guarantees},
            @RefSect{extras.disable_contract_checking,
            Disable Contract Checking},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_EXCEPTS
#endif

#if     defined(BOOST_CONTRACT_DETAIL_DOXYGEN) || \
        ( \
            !defined(BOOST_CONTRACT_NO_ENTRY_INVARIANTS) && \
             defined(BOOST_CONTRACT_NO_INVARIANTS) \
        )
    /**
    If defined, this library does not check class invariants at entry (undefined
    by default).

    If this macro is defined, this library internal code is also optimized to
    reduce compile-time (not just run-time) overhead associated with
    checking class invariants at entry.
    In addition, users can manually program @c \#ifndef statements in their code
    using this macro to completely disable compilation of entry class invariants
    or use the macros defined in @c boost/contract_macro.hpp (recommended only
    for applications where it is truly necessary to completely remove contract
    code compilation from production code).

    This macro is automatically defined when
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS} is defined.

    @see    @RefSect{tutorial.class_invariants, Class Invariants},
            @RefSect{extras.disable_contract_checking,
            Disable Contract Checking},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_ENTRY_INVARIANTS
#endif

#if     defined(BOOST_CONTRACT_DETAIL_DOXYGEN) || \
        ( \
            !defined(BOOST_CONTRACT_NO_EXIT_INVARIANTS) && \
             defined(BOOST_CONTRACT_NO_INVARIANTS) \
        )
    /**
    If defined, this library does not check class invariants at exit (undefined
    by default).

    If this macro is defined, this library internal code is also optimized to
    reduce compile-time (not just run-time) overhead associated with
    checking class invariants at exit.
    In addition, users can manually program @c \#ifndef statements in their code
    using this macro to completely disable compilation of exit class invariants
    or use the macros defined in @c boost/contract_macro.hpp (recommended only
    for applications where it is truly necessary to completely remove contract
    code compilation from production code).

    This macro is automatically defined when
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS} is defined.

    @see    @RefSect{tutorial.class_invariants, Class Invariants},
            @RefSect{extras.disable_contract_checking,
            Disable Contract Checking},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_EXIT_INVARIANTS
#endif

#if     !defined(BOOST_CONTRACT_NO_INVARIANTS) && \
         defined(BOOST_CONTRACT_NO_ENTRY_INVARIANTS) && \
         defined(BOOST_CONTRACT_NO_EXIT_INVARIANTS)
    /**
    If defined, this library does not check class invariants (undefined by
    default).

    If this macro is defined, this library internal code is also optimized to
    reduce compile-time (not just run-time) overhead associated with
    checking class invariants.
    In addition, users can manually program @c \#ifndef statements in their code
    using this macro to completely disable compilation of class invariants or
    use the macros defined in @c boost/contract_macro.hpp (recommended only for
    applications where it is truly necessary to completely remove contract code
    compilation from production code).

    Defining this macro is equivalent to defining both
    @RefMacro{BOOST_CONTRACT_NO_ENTRY_INVARIANTS} and
    @RefMacro{BOOST_CONTRACT_NO_EXIT_INVARIANTS}.

    @see    @RefSect{tutorial.class_invariants, Class Invariants},
            @RefSect{extras.disable_contract_checking,
            Disable Contract Checking},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_INVARIANTS
#endif

#ifdef BOOST_CONTRACT_NO_OLDS
    #error "define NO_POSTCONDITIONS and NO_EXCEPTS instead"
#elif   defined(BOOST_CONTRACT_NO_POSTCONDITIONS) && \
        defined(BOOST_CONTRACT_NO_EXCEPTS)
    /**
    Automatically defined by this library when old value copies are not to be
    performed.

    This macro is not a configuration macro and this library will generate a
    compile-time error if users try to define it directly.
    This library will automatically define this macro when users define both
    @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS} and
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS}.
    Users can manually program @c \#ifndef statements in their code using this
    macro to completely disable compilation of old value copies or use the
    macros defined in @c boost/contract_macro.hpp (recommended only for
    applications where it is truly necessary to completely remove contract code
    compilation from production code).

    @see    @RefSect{tutorial.old_values, Old Values},
            @RefSect{advanced.old_values_copied_at_body,
            Old Values Copied at Body},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_OLDS
#endif

// Ctor pre checked separately and outside RAII so not part of this #define.
#ifdef BOOST_CONTRACT_NO_CONSTRUCTORS
    #error "define NO_INVARIANTS, NO_POSTCONDITIONS, and NO_EXCEPTS instead"
#elif   defined(BOOST_CONTRACT_NO_INVARIANTS) && \
        defined(BOOST_CONTRACT_NO_POSTCONDITIONS) && \
        defined(BOOST_CONTRACT_NO_EXCEPTS)
    /**
    Automatically defined by this library when contracts are not checked for
    constructors.

    This macro is not a configuration macro and this library will generate a
    compile-time error if users try to define it directly.
    This library will automatically define this macro when users define all
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS},
    @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS}, and
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS}.
    Users can manually program @c \#ifndef statements in their code using this
    macro to completely disable compilation of contracts for constructors or use
    the macros defined in @c boost/contract_macro.hpp (recommended only for
    applications where it is truly necessary to completely remove contract code
    compilation from production code).
    
    @note   Constructor preconditions are checked separately by
            @RefClass{boost::contract::constructor_precondition} so they are
            disabled by @RefMacro{BOOST_CONTRACT_NO_PRECONDITIONS} instead.

    @see    @RefSect{tutorial.constructors, Constructors},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_CONSTRUCTORS
#endif

#ifdef BOOST_CONTRACT_NO_DESTRUCTORS
    #error "define NO_INVARIANTS, NO_POSTCONDITIONS, and NO_EXCEPTS instead"
#elif   defined(BOOST_CONTRACT_NO_INVARIANTS) && \
        defined(BOOST_CONTRACT_NO_POSTCONDITIONS) && \
        defined(BOOST_CONTRACT_NO_EXCEPTS)
    /**
    Automatically defined by this library when contracts are not checked for
    destructors.

    This macro is not a configuration macro and this library will generate a
    compile-time error if users try to define it directly.
    This library will automatically define this macro when users define all
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS},
    @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS}, and
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS}.
    Users can manually program @c \#ifndef statements in their code using this
    macro to completely disable compilation of contracts for destructors or use
    the macros defined in @c boost/contract_macro.hpp (recommended only for
    applications where it is truly necessary to completely remove contract code
    compilation from production code).
    
    @see    @RefSect{tutorial.destructors, Destructors},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_DESTRUCTORS
#endif

#ifdef BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS
    #error "define NO_INVARIANTS, NO_PRECONDITIONS, NO_POSTCONDITIONS, and NO_EXCEPTS instead"
#elif   defined(BOOST_CONTRACT_NO_INVARIANTS) && \
        defined(BOOST_CONTRACT_NO_PRECONDITIONS) && \
        defined(BOOST_CONTRACT_NO_POSTCONDITIONS) && \
        defined(BOOST_CONTRACT_NO_EXCEPTS)
    /**
    Automatically defined by this library when contracts are not checked for
    public functions.

    This macro is not a configuration macro and this library will generate a
    compile-time error if users try to define it directly.
    This library will automatically define this macro when users define all
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS},
    @RefMacro{BOOST_CONTRACT_NO_PRECONDITIONS},
    @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS}, and
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS}.
    Users can manually program @c \#ifndef statements in their code using this
    macro to completely disable compilation of contracts for public functions or
    use the macros defined in @c boost/contract_macro.hpp (recommended only for
    applications where it is truly necessary to completely remove contract code
    compilation from production code).
    
    @see    @RefSect{tutorial.public_functions, Public Functions},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS
#endif
    
#ifdef BOOST_CONTRACT_NO_FUNCTIONS
    #error "define NO_PRECONDITIONS, NO_POSTCONDITIONS, and NO_EXCEPTS instead"
#elif   defined(BOOST_CONTRACT_NO_PRECONDITIONS) && \
        defined(BOOST_CONTRACT_NO_POSTCONDITIONS) && \
        defined(BOOST_CONTRACT_NO_EXCEPTS)
    /**
    Automatically defined by this library when contracts are not checked for
    non-member, private, or protected functions.

    This macro is not a configuration macro and this library will generate a
    compile-time error if users try to define it directly.
    This library will automatically define this macro when users define all
    @RefMacro{BOOST_CONTRACT_NO_PRECONDITIONS},
    @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS}, and
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS}.
    Users can manually program @c \#ifndef statements in their code using this
    macro to completely disable compilation of contracts for non-member,
    private and protected functions, or use the macros defined in
    @c boost/contract_macro.hpp (recommended only for applications where it is
    truly necessary to completely remove contract code compilation from
    production code).

    This macro is also used when contracts are not checked for private or
    protected functions, lambda functions, code blocks, loops, etc.

    @see    @RefSect{tutorial.non_member_functions, Non-Member Functions},
            @RefSect{advanced.private_and_protected_functions,
            Private and Protected Functions},
            @RefSect{advanced.lambdas__loops__code_blocks__and__constexpr__,
            Lambdas\, Loops\, Code Blocks},
            @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_FUNCTIONS
#endif

#ifdef BOOST_CONTRACT_NO_CONDITIONS
    #error "define NO_INVARIANTS, NO_PRECONDITIONS, NO_POSTCONDITIONS, and NO_EXCEPTS instead"
#elif   defined(BOOST_CONTRACT_NO_INVARIANTS) && \
        defined(BOOST_CONTRACT_NO_PRECONDITIONS) && \
        defined(BOOST_CONTRACT_NO_POSTCONDITIONS) && \
        defined(BOOST_CONTRACT_NO_EXCEPTS)
    /**
    Automatically defined by this library when contracts are not checked for
    preconditions, postconditions, exceptions guarantees, and class invariants
    (excluding implementation checks).

    This macro is not a configuration macro and this library will generate a
    compile-time error if users try to define it directly.
    This library will automatically define this macro when users define all
    @RefMacro{BOOST_CONTRACT_NO_PRECONDITIONS},
    @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS},
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS}, and
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS}.
    Users can manually program @c \#ifndef statements in their code using this
    macro to completely disable compilation of contracts within specifications
    (so excluding implementation checks which are contracts within
    implementations instead), or use the macros defined in
    @c boost/contract_macro.hpp (recommended only for applications where it is
    truly necessary to completely remove contract code compilation from
    production code).

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_CONDITIONS
#endif

#ifdef BOOST_CONTRACT_NO_ALL
    #error "define NO_INVARIANTS, NO_PRECONDITIONS, NO_POSTCONDITIONS, NO_EXCEPTS, and NO_CHECKS instead"
#elif   defined(BOOST_CONTRACT_NO_INVARIANTS) && \
        defined(BOOST_CONTRACT_NO_PRECONDITIONS) && \
        defined(BOOST_CONTRACT_NO_POSTCONDITIONS) && \
        defined(BOOST_CONTRACT_NO_EXCEPTS) && \
        defined(BOOST_CONTRACT_NO_CHECKS)
    /**
    Automatically defined by this library when contracts are not checked at all
    (neither for specifications nor for implementations).

    This macro is not a configuration macro and this library will generate a
    compile-time error if users try to define it directly.
    This library will automatically define this macro when users define all
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS},
    @RefMacro{BOOST_CONTRACT_NO_PRECONDITIONS},
    @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS},
    @RefMacro{BOOST_CONTRACT_NO_EXCEPTS}, and
    @RefMacro{BOOST_CONTRACT_NO_CHECKS}.
    For example, users can manually program @c \#ifndef statements in their code
    using this macro to avoid including the @c boost/contract.hpp header all
    together:

    @code
    #include <boost/contract/core/config.hpp>
    #ifndef BOOST_CONTRACT_NO_ALL
        #include <boost/contract.hpp>
    #endif
    @endcode
    
    Or, use the @c boost/contract_macro.hpp header and related macros instead
    (because the @c boost/contract_macro.hpp header is already optimized to not
    include other headers from this library when contracts are not checked, but
    recommended only for applications where it is truly necessary to completely
    remove contract code compilation from production code).

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation}
    */
    #define BOOST_CONTRACT_NO_ALL
#endif

#endif // #include guard

