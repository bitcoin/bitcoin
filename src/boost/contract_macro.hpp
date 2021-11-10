
#ifndef BOOST_CONTRACT_MACRO_HPP_
#define BOOST_CONTRACT_MACRO_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Allow to disable contracts to completely remove their compile-time and run-time
overhead.
This header automatically includes all header files <c>boost/contract/\*.hpp</c>
necessary to use its macros.

Almost all the macros defined in this header file are variadic macros. On
compilers that do not support variadic macros, programmers can manually code
<c>\#ifndef BOOST_CONTRACT_NO_...</c> statements instead (see
@RefSect{extras.disable_contract_compilation__macro_interface_,
Disable Contract Compilation}).
*/

// IMPORTANT: Following headers can always be #included (without any #if-guard)
// because they expand to trivial code that does not affect compile-time. These
// headers must always be #included here (without any #if-guard) because they
// define types and macros that are typically left in user code even when
// contracts are disables (these types and macros never affect run-time and
// their definitions are trivial when contracts are disabled so their impact on
// compile-time is negligible).
#include <boost/contract/override.hpp>
#include <boost/contract/base_types.hpp>
#include <boost/contract/core/constructor_precondition.hpp>
#include <boost/contract/core/check_macro.hpp>
#include <boost/contract/core/access.hpp>
#include <boost/contract/core/virtual.hpp>
#include <boost/contract/core/exception.hpp>
#include <boost/contract/core/config.hpp>

#ifndef BOOST_CONTRACT_NO_CONDITIONS
    #include <boost/contract/assert.hpp>
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program preconditions that can be completely disabled at compile-time.

    @c BOOST_CONTRACT_PRECONDITION(f) expands to code equivalent to the
    following (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_PRECONDITIONS} is defined):
    
    @code
    #ifndef BOOST_CONTRACT_NO_PRECONDITIONS
        .precondition(f)
    #endif
    @endcode
    
    Where:
    
    @arg    <c><b>f</b></c> is the nullay functor called by this library to
            check preconditions @c f().
            Assertions within this functor are usually programmed using
            @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a call
            to this functor indicates a contract assertion failure (and will
            result in this library calling
            @RefFunc{boost::contract::precondition_failure}).
            This functor should capture variables by (constant) value, or better
            by (constant) reference (to avoid extra copies).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.preconditions, Preconditions}
    */
    #define BOOST_CONTRACT_PRECONDITION(...)
#elif !defined(BOOST_CONTRACT_NO_PRECONDITIONS)
    #define BOOST_CONTRACT_PRECONDITION(...) .precondition(__VA_ARGS__)
#else
    #define BOOST_CONTRACT_PRECONDITION(...) /* nothing */
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program postconditions that can be completely disabled at compile-time.

    @c BOOST_CONTRACT_POSTCONDITION(f) expands to code equivalent to the
    following (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_POSTCONDITIONS} is defined):
    
    @code
    #ifndef BOOST_CONTRACT_NO_POSTCONDITIONS
        .postcondition(f)
    #endif
    @endcode
    
    Where:

    @arg    <c><b>f</b></c> is the functor called by this library to check
            postconditions @c f() or @c f(result).
            Assertions within this functor are usually programmed using
            @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a call
            to this functor indicates a contract assertion failure (and will
            result in this library calling
            @RefFunc{boost::contract::postcondition_failure}).
            This functor should capture variables by (constant) references (to
            access the values they will have at function exit).
            This functor takes the return value (preferably by <c>const&</c>)
            @c result as its one single parameter @c f(result) but only for
            virtual public functions and public functions overrides, otherwise
            it takes no parameter @c f().
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.postconditions, Postconditions}
    */
    #define BOOST_CONTRACT_POSTCONDITION(...)
#elif !defined(BOOST_CONTRACT_NO_POSTCONDITIONS)
    #define BOOST_CONTRACT_POSTCONDITION(...) .postcondition(__VA_ARGS__)
#else
    #define BOOST_CONTRACT_POSTCONDITION(...) /* nothing */
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program exception guarantees that can be completely disabled at
    compile-time.
    
    @c BOOST_CONTRACT_EXCEPT(f) expands to code equivalent to the following
    (note that no code is generated when @RefMacro{BOOST_CONTRACT_NO_EXCEPTS}
    is defined):
    
    @code
    #ifndef BOOST_CONTRACT_NO_EXCEPTS
        .except(f)
    #endif
    @endcode
    
    Where:
    
    @arg    <c><b>f</b></c> is the nullary functor called by this library to
            check exception guarantees @c f().
            Assertions within this functor are usually programmed using
            @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a call
            to this functor indicates a contract assertion failure (and will
            result in this library calling
            @RefFunc{boost::contract::except_failure}).
            This functor should capture variables by (constant) references (to
            access the values they will have at function exit).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.exception_guarantees, Exception Guarantees}
    */
    #define BOOST_CONTRACT_EXCEPT(...)
#elif !defined(BOOST_CONTRACT_NO_EXCEPTS)
    #define BOOST_CONTRACT_EXCEPT(...) .except(__VA_ARGS__)
