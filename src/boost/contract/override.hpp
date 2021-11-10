
#ifndef BOOST_CONTRACT_OVERRIDE_HPP_
#define BOOST_CONTRACT_OVERRIDE_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Handle public function overrides (for subcontracting).
*/

// IMPORTANT: Included by contract_macro.hpp so must #if-guard all its includes.
#include <boost/contract/core/config.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/config/config.hpp>

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Declare an override type trait with an arbitrary name.

    Declare the override type trait named @c type_name to pass as an explicit
    template parameter to @RefFunc{boost::contract::public_function} for public
    function overrides.
    
    @see @RefSect{advanced.named_overrides, Named Overrides}

    @param type_name    Name of the override type trait this macro will declare.
                        (This is not a variadic macro parameter but it should
                        never contain commas because it is an identifier.)
    @param func_name    Function name of the public function override.
                        This macro is called just once even if the function name
                        is overloaded (the same override type trait is used for
                        all overloaded functions with the same name, see
                        @RefSect{advanced.function_overloads,
                        Function Overloads}).
                        (This is not a variadic macro parameter but it should
                        never contain commas because it is an identifier.)
    */
    #define BOOST_CONTRACT_NAMED_OVERRIDE(type_name, func_name)
#elif !defined(BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS)
    #include <boost/contract/core/virtual.hpp>
    #include <boost/contract/detail/type_traits/mirror.hpp>
    #include <boost/contract/detail/tvariadic.hpp>
    #include <boost/contract/detail/none.hpp>
    #include <boost/contract/detail/name.hpp>

    /* PRIVATE */

    #define BOOST_CONTRACT_OVERRIDE_CALL_BASE_(z, arity, arity_compl, \
            func_name) \
        template< \
            class BOOST_CONTRACT_DETAIL_NAME1(B), \
            class BOOST_CONTRACT_DETAIL_NAME1(C) \
            BOOST_CONTRACT_DETAIL_TVARIADIC_COMMA(arity) \
            BOOST_CONTRACT_DETAIL_TVARIADIC_TPARAMS_Z(z, arity, \
                    BOOST_CONTRACT_DETAIL_NAME1(Args)) \
        > \
        static void BOOST_CONTRACT_DETAIL_NAME1(call_base)( \
            boost::contract::virtual_* BOOST_CONTRACT_DETAIL_NAME1(v), \
            BOOST_CONTRACT_DETAIL_NAME1(C)* BOOST_CONTRACT_DETAIL_NAME1(obj) \
            BOOST_CONTRACT_DETAIL_TVARIADIC_COMMA(arity) \
            BOOST_CONTRACT_DETAIL_TVARIADIC_FPARAMS_Z(z, arity, \
                BOOST_CONTRACT_DETAIL_NAME1(Args), \
                &, \
                BOOST_CONTRACT_DETAIL_NAME1(args) \
            ) \
            BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA(arity_compl) \
            BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(z, arity_compl, \
                    boost::contract::detail::none&) \
        ) { \
            BOOST_CONTRACT_DETAIL_NAME1(obj)-> \
            BOOST_CONTRACT_DETAIL_NAME1(B)::func_name( \
                BOOST_CONTRACT_DETAIL_TVARIADIC_ARGS_Z(z, arity, \
                        BOOST_CONTRACT_DETAIL_NAME1(args)) \
                BOOST_CONTRACT_DETAIL_TVARIADIC_COMMA(arity) \
                BOOST_CONTRACT_DETAIL_NAME1(v) \
            ); \
        }

    #if BOOST_CONTRACT_DETAIL_TVARIADIC
        #define BOOST_CONTRACT_OVERRIDE_CALL_BASE_DECL_(func_name) \
            BOOST_CONTRACT_OVERRIDE_CALL_BASE_(1, ~, ~, func_name)
    #else
        #include <boost/preprocessor/repetition/repeat.hpp>
        #include <boost/preprocessor/arithmetic/inc.hpp>
        #include <boost/preprocessor/arithmetic/sub.hpp>

        #define BOOST_CONTRACT_OVERRIDE_CALL_BASE_DECL_(func_name) \
            BOOST_PP_REPEAT(BOOST_PP_INC(BOOST_CONTRACT_MAX_ARGS), \
                    BOOST_CONTRACT_OVERRIDE_CALL_BASE_ARITY_, func_name) \
        
        #define BOOST_CONTRACT_OVERRIDE_CALL_BASE_ARITY_(z, arity, func_name) \
            BOOST_CONTRACT_OVERRIDE_CALL_BASE_(z, arity, \
                    BOOST_PP_SUB(BOOST_CONTRACT_MAX_ARGS, arity), func_name)
    #endif

    /* PUBLIC */

    #define BOOST_CONTRACT_NAMED_OVERRIDE(type_name, func_name) \
        struct type_name { \
            BOOST_CONTRACT_DETAIL_MIRROR_HAS_MEMBER_FUNCTION( \
                BOOST_CONTRACT_DETAIL_NAME1(has_member_function), \
                func_name \
            ) \
            BOOST_CONTRACT_OVERRIDE_CALL_BASE_DECL_(func_name) \
        };
