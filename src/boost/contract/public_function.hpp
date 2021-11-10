
#ifndef BOOST_CONTRACT_PUBLIC_FUNCTION_HPP_
#define BOOST_CONTRACT_PUBLIC_FUNCTION_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Program contracts for public functions (including subcontracting).
The different overloads handle public functions that are static, virtual void,
virtual non-void, overriding void, and overriding non-void.
*/

#include <boost/contract/core/config.hpp>
#include <boost/contract/core/specify.hpp>
#include <boost/contract/core/access.hpp>
#include <boost/contract/core/virtual.hpp>
/** @cond */
// Needed within macro expansions below instead of defined(...) (PRIVATE macro).
#if !defined(BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS) || \
        defined(BOOST_CONTRACT_STATIC_LINK)
    #define BOOST_CONTRACT_PUBLIC_FUNCTIONS_IMPL_ 1
#else
    #define BOOST_CONTRACT_PUBLIC_FUNCTIONS_IMPL_ 0
#endif
/** @endcond */
#include <boost/contract/detail/decl.hpp>
#include <boost/contract/detail/tvariadic.hpp>
#if BOOST_CONTRACT_PUBLIC_FUNCTIONS_IMPL_
    #include <boost/contract/detail/operation/static_public_function.hpp>
    #include <boost/contract/detail/operation/public_function.hpp>
    #include <boost/contract/detail/type_traits/optional.hpp>
    #include <boost/contract/detail/none.hpp>
    #include <boost/function_types/result_type.hpp>
    #include <boost/function_types/function_arity.hpp>
    #include <boost/optional.hpp>
    #include <boost/type_traits/remove_reference.hpp>
    #include <boost/type_traits/is_same.hpp>
    #include <boost/static_assert.hpp>
    #include <boost/preprocessor/tuple/eat.hpp>
#endif
#if !BOOST_CONTRACT_DETAIL_TVARIADIC
    #include <boost/preprocessor/repetition/repeat.hpp>
    #include <boost/preprocessor/arithmetic/sub.hpp>
    #include <boost/preprocessor/arithmetic/inc.hpp>
#endif
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>