#else
    #define BOOST_CONTRACT_EXCEPT(...) /* nothing */
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program old value copies at body that can be completely disabled at
    compile-time.

    @c BOOST_CONTRACT_OLD(f) expands to code equivalent to the following (note
    that no code is generated when @RefMacro{BOOST_CONTRACT_NO_OLDS} is
    defined):
    
    @code
    #ifndef BOOST_CONTRACT_NO_OLDS
        .old(f)
    #endif
    @endcode
    
    Where:

    @arg    <c><b>f</b></c> is the nullary functor called by this library
            @c f() to assign old value copies just before the body is execute
            but after entry invariants (when they apply) and preconditions are
            checked.
            Old value pointers within this functor call are usually assigned
            using @RefMacro{BOOST_CONTRACT_OLDOF}.
            Any exception thrown by a call to this functor will result in
            this library calling @RefFunc{boost::contract::old_failure} (because
            old values could not be copied to check postconditions and exception
            guarantees).
            This functor should capture old value pointers by references so they
            can be assigned (all other variables needed to evaluate old value
            expressions can be captured by (constant) value, or better by
            (constant) reference to avoid extra copies).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{advanced.old_values_copied_at_body,
            Old Values Copied at Body}
    */
    #define BOOST_CONTRACT_OLD(...)

    /**
    Program old values that can be completely disabled at compile-time for old
    value types that are required to be copyable.

    This is used to program old value copies for copyable types:

    @code
    class u {
    public:
        void f(...) {
            BOOST_CONTRACT_OLD_PTR(old_type_a)(old_var_a); // Null...
            BOOST_CONTRACT_OLD_PTR(old_type_b)(old_var_b, old_expr_b); // Set.
            BOOST_CONTRACT_PUBLIC_FUNCTION(this)
                ...
                BOOST_CONTRACT_OLD([&] {
                    old_var_a = BOOST_CONTRACT_OLDOF(old_expr_a); // ...set.
                    ...
                })
                ...
            ;

            ... // Function body.
        }

        virtual void g(..., boost::contract::virtual_* v = 0) {
            BOOST_CONTRACT_OLD_PTR(old_type_a)(old_var_a); // No `v`
            BOOST_CONTRACT_OLD_PTR(old_type_b)(v, old_var_b, old_expr_b); // `v`
            BOOST_CONTRACT_PUBLIC_FUNCTION(v, this)
                ...
                BOOST_CONTRACT_OLD([&] {
                    old_var_a = BOOST_CONTRACT_OLDOF(v, old_expr_a); // `v`
                    ...
                })
                ...
            ;

            ... // Function body.
        }

        ...
    };
    @endcode
    
    This is an overloaded variadic macro and it can be used in the following
    different ways (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_OLDS} is defined).

    1\. <c>BOOST_CONTRACT_OLD_PTR(old_type)(old_var)</c> expands to code
        equivalent to the following (this leaves the old value pointer null):

    @code
    #ifndef BOOST_CONTRACT_NO_OLDS
        // This declaration does not need to use `v`.
        boost::contract::old_ptr<old_type> old_var
    #endif
    @endcode
    
    2\. <c>BOOST_CONTRACT_OLD_PTR(old_type)(old_var, old_expr)</c> expands to
        code equivalent to the following (this initializes the pointer to the
        old value copy, but not to be used for virtual public functions and
        public function overrides):
    
    @code
    #ifndef BOOST_CONTRACT_NO_OLDS
        boost::contract::old_ptr<old_type> old_var =
                BOOST_CONTRACT_OLDOF(old_expr)
    #endif
    @endcode
    
    3\. <c>BOOST_CONTRACT_OLD_PTR(old_type)(v, old_var, old_expr)</c> expands to
        code equivalent to the following (this initializes the pointer to the
        old value copy for virtual public functions and public function
        overrides):

    @code
    #ifndef BOOST_CONTRACT_NO_OLDS
        boost::contract::old_ptr<old_type> old_var =
                BOOST_CONTRACT_OLDOF(v, old_expr)
    #endif
    @endcode
    
    Where:
    
    @arg    <c><b>old_type</b></c> is the type of the pointed old value.
            This type must be copyable (i.e.,
            <c>boost::contract::is_old_value_copyable<old_type>::value</c> is
            @c true), otherwise this pointer will always be null and this
            library will generate a compile-time error when the pointer is
            dereferenced (see @RefMacro{BOOST_CONTRACT_OLD_PTR_IF_COPYABLE}).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)
            (Rationale: Template parameters like this one are specified to
            this library macro interface using their own set of parenthesis
            <c>SOME_MACRO(template_params)(other_params)</c>.)
    @arg    <c><b>v</b></c> is the extra training parameter of type
            @RefClass{boost::contract::virtual_}<c>*</c> and default value @c 0
            from the enclosing virtual public function or public function
            override declaring the contract.
            (This is not a variadic macro parameter but it should never contain
            commas because it is an identifier.)
    @arg    <c><b>old_var</b></c> is the name of the old value pointer variable.
            (This is not a variadic macro parameter but it should never contain
            commas because it is an identifier.)
    @arg    <c><b>old_expr</b></c> is the expression to be evaluated and copied
            in the old value pointer.
            (This is not a variadic macro parameter so any comma it might
            contain must be protected by round parenthesis and
            <c>BOOST_CONTRACT_OLD_PTR(old_type)(v, old_var, (old_expr))</c>
            will always work.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.old_values, Old Values}
    */
    #define BOOST_CONTRACT_OLD_PTR(...)

    /**
    Program old values that can be completely disabled at compile-time for old
    value types that are not required to be copyable.
    
    This is used to program old value copies for types that might or might not
    be copyable:

    @code
    template<typename T> // Type `T` might or not be copyable.
    class u {
    public:
        void f(...) {
            BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(old_type_a)(old_var_a);
            BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(old_type_b)(old_var_b,
                    old_expr_b);
            BOOST_CONTRACT_PUBLIC_FUNCTION(this)
                ...
                BOOST_CONTRACT_OLD([&] {
                    old_var_a = BOOST_CONTRACT_OLDOF(old_expr_a);
                    ...
                })
                ... // In postconditions or exception guarantees:
                    if(old_var_a) ... // Always null for non-copyable types.
                    if(old_var_b) ... // Always null for non-copyable types.
                ...
            ;

            ... // Function body.
        }

        virtual void g(..., boost::contract::virtual_* v = 0) {
            BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(old_type_a)(old_var_a);
            BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(old_type_b)(v, old_var_b,
                    old_expr_b);
            BOOST_CONTRACT_PUBLIC_FUNCTION(v, this)
                ...
                BOOST_CONTRACT_OLD([&] {
                    old_var_a = BOOST_CONTRACT_OLDOF(v, old_expr_a);
                    ...
                })
                ... // In postconditions or exception guarantees:
                    if(old_var_a) ... // Always null for non-copyable types.
                    if(old_var_b) ... // Always null for non-copyable types.
                ...
            ;

            ... // Function body.
        }

        ...
    };
    @endcode
    
    This is an overloaded variadic macro and it can be used in the following
    different ways (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_OLDS} is defined).

    1\. <c>BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(old_type)(old_var)</c> expands to
        code equivalent to the following (this leaves the old value pointer
        null):

    @code
    #ifndef BOOST_CONTRACT_NO_OLDS
        // This declaration does not need to use `v`.
        boost::contract::old_ptr_if_copyable<old_type> old_var
    #endif
    @endcode
    
    2\. <c>BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(old_type)(old_var, old_expr)</c>
        expands to code equivalent to the following (this initializes the
        pointer to the old value copy, but not to be used for virtual public
        functions and public function overrides):
    
    @code
    #ifndef BOOST_CONTRACT_NO_OLDS
        boost::contract::old_ptr_if_copyable<old_type> old_var =
                BOOST_CONTRACT_OLDOF(old_expr)
    #endif
    @endcode
    
    3\. <c>BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(old_type)(v, old_var,
        old_expr)</c> expands to code equivalent to the following (this
        initializes the pointer to the old value copy for virtual public
        functions and public function overrides):

    @code
    #ifndef BOOST_CONTRACT_NO_OLDS
        boost::contract::old_ptr_if_copyable<old_type> old_var =
                BOOST_CONTRACT_OLDOF(v, old_expr)
    #endif
    @endcode
    
    Where:
    
    @arg    <c><b>old_type</b></c> is the type of the pointed old value.
            If this type is not copyable (i.e.,
            <c>boost::contract::is_old_value_copyable<old_type>::value</c> is
            @c false), this pointer will always be null, but this library will
            not generate a compile-time error when this pointer is dereferenced
            (see @RefMacro{BOOST_CONTRACT_OLD_PTR}).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)
    @arg    <c><b>v</b></c> is the extra trailing parameter of type
            @RefClass{boost::contract::virtual_}<c>*</c> and default value @c 0
            from the enclosing virtual public function or public function
            override declaring the contract.
            (This is not a variadic macro parameter but it should never contain
            commas because it is an identifier.)
    @arg    <c><b>old_var</b></c> is the name of the old value pointer variable.
            (This is not a variadic macro parameter but it should never contain
            commas because it is an identifier.)
    @arg    <c><b>old_expr</b></c> is the expression to be evaluated and copied
            in the old value pointer.
            (This is not a variadic macro parameter so any comma it might
            contain must be protected by round parenthesis and
            <c>BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(old_type)(v, old_var,
            (old_expr))</c> will always work.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{extras.old_value_requirements__templates_,
            Old Value Requirements}
    */
    #define BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(...)