#else
    #define BOOST_CONTRACT_NAMED_OVERRIDE(type_name, func_name) \
            struct type_name {}; /* empty (not used), just to compile */
#endif
    
/* PUBLIC */

/**
Declare an override type trait named <c>override_<i>func_name</i></c>.

Declare the override type trait named <c>override_<i>func_name</i></c> to pass
as an explicit template parameter to @RefFunc{boost::contract::public_function}
for public function overrides.
Use @RefMacro{BOOST_CONTRACT_NAMED_OVERRIDE} to generate an override type trait
with a name different than <c>override_<i>func_name</i></c> (usually not
needed).

@see    @RefSect{tutorial.public_function_overrides__subcontracting_,
        Public Function Overrides}

@param func_name    Function name of the public function override.
                    This macro is called just once even if the function name is
                    overloaded (the same override type trait is used for all
                    overloaded functions with the same name, see
                    @RefSect{advanced.function_overloads, Function Overloads}).
                    (This is not a variadic macro parameter but it should never
                    contain any comma because it is an identifier.)
*/
#define BOOST_CONTRACT_OVERRIDE(func_name) \
    BOOST_CONTRACT_NAMED_OVERRIDE(BOOST_PP_CAT(override_, func_name), func_name)
    
#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN
    /**
    Declare multiple override type traits at once naming them
    <c>override_...</c> (for convenience).

    This variadic macro is provided for convenience as
    <c>BOOST_CONTRACT_OVERRIDES(f_1, f_2, ..., f_n)</c> expands to code
    equivalent to:

    @code
    BOOST_CONTRACT_OVERRIDE(f_1)
    BOOST_CONTRACT_OVERRIDE(f_2)
    ...
    BOOST_CONTRACT_OVERRIDE(f_n)
    @endcode

    On compilers that do not support variadic macros,
    the override type traits can be equivalently programmed one-by-one calling
    @RefMacro{BOOST_CONTRACT_OVERRIDE} for each function name as shown above.
    
    @see    @RefSect{tutorial.public_function_overrides__subcontracting_,
            Public Function Overrides}
    
    @param ...  A comma separated list of one or more function names of public
                function overrides.
                (Each function name should never contain commas because it is an
                identifier.)
    */
    #define BOOST_CONTRACT_OVERRIDES(...)
#elif BOOST_PP_VARIADICS
    #include <boost/preprocessor/seq/for_each.hpp>
    #include <boost/preprocessor/variadic/to_seq.hpp>
    
    /* PRIVATE */

    #define BOOST_CONTRACT_OVERRIDES_SEQ_(r, unused, func_name) \
        BOOST_CONTRACT_OVERRIDE(func_name)
    
    /* PUBLIC */

    #define BOOST_CONTRACT_OVERRIDES(...) \
        BOOST_PP_SEQ_FOR_EACH(BOOST_CONTRACT_OVERRIDES_SEQ_, ~, \
                BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))
#else
    #define BOOST_CONTRACT_OVERRIDES \
BOOST_CONTRACT_ERROR_macro_OVERRIDES_requires_variadic_macros_otherwise_manually_repeat_OVERRIDE_macro
#endif

#endif // #include guard