namespace boost { namespace contract {

// NOTE: Override and (optionally) VirtualResult allowed only when v is present
// because:
// * An overriding func must override a base func declared virtual so with
//   v extra param, thus the overriding func must also always have v (i.e.,
//   Override might be present only if v is also present). However, the first
//   appearing virtual func (e.g., in root class) will not override any
//   previously declared virtual func so does not need Override (i.e., Override
//   always optional).
//   Furthermore, F needs to be specified only together with Override.
// * VirtualResult is only used for virtual functions (i.e., VirtualResult might
//   be present only if v is also present).
//   However, VirtualResult is never specified, not even for virtual functions,
//   when the return type is void (i.e., VirtualResult always optional).

/**
Program contracts for static public functions.

This is used to specify preconditions, postconditions, exception guarantees, old
value copies at body, and check static class invariants for static public
functions:

@code
class u {
    friend class boost::contract::access;

    static void static_invariant() { // Optional (as for non-static).
        BOOST_CONTRACT_ASSERT(...);
        ...
    }

public:
    static void f(...) {
        boost::contract::old_ptr<old_type> old_var;
        boost::contract::check c = boost::contract::public_function<u>()
            .precondition([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
            .old([&] { // Optional.
                old_var = BOOST_CONTRACT_OLDOF(old_expr);
                ...
            })
            .postcondition([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
            .except([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
        ;

        ... // Function body.
    }
    
    ...
};
@endcode

For optimization, this can be omitted for static public functions that do not
have preconditions, postconditions and exception guarantees, within classes that
have no static invariants.

@see @RefSect{tutorial.static_public_functions, Static Public Functions}

@tparam Class   The type of the class containing the static public function
                declaring the contract.
                This template parameter must be explicitly specified for static
                public functions (because they have no object @c this so there
                is no function argument from which this type template parameter
                can be automatically deduced by C++).

@return The result of this function must be assigned to a variable of type
        @RefClass{boost::contract::check} declared explicitly (i.e., without
        using C++11 @c auto declarations) and locally just before the code of
        the static public function body (otherwise this library will generate a
        run-time error, see @RefMacro{BOOST_CONTRACT_ON_MISSING_CHECK_DECL}).
*/
template<class Class>
specify_precondition_old_postcondition_except<> public_function() {
    #if BOOST_CONTRACT_PUBLIC_FUNCTIONS_IMPL_
        return specify_precondition_old_postcondition_except<>(
            new boost::contract::detail::static_public_function<Class>());
    #else
        return specify_precondition_old_postcondition_except<>();
    #endif
}

/**
Program contracts for public functions that are not static, not virtual, and do
not not override.

This is used to specify preconditions, postconditions, exception guarantees, old
value copies at body, and check class invariants for public functions that are
not static, not virtual, and do not override:

@code
class u {
    friend class boost::contract::access;

    void invariant() const { // Optional (as for static and volatile).
        BOOST_CONTRACT_ASSERT(...);
        ...
    }

public:
    void f(...) {
        boost::contract::old_ptr<old_type> old_var;
        boost::contract::check c = boost::contract::public_function(this)
            .precondition([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
            .old([&] { // Optional.
                old_var = BOOST_CONTRACT_OLDOF(old_expr);
                ...
            })
            .postcondition([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
            .except([&] { // Optional.
                BOOST_CONTRACT_ASSERT(...);
                ...
            })
        ;

        ... // Function body.
    }
    
    ...
};
@endcode

For optimization, this can be omitted for public functions that do not have
preconditions, postconditions and exception guarantees, within classes that have
no invariants.

@see @RefSect{tutorial.public_functions, Public Functions}

@param obj  The object @c this from the scope of the enclosing public function
            declaring the contract.
            This object might be mutable, @c const, @c volatile, or
            <c>const volatile</c> depending on the cv-qualifier of the enclosing
            function (volatile public functions will check volatile class
            invariants, see
            @RefSect{extras.volatile_public_functions,
            Volatile Public Functions}).

@tparam Class   The type of the class containing the public function declaring
                the contract.
                (Usually this template parameter is automatically deduced by C++
                and it does not need to be explicitly specified by programmers.)

@return The result of this function must be assigned to a variable of type
        @RefClass{boost::contract::check} declared explicitly (i.e., without
        using C++11 @c auto declarations) and locally just before the code of
        the public function body (otherwise this library will generate a
        run-time error, see @RefMacro{BOOST_CONTRACT_ON_MISSING_CHECK_DECL}).
*/
template<class Class>
specify_precondition_old_postcondition_except<> public_function(Class* obj) {
    #if BOOST_CONTRACT_PUBLIC_FUNCTIONS_IMPL_
        return specify_precondition_old_postcondition_except<>(
            new boost::contract::detail::public_function<
                boost::contract::detail::none,
                boost::contract::detail::none,
                boost::contract::detail::none,
                Class
                BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA(
                        BOOST_CONTRACT_MAX_ARGS)
                BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(1,
                    BOOST_CONTRACT_MAX_ARGS,
                    boost::contract::detail::none
                )
            >(
                static_cast<boost::contract::virtual_*>(0),
                obj,
                boost::contract::detail::none::value()
                BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA(
                        BOOST_CONTRACT_MAX_ARGS)
                BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(1,
                    BOOST_CONTRACT_MAX_ARGS,
                    boost::contract::detail::none::value()
                )
            )
        );
    #else
        return specify_precondition_old_postcondition_except<>();
    #endif
}

/** @cond */

// For non-static, virtual, and non-overriding public functions (PRIVATE macro).
#define BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_NO_OVERRIDE_( \
        has_virtual_result) \
    template< \
        BOOST_PP_EXPR_IIF(has_virtual_result, typename VirtualResult) \
        BOOST_PP_COMMA_IF(has_virtual_result) \
        class Class \
    > \
    specify_precondition_old_postcondition_except< \
            BOOST_PP_EXPR_IIF(has_virtual_result, VirtualResult)> \
    public_function( \
        virtual_* v \
        BOOST_PP_COMMA_IF(has_virtual_result) \
        BOOST_PP_EXPR_IIF(has_virtual_result, VirtualResult& r) \
        , Class* obj \
    ) { \
        BOOST_PP_IIF(BOOST_CONTRACT_PUBLIC_FUNCTIONS_IMPL_, \
            /* no F... so cannot enforce contracted F returns VirtualResult */ \
            return (specify_precondition_old_postcondition_except< \
                    BOOST_PP_EXPR_IIF(has_virtual_result, VirtualResult)>( \
                new boost::contract::detail::public_function< \
                    boost::contract::detail::none, \
                    BOOST_PP_IIF(has_virtual_result, \
                        VirtualResult \
                    , \
                        boost::contract::detail::none \
                    ), \
                    boost::contract::detail::none, \
                    Class \
                    BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA( \
                            BOOST_CONTRACT_MAX_ARGS) \
                    BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(1, \
                        BOOST_CONTRACT_MAX_ARGS, \
                        boost::contract::detail::none \
                    ) \
                >( \
                    v, \
                    obj, \
                    BOOST_PP_IIF(has_virtual_result, \
                        r \
                    , \
                        boost::contract::detail::none::value() \
                    ) \
                    BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA( \
                            BOOST_CONTRACT_MAX_ARGS) \
                    BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(1, \
                        BOOST_CONTRACT_MAX_ARGS, \
                        boost::contract::detail::none::value() \
                    ) \
                ) \
            )); \
        , \
            return specify_precondition_old_postcondition_except< \
                    BOOST_PP_EXPR_IIF(has_virtual_result, VirtualResult)>(); \
        ) \
    }

/** @endcond */
    
#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program contracts for void virtual public functions that do not override.