#elif !defined(BOOST_CONTRACT_NO_OLDS)
    #include <boost/contract/old.hpp>
    #include <boost/preprocessor/facilities/overload.hpp>
    #include <boost/preprocessor/facilities/empty.hpp>
    #include <boost/preprocessor/cat.hpp>

    /* PRIVATE */

    #define BOOST_CONTRACT_OLD_VAR_1(ptr) \
        ptr
    #define BOOST_CONTRACT_OLD_VAR_2(ptr, expr) \
        ptr = BOOST_CONTRACT_OLDOF(expr)
    #define BOOST_CONTRACT_OLD_VAR_3(v, ptr, expr) \
        ptr = BOOST_CONTRACT_OLDOF(v, expr)

    #define BOOST_CONTRACT_OLD_VAR_(...) \
        BOOST_PP_CAT(BOOST_PP_OVERLOAD(BOOST_CONTRACT_OLD_VAR_, __VA_ARGS__) \
                (__VA_ARGS__), BOOST_PP_EMPTY())

    /* PUBLIC */
    
    #define BOOST_CONTRACT_OLD(...) .old(__VA_ARGS__)

    #define BOOST_CONTRACT_OLD_PTR(...) \
        boost::contract::old_ptr< __VA_ARGS__ > \
        BOOST_CONTRACT_OLD_VAR_

    #define BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(...) \
        boost::contract::old_ptr_if_copyable< __VA_ARGS__ > \
        BOOST_CONTRACT_OLD_VAR_
