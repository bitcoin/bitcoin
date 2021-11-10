
#ifndef BOOST_CONTRACT_BASE_TYPES_HPP_
#define BOOST_CONTRACT_BASE_TYPES_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

/** @file
Specify inheritance form base classes (for subcontracting).
*/

// IMPORTANT: Included by contract_macro.hpp so must #if-guard all its includes.
#include <boost/contract/core/config.hpp>
#include <boost/preprocessor/config/config.hpp>

#ifdef BOOST_CONTRACT_DETAIL_DOXYGEN

/**
Used to program the @c typedef that lists the bases of a derived class.

In order to support subcontracting, a derived class that specifies contracts for
one or more overriding public functions must declare a @c typedef named
@c base_types (or @RefMacro{BOOST_CONTRACT_BASES_TYPEDEF}) using this macro:

    @code
        class u
            #define BASES public b, protected virtual w1, private w2
            : BASES
        {
            friend class boost::contract:access;

            typedef BOOST_CONTRACT_BASE_TYPES(BASES) base_types;
            #undef BASES

            ...
        };
    @endcode

This @c typedef must be @c public unless @RefClass{boost::contract::access} is
used.

@see @RefSect{tutorial.base_classes__subcontracting_, Base Classes}

@param ...  Comma separated list of base classes.
            Each base must explicitly specify its access specifier @c public,
            @c protected, or @c private, and also @c virtual when present
            (this not always required in C++ instead).
            There is a limit of about 20 maximum bases that can be listed
            (because of similar limits in Boost.MPL internally used by this
            library).
            This is a variadic macro parameter, on compilers that do not support
            variadic macros, the @c typedef for base classes can be programmed
            manually without using this macro (see
            @RefSect{extras.no_macros__and_no_variadic_macros_, No Macros}).
            
*/
#define BOOST_CONTRACT_BASE_TYPES(...)

#elif !BOOST_PP_VARIADICS
    
#define BOOST_CONTRACT_BASE_TYPES \
BOOST_CONTRACT_ERROR_macro_BASE_TYPES_requires_variadic_macros_otherwise_manually_program_base_types

#elif !defined(BOOST_CONTRACT_NO_PUBLIC_FUNCTIONS)

#include <boost/mpl/vector.hpp>
#include <boost/contract/detail/preprocessor/keyword/virtual.hpp>
#include <boost/contract/detail/preprocessor/keyword/public.hpp>
#include <boost/contract/detail/preprocessor/keyword/protected.hpp>
#include <boost/contract/detail/preprocessor/keyword/private.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/push_back.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp> // For HEAD, TAIL, etc.
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/rem.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/expand.hpp>

/* PRIVATE */

#define BOOST_CONTRACT_BASE_TYPES_REMOVE_VIRTUAL_(base) \
    BOOST_PP_EXPAND( \
        BOOST_PP_IIF(BOOST_CONTRACT_DETAIL_PP_KEYWORD_IS_VIRTUAL(base), \
            BOOST_CONTRACT_DETAIL_PP_KEYWORD_REMOVE_VIRTUAL \
        , \
            BOOST_PP_TUPLE_REM(1) \
        )(base) \
    )

#define BOOST_CONTRACT_BASE_TYPES_PUSH_BACK_IF_(is_public, types_nilseq, base) \
    ( \
        is_public, \
        BOOST_PP_IIF(is_public, \
            BOOST_PP_SEQ_PUSH_BACK \
        , \
            types_nilseq BOOST_PP_TUPLE_EAT(2) \
        )(types_nilseq, base) \
    )

#define BOOST_CONTRACT_BASE_TYPES_SKIP_NOT_PUBLIC_(is_public, types_nilseq, \
        base) \
    (0, types_nilseq)

// Precondition: base = `public [virtual] ...`.
#define BOOST_CONTRACT_BASE_TYPES_PUSH_BACK_PUBLIC_(is_public, types_nilseq, \
        base) \
    ( \
        1, \
        BOOST_PP_SEQ_PUSH_BACK(types_nilseq, \
            BOOST_CONTRACT_BASE_TYPES_REMOVE_VIRTUAL_( \
                    BOOST_CONTRACT_DETAIL_PP_KEYWORD_REMOVE_PUBLIC(base)) \
        ) \
    )