    This is used to specify preconditions, postconditions, exception guarantees,
    old value copies at body, and check class invariants for public functions
    that are virtual, do not override, and return @c void:

    @code
    class u {
        friend class boost::contract::access;

        void invariant() const { // Optional (as for static and volatile).
            BOOST_CONTRACT_ASSERT(...);
            ...
        }

    public:
        void f(..., boost::contract::virtual_* v = 0) {
            boost::contract::old_ptr<old_type> old_var;
            boost::contract::check c = boost::contract::public_function(v, this)
                .precondition([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                .old([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(v, old_expr);
                    ...
                })
                .postcondition([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                .except([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ;

            ... // Function body.
        }
        
        ...
    };
    @endcode

    A virtual public function should always call
    @RefFunc{boost::contract::public_function} otherwise this library will not
    be able to correctly use it for subcontracting.

    @see @RefSect{tutorial.virtual_public_functions, Virtual Public Functions}
    
    @param v    The trailing parameter of type
                @RefClass{boost::contract::virtual_}<c>*</c> and default value
                @c 0 from the enclosing virtual public function.
    @param obj  The object @c this from the scope of the enclosing virtual
                public function declaring the contract.
                This object might be mutable, @c const, @c volatile, or
                <c>const volatile</c> depending on the cv-qualifier of the
                enclosing function (volatile public functions will check
                volatile class invariants, see
                @RefSect{extras.volatile_public_functions,
                Volatile Public Functions}).

    @tparam Class   The type of the class containing the virtual public function
                    declaring the contract.
                    (Usually this template parameter is automatically deduced by
                    C++ and it does not need to be explicitly specified by
                    programmers.)
    
    @return The result of this function must be assigned to a variable of type
            @RefClass{boost::contract::check} declared explicitly (i.e., without
            using C++11 @c auto declarations) and locally just before the code
            of the public function body (otherwise this library will generate a
            run-time error, see
            @RefMacro{BOOST_CONTRACT_ON_MISSING_CHECK_DECL}).
    */
    template<class Class>
    specify_precondition_old_postcondition_except<> public_function(
            virtual_* v, Class* obj);
    
    /**
    Program contracts for non-void virtual public functions that do not
    override.

    This is used to specify preconditions, postconditions, exception guarantees,
    old value copies at body, and check class invariants for public functions
    that are virtual, do not override, and do not return @c void:
    
    @code
    class u {
        friend class boost::contract::access;

        void invariant() const { // Optional (as for static and volatile).
            BOOST_CONTRACT_ASSERT(...);
            ...
        }

    public:
        t f(..., boost::contract::virtual_* v = 0) {
            t result;
            boost::contract::old_ptr<old_type> old_var;
            boost::contract::check c = boost::contract::public_function(
                    v, result, this)
                .precondition([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                .old([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(v, old_expr);
                    ...
                })
                .postcondition([&] (t const& result) { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                .except([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ;

            ... // Function body (use `return result = return_expr`).
        }
        
        ...
    };
    @endcode

    A virtual public function should always call
    @RefFunc{boost::contract::public_function} otherwise this library will not
    be able to correctly use it for subcontracting.

    @see @RefSect{tutorial.virtual_public_functions, Virtual Public Functions}
    
    @param v    The trailing parameter of type
                @RefClass{boost::contract::virtual_}<c>*</c> and default value
                @c 0 from the enclosing virtual public function.
    @param r    A reference to the return value of the enclosing virtual public
                function declaring the contract.
                This is usually a local variable declared by the enclosing
                virtual public function just before the contract, but
                programmers must set it to the actual value being returned by
                the function at each @c return statement.
    @param obj  The object @c this from the scope of the enclosing virtual
                public function declaring the contract.
                This object might be mutable, @c const, @c volatile, or
                <c>const volatile</c> depending on the cv-qualifier of the
                enclosing function (volatile public functions will check
                volatile class invariants, see
                @RefSect{extras.volatile_public_functions,
                Volatile Public Functions}).
    
    @tparam VirtualResult   This type must be the same as, or compatible with,
                            the return type of the enclosing virtual public
                            function declaring the contract (this library might
                            not be able to generate a compile-time error if
                            these types mismatch, but in general that will cause
                            run-time errors or undefined behaviour).
                            Alternatively,
                            <c>boost::optional<<i>return-type</i>></c> can also
                            be used (see
                            @RefSect{advanced.optional_return_values,
                            Optional Return Values}).
                            (Usually this template parameter is automatically
                            deduced by C++ and it does not need to be explicitly
                            specified by programmers.)
    @tparam Class   The type of the class containing the virtual public function
                    declaring the contract.
                    (Usually this template parameter is automatically deduced by
                    C++ and it does not need to be explicitly specified by
                    programmers.)
    
    @return The result of this function must be assigned to a variable of type
            @RefClass{boost::contract::check} declared explicitly (i.e., without
            using C++11 @c auto declarations) and locally just before the code
            of the public function body (otherwise this library will generate a
            run-time error, see
            @RefMacro{BOOST_CONTRACT_ON_MISSING_CHECK_DECL}).
    */
    template<typename VirtualResult, class Class>
    specify_precondition_old_postcondition_except<VirtualResult>
    public_function(virtual_* v, VirtualResult& r, Class* obj);
#else
    BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_NO_OVERRIDE_(
            /* has_virtual_result = */ 0)
    BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_NO_OVERRIDE_(
            /* has_virtual_result = */ 1)
#endif

/** @cond */

// For non-static, virtual, and overriding public functions (PRIVATE macro).
#define BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDE_Z_( \
        z, arity, arity_compl, has_virtual_result) \
    BOOST_CONTRACT_DETAIL_DECL_OVERRIDING_PUBLIC_FUNCTION_Z(z, \
        arity, /* is_friend = */ 0, has_virtual_result, \
        Override, VirtualResult, F, Class, Args, \
        v, r, /* f */ BOOST_PP_EMPTY(), obj, args \
    ) { \
        BOOST_PP_IIF(BOOST_CONTRACT_PUBLIC_FUNCTIONS_IMPL_, \
            { /* extra scope paren to expand STATIC_STATIC emu on same line */ \
                /* assert not strictly necessary as compilation will fail */ \
                /* anyways, but helps limiting cryptic compiler's errors */ \
                BOOST_STATIC_ASSERT_MSG( \
                    /* -2 for both `this` and `virtual_*` extra parameters */ \
                    ( \
                        boost::function_types::function_arity<F>::value - 2 \
                    == \
                        BOOST_CONTRACT_DETAIL_TVARIADIC_SIZEOF(arity, Args) \
                    ), \
                    "missing one or more arguments for specified function" \
                ); \
            } \
            { /* extra scope paren to expand STATIC_STATIC emu on same line */ \
                /* assert consistency of F's result type and VirtualResult */ \
                BOOST_PP_IIF(has_virtual_result, \
                    BOOST_STATIC_ASSERT_MSG \
                , \
                    BOOST_PP_TUPLE_EAT(2) \
                )( \
                    (boost::is_same< \
                        typename boost::remove_reference<typename boost:: \
                                function_types::result_type<F>::type>::type, \
                        typename boost::contract::detail:: \
                                remove_value_reference_if_optional< \
                            VirtualResult \
                        >::type \
                    >::value), \
                    "mismatching result type for specified function" \
                ); \
            } \
            { /* extra scope paren to expand STATIC_STATIC emu on same line */ \
                /* assert this so lib can check and enforce override */ \
                BOOST_STATIC_ASSERT_MSG( \
                    boost::contract::access::has_base_types<Class>::value, \
                    "enclosing class missing 'base-types' typedef" \
                ); \
            } \
            return (specify_precondition_old_postcondition_except< \
                    BOOST_PP_EXPR_IIF(has_virtual_result, VirtualResult)>( \
                new boost::contract::detail::public_function< \
                    Override, \
                    BOOST_PP_IIF(has_virtual_result, \
                        VirtualResult \
                    , \
                        boost::contract::detail::none \
                    ), \
                    F, \
                    Class \
                    BOOST_CONTRACT_DETAIL_TVARIADIC_COMMA(arity) \
                    BOOST_CONTRACT_DETAIL_TVARIADIC_ARGS_Z(z, arity, Args) \
                    BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA(arity_compl) \
                    BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(z, arity_compl, \
                            boost::contract::detail::none) \
                >( \
                    v, \
                    obj, \
                    BOOST_PP_IIF(has_virtual_result, \
                        r \
                    , \
                        boost::contract::detail::none::value() \
                    ) \
                    BOOST_CONTRACT_DETAIL_TVARIADIC_COMMA(arity) \
                    BOOST_CONTRACT_DETAIL_TVARIADIC_ARGS_Z(z, arity, args) \
                    BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA(arity_compl) \
                    BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(z, arity_compl, \
                            boost::contract::detail::none::value()) \
                ) \
            )); \
        , \
            return specify_precondition_old_postcondition_except< \
                    BOOST_PP_EXPR_IIF(has_virtual_result, VirtualResult)>(); \
        ) \
    }

/** @endcond */

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Program contracts for void public functions overrides (virtual or not).

    This is used to specify preconditions, postconditions, exception guarantees,
    old value copies at body, and check class invariants for public function
    overrides (virtual or not) that return @c void:
    
    @code
    class u
        #define BASES private boost::contract::constructor_precondition<u>, \
                public b, private w
        : BASES
    {
        friend class boost::contract::access;

        typedef BOOST_CONTRACT_BASE_TYPES(BASES) base_types;
        #undef BASES

        void invariant() const { // Optional (as for static and volatile).
            BOOST_CONTRACT_ASSERT(...);
            ...
        }

        BOOST_CONTRACT_OVERRIDES(f)

    public:
        // Override from `b::f`.
        void f(t_1 a_1, ..., t_n a_n, boost::contract::virtual_* v = 0) {
            boost::contract::old_ptr<old_type> old_var;
            boost::contract::check c = boost::contract::public_function<
                    override_f>(v, &u::f, this, a_1, ..., a_n)
                .precondition([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                .old([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(v, old_expr);
                    ...
                })
                .postcondition([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                .except([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ;

            ... // Function body.
        }
        
        ...
    };
    @endcode

    A public function override should always call
    @RefFunc{boost::contract::public_function} otherwise this library will not
    be able to correctly use it for subcontracting.

    @see    @RefSect{tutorial.public_function_overrides__subcontracting_,
            Public Function Overrides}
    
    @param v    The trailing parameter of type
                @RefClass{boost::contract::virtual_}<c>*</c> and default value
                @c 0 from the enclosing public function override.
    @param f    A pointer to the enclosing public function override declaring
                the contract (but see @RefSect{advanced.function_overloads,
                Function Overloads}).
    @param obj  The object @c this from the scope of the enclosing public
                function override declaring the contract.
                This object might be mutable, @c const, @c volatile, or
                <c>const volatile</c> depending on the cv-qualifier of the
                enclosing function (volatile public functions will check
                volatile class invariants, see
                @RefSect{extras.volatile_public_functions,
                Volatile Public Functions}).
    @param args All arguments passed to the enclosing public function override
                declaring the contract (by reference and in the order they
                appear in the enclosing function declaration), but excluding the
                trailing argument @c v.

    @tparam Override    The type trait <c>override_<i>function-name</i></c>
                        declared using the @RefMacro{BOOST_CONTRACT_OVERRIDE} or
                        related macros.
                        This template parameter must be explicitly specified
                        (because there is no function argument from which it can
                        be automatically deduced by C++).
    @tparam F   The function pointer type of the enclosing public function
                override declaring the contract.
                (Usually this template parameter is automatically deduced by
                C++ and it does not need to be explicitly specified by
                programmers.)
    @tparam Class   The type of the class containing the virtual public function
                    declaring the contract.
                    (Usually this template parameter is automatically deduced by
                    C++ and it does not need to be explicitly specified by
                    programmers.)
    @tparam Args    The types of all parameters passed to the enclosing public
                    function override declaring the contract, but excluding the
                    trailing parameter type <c>boost::contract::virtual_*</c>.
                    On compilers that do not support variadic templates, this
                    library internally implements this function using
                    preprocessor meta-programming (in this case, the maximum
                    number of supported arguments is defined by
                    @RefMacro{BOOST_CONTRACT_MAX_ARGS}).
                    (Usually these template parameters are automatically deduced
                    by C++ and they do not need to be explicitly specified by
                    programmers.)

    @return The result of this function must be assigned to a variable of type
            @RefClass{boost::contract::check} declared explicitly (i.e., without
            using C++11 @c auto declarations) and locally just before the code
            of the public function body (otherwise this library will generate a
            run-time error, see
            @RefMacro{BOOST_CONTRACT_ON_MISSING_CHECK_DECL}).
    */
    template<class Override, typename F, class Class, typename... Args>
    specify_precondition_old_postcondition_except<> public_function(
            virtual_* v, F f, Class* obj, Args&... args);

    /**
    Program contracts for non-void public functions overrides (virtual or not).

    This is used to specify preconditions, postconditions, exception guarantees,
    old value copies at body, and check class invariants for public function
    overrides (virtual or not) that do not return @c void:
    
    @code
    class u
        #define BASES private boost::contract::constructor_precondition<u>, \
                public b, private w
        : BASES
    {
        friend class boost::contract::access;

        typedef BOOST_CONTRACT_BASE_TYPES(BASES) base_types;
        #undef BASES

        void invariant() const { // Optional (as for static and volatile).
            BOOST_CONTRACT_ASSERT(...);
            ...
        }

        BOOST_CONTRACT_OVERRIDES(f)

    public:
        // Override from `b::f`.
        t f(t_1 a_1, ..., t_n a_n, boost::contract::virtual_* v = 0) {
            t result;
            boost::contract::old_ptr<old_type> old_var;
            boost::contract::check c = boost::contract::public_function<
                    override_f>(v, result, &u::f, this, a_1, ..., a_n)
                .precondition([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                .old([&] { // Optional.
                    old_var = BOOST_CONTRACT_OLDOF(v, old_expr);
                    ...
                })
                .postcondition([&] (t const& result) { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
                .except([&] { // Optional.
                    BOOST_CONTRACT_ASSERT(...);
                    ...
                })
            ;

            ... // Function body (use `return result = return_expr`).
        }
        
        ...
    };
    @endcode

    A public function override should always call
    @RefFunc{boost::contract::public_function} otherwise this library will not
    be able to correctly use it for subcontracting.

    @see    @RefSect{tutorial.public_function_overrides__subcontracting_,
            Public Function Overrides}
    
    @param v    The trailing parameter of type
                @RefClass{boost::contract::virtual_}<c>*</c> and default value
                @c 0 from the enclosing public function override.
    @param r    A reference to the return value of the enclosing public function
                override declaring the contract.
                This is usually a local variable declared by the enclosing
                public function override just before the contract, but
                programmers must set it to the actual value being returned by
                the function at each @c return statement.
    @param f    A pointer to the enclosing public function override declaring
                the contract (but see @RefSect{advanced.function_overloads,
                Function Overloads}).
    @param obj  The object @c this from the scope of the enclosing public
                function override declaring the contract.
                This object might be mutable, @c const, @c volatile, or
                <c>const volatile</c> depending on the cv-qualifier of the
                enclosing function (volatile public functions will check
                volatile class invariants, see
                @RefSect{extras.volatile_public_functions,
                Volatile Public Functions}).
    @param args All arguments passed to the enclosing public function override
                declaring the contract (by reference and in the order they
                appear in the enclosing function declaration), but excluding the
                trailing argument @c v.

    @tparam Override    The type trait <c>override_<i>function-name</i></c>
                        declared using the @RefMacro{BOOST_CONTRACT_OVERRIDE} or
                        related macros.
                        This template parameter must be explicitly specified
                        (because there is no function argument from which it can
                        be automatically deduced by C++).
    @tparam VirtualResult   This type must be the same as, or compatible with,
                            the return type of the enclosing public function
                            override declaring the contract (this library might
                            not be able to generate a compile-time error if
                            these types mismatch, but in general that will cause
                            run-time errors or undefined behaviour).
                            Alternatively,
                            <c>boost::optional<<i>return-type</i>></c> can also
                            be used (see
                            @RefSect{advanced.optional_return_values,
                            Optional Return Values}).
                            (Usually this template parameter is automatically
                            deduced by C++ and it does not need to be explicitly
                            specified by programmers.)
    @tparam F   The function pointer type of the enclosing public function
                override declaring the contract.
                (Usually this template parameter is automatically deduced by
                C++ and it does not need to be explicitly specified by
                programmers.)
    @tparam Class   The type of the class containing the virtual public function
                    declaring the contract.
                    (Usually this template parameter is automatically deduced by
                    C++ and it does not need to be explicitly specified by
                    programmers.)
    @tparam Args    The types of all parameters passed to the enclosing public
                    function override declaring the contract, but excluding the
                    trailing parameter type <c>boost::contract::virtual_*</c>.
                    On compilers that do not support variadic templates, this
                    library internally implements this function using
                    preprocessor meta-programming (in this case, the maximum
                    number of supported arguments is defined by
                    @RefMacro{BOOST_CONTRACT_MAX_ARGS}).
                    (Usually these template parameters are automatically deduced
                    by C++ and they do not need to be explicitly specified by
                    programmers.)

    @return The result of this function must be assigned to a variable of type
            @RefClass{boost::contract::check} declared explicitly (i.e., without
            using C++11 @c auto declarations) and locally just before the code
            of the public function body (otherwise this library will generate a
            run-time error, see
            @RefMacro{BOOST_CONTRACT_ON_MISSING_CHECK_DECL}).
    */
    template<class Override, typename VirtualResult, typename F, class Class,
            typename... Args>
    specify_precondition_old_postcondition_except<VirtualResult>
    public_function(virtual_* v, VirtualResult& r, F f, Class* obj,
            Args&... args);

#elif BOOST_CONTRACT_DETAIL_TVARIADIC
    BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDE_Z_(1, /* arity = */ ~,
            /* arity_compl = */ ~, /* has_virtual_result = */ 0)
    BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDE_Z_(1, /* arity = */ ~,
            /* arity_compl = */ ~, /* has_virtual_result = */ 1)

#else
    /* PRIVATE */

    #define BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDE_ARITY_( \
            z, arity, unused) \
        BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDES_(z, arity, \
                BOOST_PP_SUB(BOOST_CONTRACT_MAX_ARGS, arity), ~)
    
    #define BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDES_(z, \
            arity, arity_compl, unused) \
        BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDE_Z_(z, \
                arity, arity_compl, /* has_virtual_result = */ 0) \
        BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDE_Z_(z, \
                arity, arity_compl, /* has_virtual_result = */ 1)

    /* CODE */

    BOOST_PP_REPEAT(BOOST_PP_INC(BOOST_CONTRACT_MAX_ARGS),
            BOOST_CONTRACT_PUBLIC_FUNCTION_VIRTUAL_OVERRIDE_ARITY_, ~)
#endif

} } // namespace

#endif // #include guard