#else
    #include <boost/preprocessor/tuple/eat.hpp>
   
    #define BOOST_CONTRACT_OLD(...) /* nothing */

    #define BOOST_CONTRACT_OLD_PTR(...) BOOST_PP_TUPLE_EAT(0)
    
    #define BOOST_CONTRACT_OLD_PTR_IF_COPYABLE(...) BOOST_PP_TUPLE_EAT(0)
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program (constant) class invariants that can be completely disabled at
    compile-time.

    @c BOOST_CONTRACT_INVARIANT({ ... }) expands to code equivalent to the
    following (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS} is defined):

    @code
        #ifndef BOOST_CONTRACT_NO_INVARIANTS
            void BOOST_CONTRACT_INVARIANT_FUNC() const {
                ...
            }
        #endif
    @endcode
    
    Where:
    
    @arg    <b>{ ... }</b> is the definition of the function that checks class
            invariants for public functions that are not static and not volatile
            (see @RefMacro{BOOST_CONTRACT_STATIC_INVARIANT} and
            @RefMacro{BOOST_CONTRACT_INVARIANT_VOLATILE}).
            The curly parenthesis are mandatory (rationale: this is so the
            syntax of this macro resembles mote the syntax of the lambda
            functions usually used to specify preconditions, etc.).
            Assertions within this function are usually programmed using
            @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a call
            to this function indicates a contract assertion failure (and will
            result in this library calling either
            @RefFunc{boost::contract::entry_invariant_failure} or
            @RefFunc{boost::contract::exit_invariant_failure}).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.class_invariants, Class Invariants}
    */
    #define BOOST_CONTRACT_INVARIANT(...)

    /**
    Program volatile class invariants that can be completely disabled at
    compile-time.

    @c BOOST_CONTRACT_INVARIANT_VOLATILE({ ... }) expands to code equivalent to
    the following (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS} is defined):

    @code
        #ifndef BOOST_CONTRACT_NO_INVARIANTS
            void BOOST_CONTRACT_INVARIANT_FUNC() const volatile {
                ...
            }
        #endif
    @endcode
    
    Where:
    
    @arg    <b>{ ... }</b> is the definition of the function that checks class
            invariants for volatile public functions
            (see @RefMacro{BOOST_CONTRACT_INVARIANT} and
            @RefMacro{BOOST_CONTRACT_STATIC_INVARIANT}).
            The curly parenthesis are mandatory.
            Assertions within this function are usually programmed using
            @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a call
            to this function indicates a contract assertion failure (and will
            result in this library calling either
            @RefFunc{boost::contract::entry_invariant_failure} or
            @RefFunc{boost::contract::exit_invariant_failure}).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{extras.volatile_public_functions,
            Volatile Public Functions}
    */
    #define BOOST_CONTRACT_INVARIANT_VOLATILE(...)
    
    /**
    Program static class invariants that can be completely disabled at
    compile-time.

    @c BOOST_CONTRACT_STATIC_INVARIANT({ ... }) expands to code equivalent to
    the following (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_INVARIANTS} is defined):

    @code
        #ifndef BOOST_CONTRACT_NO_INVARIANTS
            static void BOOST_CONTRACT_STATIC_INVARIANT_FUNC() {
                ...
            }
        #endif
    @endcode
    
    Where:
    
    @arg    <b>{ ... }</b> is the definition of the function that checks class
            invariants for static public functions
            (see @RefMacro{BOOST_CONTRACT_INVARIANT} and
            @RefMacro{BOOST_CONTRACT_INVARIANT_VOLATILE}).
            The curly parenthesis are mandatory.
            Assertions within this function are usually programmed using
            @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a call
            to this function indicates a contract assertion failure (and will
            result in this library calling either
            @RefFunc{boost::contract::entry_invariant_failure} or
            @RefFunc{boost::contract::exit_invariant_failure}).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.class_invariants, Class Invariants}
    */
    #define BOOST_CONTRACT_STATIC_INVARIANT(...)
#elif !defined(BOOST_CONTRACT_NO_INVARIANTS)
    #include <boost/contract/core/config.hpp>

    #define BOOST_CONTRACT_INVARIANT(...) \
        void BOOST_CONTRACT_INVARIANT_FUNC() const __VA_ARGS__

    #define BOOST_CONTRACT_INVARIANT_VOLATILE(...) \
        void BOOST_CONTRACT_INVARIANT_FUNC() const volatile __VA_ARGS__
    
    #define BOOST_CONTRACT_STATIC_INVARIANT(...) \
        static void BOOST_CONTRACT_STATIC_INVARIANT_FUNC() __VA_ARGS__