#define BOOST_CONTRACT_BASE_TYPES_ACCESS_(is_public, types_nilseq, base) \
    BOOST_PP_IIF(BOOST_CONTRACT_DETAIL_PP_KEYWORD_IS_PUBLIC(base), \
        BOOST_CONTRACT_BASE_TYPES_PUSH_BACK_PUBLIC_ \
    , BOOST_PP_IIF(BOOST_CONTRACT_DETAIL_PP_KEYWORD_IS_PROTECTED(base), \
        BOOST_CONTRACT_BASE_TYPES_SKIP_NOT_PUBLIC_ \
    , BOOST_PP_IIF(BOOST_CONTRACT_DETAIL_PP_KEYWORD_IS_PRIVATE(base), \
        BOOST_CONTRACT_BASE_TYPES_SKIP_NOT_PUBLIC_ \
    , \
        BOOST_CONTRACT_BASE_TYPES_PUSH_BACK_IF_ \
    )))(is_public, types_nilseq, base)

#define BOOST_CONTRACT_BASE_TYPES_(s, public_types, base) \
    BOOST_CONTRACT_BASE_TYPES_ACCESS_( \
        BOOST_PP_TUPLE_ELEM(2, 0, public_types), \
        BOOST_PP_TUPLE_ELEM(2, 1, public_types), \
        BOOST_CONTRACT_BASE_TYPES_REMOVE_VIRTUAL_(base) \
    )

#define BOOST_CONTRACT_BASE_TYPES_RETURN_YES_(types_nilseq) \
    BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TAIL(types_nilseq))

#define BOOST_CONTRACT_BASE_TYPES_RETURN_(types_nilseq) \
    BOOST_PP_IIF(BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE(types_nilseq), 1), \
        BOOST_PP_TUPLE_EAT(1) \
    , \
        BOOST_CONTRACT_BASE_TYPES_RETURN_YES_ \
    )(types_nilseq)

#define BOOST_CONTRACT_BASE_TYPES_OK_(base_tuple, bases_seq) \
    boost::mpl::vector< \
        BOOST_CONTRACT_BASE_TYPES_RETURN_(BOOST_PP_TUPLE_ELEM(2, 1, \
            BOOST_PP_SEQ_FOLD_LEFT( \
                BOOST_CONTRACT_BASE_TYPES_, \
                (0, (BOOST_PP_NIL)), \
                bases_seq \
            ) \
        )) \
    >

#define BOOST_CONTRACT_BASE_TYPES_ERR_(bases_tuple, bases_seq) \
    BOOST_CONTRACT_ERROR_all_bases_must_explicitly_specify_public_protected_or_private base_tuple

#define BOOST_CONTRACT_BASE_TYPES_IS_ACCESS_(base) \
    BOOST_PP_IIF(BOOST_CONTRACT_DETAIL_PP_KEYWORD_IS_PUBLIC(base), \
        1 \
    , BOOST_PP_IIF(BOOST_CONTRACT_DETAIL_PP_KEYWORD_IS_PROTECTED(base), \
        1 \
    , BOOST_PP_IIF(BOOST_CONTRACT_DETAIL_PP_KEYWORD_IS_PRIVATE(base), \
        1 \
    , \
        0 \
    )))

// Cannot check that all base types have access specifiers (unless users have to
// specify bases using pp-seq, because user specified base list can have
// unwrapped commas between bases but also within a given base type, when base
// types are templates), but at least check the very first base type explicitly
// specifies access `[virtual] public | protected | private [virtual] ...`.
#define BOOST_CONTRACT_BASE_TYPES_CHECK_(bases_tuple, bases_seq) \
    BOOST_PP_IIF(BOOST_CONTRACT_BASE_TYPES_IS_ACCESS_( \
            BOOST_CONTRACT_BASE_TYPES_REMOVE_VIRTUAL_(BOOST_PP_SEQ_HEAD( \
                    bases_seq))), \
        BOOST_CONTRACT_BASE_TYPES_OK_ \
    , \
        BOOST_CONTRACT_BASE_TYPES_ERR_ \
    )(bases_tuple, bases_seq)

/* PUBLIC */

#define BOOST_CONTRACT_BASE_TYPES(...) \
    BOOST_CONTRACT_BASE_TYPES_CHECK_((__VA_ARGS__), \
            BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#else

#define BOOST_CONTRACT_BASE_TYPES(...) void /* dummy type for typedef */

#endif

#endif // #include guard