#else
    #define BOOST_CONTRACT_INVARIANT(...) /* nothing */

    #define BOOST_CONTRACT_INVARIANT_VOLATILE(...) /* nothing */
    
    #define BOOST_CONTRACT_STATIC_INVARIANT(...) /* nothing */
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program contracts that can be completely disabled at compile-time for
    constructors.
    
    This is used together with @RefMacro{BOOST_CONTRACT_POSTCONDITION},
    @RefMacro{BOOST_CONTRACT_EXCEPT}, and @RefMacro{BOOST_CONTRACT_OLD} to
    specify postconditions, exception guarantees, and old value copies at body
    that can be completely disabled at compile-time for constructors (see
    @RefMacro{BOOST_CONTRACT_CONSTRUCTOR_PRECONDITION} to specify preconditions
    for constructors):

    @code
    class u {
        friend class boost::contract::access;

        BOOST_CONTRACT_INVARIANT({ // Optional (as for static and volatile).
            BOOST_CONTRACT_ASSERT(...);
            ...
        })

    public:
        u(...) {
            BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
            BOOST_CONTRACT_CONSTRUCTOR(this)
                // No `PRECONDITION` (use `CONSTRUCTOR_PRECONDITION` if needed).
                BOOST_CONTRACT_OLD([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(old_epxr);
                    ...
                })
                BOOST_CONTRACT_POSTCONDITION([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                BOOST_CONTRACT_EXCEPT([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ; // Trailing `;` is required.

            ... // Constructor body.
        }

        ...
    };
    @endcode

    For optimization, this can be omitted for constructors that do not have
    postconditions and exception guarantees, within classes that have no
    invariants.
            
    @c BOOST_CONTRACT_CONSTRUCTOR(obj) expands to code equivalent to the
    following (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_CONSTRUCTORS} is defined):

    @code
        #ifndef BOOST_CONTRACT_NO_CONSTRUCTORS
            boost::contract::check internal_var =
                    boost::contract::constructor(obj)
        #endif
    @endcode

    Where:
    
    @arg    <c><b>obj</b></c> is the object @c this from the scope of the
            enclosing constructor declaring the contract.
            Constructors check all class invariants, including static and
            volatile invariants (see @RefMacro{BOOST_CONTRACT_INVARIANT},
            @RefMacro{BOOST_CONTRACT_STATIC_INVARIANT}, and
            @RefMacro{BOOST_CONTRACT_INVARIANT_VOLATILE}).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)
    @arg    <c><b>internal_var</b></c> is a variable name internally generated
            by this library (this name is unique but only on different line
            numbers so this macro cannot be expanded multiple times on the same
            line).

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.constructors, Constructors}
    */
    #define BOOST_CONTRACT_CONSTRUCTOR(...)
#elif !defined(BOOST_CONTRACT_NO_CONSTRUCTORS)
    #include <boost/contract/constructor.hpp>
    #include <boost/contract/check.hpp>
    #include <boost/contract/detail/name.hpp>

    #define BOOST_CONTRACT_CONSTRUCTOR(...) \
        boost::contract::check BOOST_CONTRACT_DETAIL_NAME2(c, __LINE__) = \
                boost::contract::constructor(__VA_ARGS__)
#else
    #define BOOST_CONTRACT_CONSTRUCTOR(...) /* nothing */
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program preconditions that can be disabled at compile-time for constructors.
            
    This is used together with @RefMacro{BOOST_CONTRACT_CONSTRUCTOR} to specify
    contracts for constructors.
    Constructors that do not have preconditions do not use this macro.
    When at least one of the class constructors uses this macro,
    @RefClass{boost::contract::constructor_precondition} must be the first and
    private base of the class declaring the constructor for which preconditions
    are programmed:

    @code
    class u
        #define BASES private boost::contract::constructor_precondition<u>, \
                public b
        : BASES
    {
        friend class boost::contract::access;

        typedef BOOST_CONTRACT_BASE_TYPES(BASES) base_types;
        #undef BASES

        ...

    public:
        explicit u(unsigned x) :
            BOOST_CONTRACT_CONSTRUCTOR_PRECONDITION(u)([&] {
                BOOST_CONTRACT_ASSERT(x != 0);
            }),
            b(1 / x)
        {
            ...
        }

        ...
    };
    @endcode

    <c>BOOST_CONTRACT_CONSTRUCTOR_PRECONDITION(class_type)(f)</c> expands
    to code equivalent to the following (note that when
    @RefMacro{BOOST_CONTRACT_NO_PRECONDITIONS} is defined, this macro trivially
    expands to a default constructor call that is internally implemented to do
    nothing so this should have minimal to no overhead):

    @code
    // Guarded only by NO_PRECONDITIONS (and not also by NO_CONSTRUCTORS)
    // because for constructor's preconditions (not for postconditions, etc.).
    #ifndef BOOST_CONTRACT_NO_PRECONDITIONS
        boost::contract::constructor_precondition<class_type>(f)
    #else // No-op call (likely optimized away, minimal to no overhead).
        boost::contract::constructor_precondition<class_type>()
    #endif
    
    @endcode
    
    Where:

    @arg    <c><b>class_type</b></c> is the type of the class containing the
            constructor for which preconditions are being programmed.
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)
    @arg    <c><b>f</b></c> is the nullary functor called by this library to
            check constructor preconditions @c f().
            Assertions within this functor call are usually programmed using
            @RefMacro{BOOST_CONTRACT_ASSERT}, but any exception thrown by a call
            to this functor indicates a contract failure (and will result in
            this library calling
            @RefFunc{boost::contract::precondition_failure}).
            This functor should capture variables by (constant) value, or better
            by (constant) reference to avoid extra copies.
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.constructors, Constructors}
    */
    #define BOOST_CONTRACT_CONSTRUCTOR_PRECONDITION(...)
#elif !defined(BOOST_CONTRACT_NO_PRECONDITIONS) // Not NO_CONSTRUCTORS here.
    // constructor_precondition.hpp already #included at top.

    #define BOOST_CONTRACT_CONSTRUCTOR_PRECONDITION(...) \
        boost::contract::constructor_precondition< __VA_ARGS__ >
#else
    #include <boost/preprocessor/tuple/eat.hpp>
    // constructor_precondition.hpp always #included at top of this file.

    #define BOOST_CONTRACT_CONSTRUCTOR_PRECONDITION(...) \
        /* always use default ctor (i.e., do nothing) */ \
        boost::contract::constructor_precondition< __VA_ARGS__ >() \
        BOOST_PP_TUPLE_EAT(0)
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program contracts that can be completely disabled at compile-time for
    destructors.
    
    This is used together with @RefMacro{BOOST_CONTRACT_POSTCONDITION},
    @RefMacro{BOOST_CONTRACT_EXCEPT}, and @RefMacro{BOOST_CONTRACT_OLD} to
    specify postconditions, exception guarantees, and old value copies at body
    that can be completely disabled at compile-time for destructors (destructors
    cannot have preconditions, see
    @RefSect{contract_programming_overview.destructor_calls, Destructor Calls}):

    @code
    class u {
        friend class boost::contract::access;

        BOOST_CONTRACT_INVARIANT({ // Optional (as for static and volatile).
            BOOST_CONTRACT_ASSERT(...);
            ...
        })

    public:
        ~u() {
            BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
            BOOST_CONTRACT_DESTRUCTOR(this)
                // No `PRECONDITION` (destructors have no preconditions).
                BOOST_CONTRACT_OLD([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(old_expr);
                    ...
                })
                BOOST_CONTRACT_POSTCONDITION([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                BOOST_CONTRACT_EXCEPT([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ; // Trailing `;` is required.

            ... // Destructor body.
        }
        
        ...
    };
    @endcode

    For optimization, this can be omitted for destructors that do not have
    postconditions and exception guarantees, within classes that have no
    invariants.
    
    @c BOOST_CONTRACT_DESTRUCTOR(obj) expands to code equivalent to the
    following (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_DESTRUCTORS} is defined):
    
    @code
        #ifndef BOOST_CONTRACT_NO_DESTRUCTORS
            boost::contract::check internal_var =
                    boost::contract::destructor(obj)
        #endif
    @endcode

    Where:
    
    @arg    <c><b>obj</b></c> is the object @c this from the scope of the
            enclosing destructor declaring the contract.
            Destructors check all class invariants, including static and
            volatile invariants (see @RefSect{tutorial.class_invariants,
            Class Invariants} and
            @RefSect{extras.volatile_public_functions,
            Volatile Public Functions}).
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)
    @arg    <c><b>internal_var</b></c> is a variable name internally generated
            by this library (this name is unique but only on different line
            numbers so this macro cannot be expanded multiple times on the same
            line).

    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.destructors, Destructors}
    */
    #define BOOST_CONTRACT_DESTRUCTOR(...)
#elif !defined(BOOST_CONTRACT_NO_DESTRUCTORS)
    #include <boost/contract/destructor.hpp>
    #include <boost/contract/check.hpp>
    #include <boost/contract/detail/name.hpp>

    #define BOOST_CONTRACT_DESTRUCTOR(...) \
        boost::contract::check BOOST_CONTRACT_DETAIL_NAME2(c, __LINE__) = \
                boost::contract::destructor(__VA_ARGS__)
#else
    #define BOOST_CONTRACT_DESTRUCTOR(...) /* nothing */
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program contracts that can be completely disabled at compile-time for
    (non-public) functions.
    
    This is used together with @RefMacro{BOOST_CONTRACT_PRECONDITION},
    @RefMacro{BOOST_CONTRACT_POSTCONDITION}, @RefMacro{BOOST_CONTRACT_EXCEPT},
    and @RefMacro{BOOST_CONTRACT_OLD} to specify preconditions, postconditions,
    exception guarantees, and old value copies at body that can be completely
    disabled at compile-time for (non-public) functions:
    
    @code
    void f(...) {
        BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
        BOOST_CONTRACT_FUNCTION()
            BOOST_CONTRACT_PRECONDITION([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
            BOOST_CONTRACT_OLD([&] { // Optional.
                old_var = BOOST_CONTRACT_OLDOF(old_expr);  
                ...
            })
            BOOST_CONTRACT_POSTCONDITION([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
            BOOST_CONTRACT_EXCEPT([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
        ; // Trailing `;` is required.

        ... // Function body.
    }
    @endcode
    
    This can be used to program contracts for non-member functions but also for
    private and protected functions, lambda functions, loops, arbitrary blocks
    of code, etc.
    For optimization, this can be omitted for code that does not have
    preconditions, postconditions, and exception guarantees.

    @c BOOST_CONTRACT_FUNCTION() expands to code equivalent to the following
    (note that no code is generated when @RefMacro{BOOST_CONTRACT_NO_FUNCTIONS}
    is defined):
    
    @code
        #ifndef BOOST_CONTRACT_NO_FUNCTIONS
            boost::contract::check internal_var =
                    boost::contract::function()
        #endif
    @endcode
    
    Where:
    
    @arg    <c><b>internal_var</b></c> is a variable name internally generated
            by this library (this name is unique but only on different line
            numbers so this macro cannot be expanded multiple times on the same
            line).
    
    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.non_member_functions, Non-Member Functions},
            @RefSect{advanced.private_and_protected_functions,
            Private and Protected Functions},
            @RefSect{advanced.lambdas__loops__code_blocks__and__constexpr__,
            Lambdas\, Loops\, Code Blocks}
    */
    #define BOOST_CONTRACT_FUNCTION()
#elif !defined(BOOST_CONTRACT_NO_FUNCTIONS)
    #include <boost/contract/function.hpp>
    #include <boost/contract/check.hpp>
    #include <boost/contract/detail/name.hpp>

    #define BOOST_CONTRACT_FUNCTION() \
        boost::contract::check BOOST_CONTRACT_DETAIL_NAME2(c, __LINE__) = \
                boost::contract::function()
#else
    #include <boost/preprocessor/facilities/empty.hpp>

    #define BOOST_CONTRACT_FUNCTION() /* nothing */
#endif

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program contracts that can be completely disabled at compile-time for static
    public functions.
    
    This is used together with @RefMacro{BOOST_CONTRACT_PRECONDITION},
    @RefMacro{BOOST_CONTRACT_POSTCONDITION}, @RefMacro{BOOST_CONTRACT_EXCEPT},
    and @RefMacro{BOOST_CONTRACT_OLD} to specify preconditions, postconditions,
    exception guarantees, and old value copies at body that can be completely
    disabled at compile-time for static public functions:

    @code
    class u {
        friend class boost::contract::access;

        BOOST_CONTRACT_STATIC_INVARIANT({ // Optional (as for non-static).
            BOOST_CONTRACT_ASSERT(...);
            ...
        })

    public:
        static void f(...) {
            BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
            BOOST_CONTRACT_PUBLIC_FUNCTION(u)
                BOOST_CONTRACT_PRECONDITION([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                BOOST_CONTRACT_OLD([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(old_expr);
                    ...
                })
                BOOST_CONTRACT_POSTCONDITION([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                BOOST_CONTRACT_EXCEPT([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ; // Trailing `;` is required.

            ... // Function body.
        }
        
        ...
    };
    @endcode

    For optimization, this can be omitted for static public functions that do
    not have preconditions, postconditions and exception guarantees, within
    classes that have no static invariants.
    
    @c BOOST_CONTRACT_STATIC_PUBLIC_FUNCTION(class_type) expands to code
    equivalent to the following (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS} is defined):
    
    @code
        #ifndef BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS
            boost::contract::check internal_var =
                    boost::contract::public_function<class_type>()
        #endif
    @endcode
    
    Where:
    
    @arg    <c><b>class_type</b></c> is the type of the class containing the
            static public function declaring the contract.
            (This is a variadic macro parameter so it can contain commas not
            protected by round parenthesis.)
    @arg    <c><b>internal_var</b></c> is a variable name internally generated
            by this library (this name is unique but only on different line
            numbers so this macro cannot be expanded multiple times on the same
            line).
    
    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.static_public_functions, Static Public Functions}
    */
    #define BOOST_CONTRACT_STATIC_PUBLIC_FUNCTION(...)

    /**
    Program contracts that can be completely disabled at compile-time for
    non-static public functions that do not override.
    
    This is used together with @RefMacro{BOOST_CONTRACT_PRECONDITION},
    @RefMacro{BOOST_CONTRACT_POSTCONDITION}, @RefMacro{BOOST_CONTRACT_EXCEPT},
    and @RefMacro{BOOST_CONTRACT_OLD} to specify preconditions, postconditions,
    exception guarantees, and old value copies at body that can be completely
    disabled at compile-time for non-static public functions (virtual or not,
    void or not) that do not override:

    @code
    class u {
        friend class boost::contract::access;

        BOOST_CONTRACT_INVARIANT({ // Optional (as for static and volatile).
            BOOST_CONTRACT_ASSERT(...);
            ...
        })

    public:
        // Non-virtual (same if void).
        t f(...) {
            t result;
            BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
            BOOST_CONTRACT_PUBLIC_FUNCTION(this)
                BOOST_CONTRACT_PRECONDITION([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                BOOST_CONTRACT_OLD([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(old_expr);
                    ...
                })
                BOOST_CONTRACT_POSTCONDITION([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                BOOST_CONTRACT_EXCEPT([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ; // Trailing `;` is required.

            ... // Function body (use `return result = return_expr`).
        }
        
        // Virtual and void.
        virtual void g(..., boost::contract::virtual_* v = 0) {
            BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
            BOOST_CONTRACT_PUBLIC_FUNCTION(v, this)
                ...
                BOOST_CONTRACT_OLD([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(v, old_expr);
                    ...
                })
                ...
            ; // Trailing `;` is required.
            
            ... // Function body.
        }
        
        // Virtual and non-void.
        virtual t h(..., boost::contract::virtual_* v = 0) {
            t result;
            BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
            BOOST_CONTRACT_PUBLIC_FUNCTION(v, result, this)
                ...
                BOOST_CONTRACT_OLD([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(v, old_expr);
                    ...
                })
                BOOST_CONTRACT_POSTCONDITION([&] (t const& result) { // Optional
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                ...
            ; // Trailing `;` is required.
            
            ... // Function body (use `return result = return_expr`).
        }
        
        ...
    };
    @endcode

    For optimization, this can be omitted for non-virtual public functions that
    do not have preconditions, postconditions and exception guarantees, within
    classes that have no invariants.
    Virtual public functions should always use
    @RefMacro{BOOST_CONTRACT_PUBLIC_FUNCTION} otherwise this library will not
    be able to correctly use them for subcontracting.
    
    This is an overloaded variadic macro and it can be used in the following
    different ways (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS} is defined).

    1\. <c>BOOST_CONTRACT_PUBLIC_FUNCTION(obj)</c> expands to code
        equivalent to the following (for non-virtual public functions that are
        not static and do not override, returning void or not):

    @code
        #ifndef BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS
            boost::contract::check internal_var =
                    boost::contract::public_function(obj)
        #endif
    @endcode
    
    2\. <c>BOOST_CONTRACT_PUBLIC_FUNCTION(v, obj)</c> expands to code
        equivalent to the following (for virtual public functions that do not
        override, returning void):

    @code
        #ifndef BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS
            boost::contract::check internal_var =
                    boost::contract::public_function(v, obj)
        #endif
    @endcode
    
    3\. <c>BOOST_CONTRACT_PUBLIC_FUNCTION(v, r, obj)</c> expands to code
        equivalent to the following (for virtual public functions that do not
        override, not returning void):

    @code
        #ifndef BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS
            boost::contract::check internal_var =
                    boost::contract::public_function(v, r, obj)
        #endif
    @endcode

    Where (these are all variadic macro parameters so they can contain commas
    not protected by round parenthesis):

    @arg    <c><b>v</b></c> is the extra parameter of type
            @RefClass{boost::contract::virtual_}<c>*</c> and default value @c 0
            from the enclosing virtual public function declaring the contract.
    @arg    <c><b>r</b></c> is a reference to the return value of the enclosing
            virtual public function declaring the contract.
            This is usually a local variable declared by the enclosing virtual
            public function just before the contract, but programmers must set
            it to the actual value being returned by the function at each
            @c return statement.
    @arg    <c><b>obj</b></c> is the object @c this from the scope of the
            enclosing public function declaring the contract.
            This object might be mutable, @c const, @c volatile, or
            <c>const volatile</c> depending on the cv-qualifier of the enclosing
            function (volatile public functions will check volatile class
            invariants, see @RefSect{extras.volatile_public_functions,
            Volatile Public Functions}).
    @arg    <c><b>internal_var</b></c> is a variable name internally generated
            by this library (this name is unique but only on different line
            numbers so this macro cannot be expanded multiple times on the same
            line).
    
    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.public_functions, Public Functions},
            @RefSect{tutorial.virtual_public_functions,
            Virtual Public Functions}
    */
    #define BOOST_CONTRACT_PUBLIC_FUNCTION(...)
    
    /**
    Program contracts that can be completely disabled at compile-time for
    public function overrides.
    
    This is used together with @RefMacro{BOOST_CONTRACT_PRECONDITION},
    @RefMacro{BOOST_CONTRACT_POSTCONDITION}, @RefMacro{BOOST_CONTRACT_EXCEPT},
    and @RefMacro{BOOST_CONTRACT_OLD} to specify preconditions, postconditions,
    exception guarantees, and old value copies at body that can be completely
    disabled at compile-time for public function overrides (virtual or not):

    @code
    class u
        #define BASES private boost::contract::constructor_precondition<u>, \
                public b, private w
        : BASES
    {
        friend class boost::contract::access;

        typedef BOOST_CONTRACT_BASE_TYPES(BASES) base_types;
        #undef BASES

        BOOST_CONTRACT_INVARIANT({ // Optional (as for static and volatile).
            BOOST_CONTRACT_ASSERT(...);
            ...
        })

        BOOST_CONTRACT_OVERRIDES(f, g)

    public:
        // Override from `b::f`, and void.
        void f(t_1 a_1, ..., t_n a_n, boost::contract::virtual_* v = 0) {
            BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
            BOOST_CONTRACT_PUBLIC_FUNCTION_OVERRIDE(override_f)(
                    v, &u::f, this, a_1, ..., a_n)
                BOOST_CONTRACT_PRECONDITION([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                BOOST_CONTRACT_OLD([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(v, old_expr);
                    ...
                })
                BOOST_CONTRACT_POSTCONDITION([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                BOOST_CONTRACT_EXCEPT([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ; // Trailing `;` is required.

            ... // Function body.
        }
        
        // Override from `b::g`, and void.
        t g(t_1 a_1, ..., t_n a_n, boost::contract::virtual_* v = 0) {
            t result;
            BOOST_CONTRACT_OLD_PTR(old_type)(old_var);
            BOOST_CONTRACT_PUBLIC_FUNCTION_OVERRIDE(override_g)(
                    v, result, &u::g, this, a_1, ..., a_n)
                ...
                BOOST_CONTRACT_OLD([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(v, old_expr);
                    ...
                })
                BOOST_CONTRACT_POSTCONDITION([&] (t const& result) { // Optional
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                ...
            ; // Trailing `;` is required.

            ... // Function body (use `return result = return_expr`).
        }
        
        ...
    };
    @endcode

    Public function overrides should always use
    @RefMacro{BOOST_CONTRACT_PUBLIC_FUNCTION_OVERRIDE} otherwise this library
    will not be able to correctly use it for subcontracting.
    
    This is an overloaded variadic macro and it can be used in the following
    different ways (note that no code is generated when
    @RefMacro{BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS} is defined).

    1\. <c>BOOST_CONTRACT_PUBLIC_FUNCTION_OVERRIDE(override_type)(v, f, obj,
        ...)</c> expands to code equivalent to the following (for public
        function overrides that return void):

    @code
        #ifndef BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS
            boost::contract::check internal_var = boost::contract::
                    public_function<override_type>(v, f, obj, ...)
        #endif
    @endcode
    
    2\. <c>BOOST_CONTRACT_PUBLIC_FUNCTION_OVERRIDE(override_type)(v, r, f, obj,
        ...)</c> expands to code equivalent to the following (for public
        function overrides that do not return void):

    @code
        #ifndef BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS
            boost::contract::check internal_var = boost::contract::
                    public_function<override_type>(v, r, f, obj, ...)
        #endif
    @endcode

    Where (these are all variadic macro parameters so they can contain commas
    not protected by round parenthesis):

    @arg    <c><b>override_type</b></c> is the type
            <c>override_<i>function-name</i></c> declared using the
            @RefMacro{BOOST_CONTRACT_OVERRIDE} or related macros.
    @arg    <c><b>v</b></c> is the extra parameter of type
            @RefClass{boost::contract::virtual_}<c>*</c> and default value @c 0
            from the enclosing virtual public function declaring the contract.
    @arg    <c><b>r</b></c> is a reference to the return value of the enclosing
            virtual public function declaring the contract.
            This is usually a local variable declared by the enclosing virtual
            public function just before the contract, but programmers must set
            it to the actual value being returned by the function at each
            @c return statement.
    @arg    <c><b>f</b></c> is a pointer to the enclosing public function
            override declaring the contract.
    @arg    <c><b>obj</b></c> is the object @c this from the scope of the
            enclosing public function declaring the contract.
            This object might be mutable, @c const, @c volatile, or
            <c>const volatile</c> depending on the cv-qualifier of the enclosing
            function (volatile public functions will check volatile class
            invariants, see @RefSect{extras.volatile_public_functions,
            Volatile Public Functions}).
    @arg    <c><b>...</b></c> is a variadic macro parameter listing all the
            arguments passed to the enclosing public function override declaring
            the contract (by reference and in the order they appear in the
            enclosing function declaration), but excluding the trailing
            argument @c v.
    @arg    <c><b>internal_var</b></c> is a variable name internally generated
            by this library (this name is unique but only on different line
            numbers so this macro cannot be expanded multiple times on the same
            line).
    
    @see    @RefSect{extras.disable_contract_compilation__macro_interface_,
            Disable Contract Compilation},
            @RefSect{tutorial.public_function_overrides__subcontracting_,
            Public Function Overrides}
    */
    #define BOOST_CONTRACT_PUBLIC_FUNCTION_OVERRIDE(...)
#elif !defined(BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS)
    #include <boost/contract/public_function.hpp>
    #include <boost/contract/check.hpp>
    #include <boost/contract/detail/name.hpp>
    
    #define BOOST_CONTRACT_STATIC_PUBLIC_FUNCTION(...) \
        boost::contract::check BOOST_CONTRACT_DETAIL_NAME2(c, __LINE__) = \
                boost::contract::public_function< __VA_ARGS__ >()

    #define BOOST_CONTRACT_PUBLIC_FUNCTION(...) \
        boost::contract::check BOOST_CONTRACT_DETAIL_NAME2(c, __LINE__) = \
                boost::contract::public_function(__VA_ARGS__)

    #define BOOST_CONTRACT_PUBLIC_FUNCTION_OVERRIDE(...) \
        boost::contract::check BOOST_CONTRACT_DETAIL_NAME2(c, __LINE__) = \
                boost::contract::public_function<__VA_ARGS__>
#else
    #include <boost/preprocessor/tuple/eat.hpp>
   
    #define BOOST_CONTRACT_STATIC_PUBLIC_FUNCTION(...) /* nothing */

    #define BOOST_CONTRACT_PUBLIC_FUNCTION(...) /* nothing */
    
    #define BOOST_CONTRACT_PUBLIC_FUNCTION_OVERRIDE(...) BOOST_PP_TUPLE_EAT(0)
#endif

#endif // #include guard

