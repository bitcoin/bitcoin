
#ifndef BOOST_CONTRACT_DETAIL_TVARIADIC_HPP_
#define BOOST_CONTRACT_DETAIL_TVARIADIC_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    #define BOOST_CONTRACT_DETAIL_TVARIADIC 0
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC 1
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #include <tuple>

    /* CODE */

    namespace boost { namespace contract { namespace detail {
            namespace tvariadic_ {
        template<int...> struct indexes {};

        template<int N, int... I> struct indexes_of :
                indexes_of<N - 1, N - 1, I...> {};
        template<int... I> struct indexes_of<0, I...>
                { typedef indexes<I...> type; };
    } } } } // namespace

#else
    #include <boost/preprocessor/repetition/enum.hpp>
    #include <boost/preprocessor/repetition/repeat.hpp>
    #include <boost/preprocessor/tuple/elem.hpp>
    #include <boost/preprocessor/punctuation/comma_if.hpp>
    #include <boost/preprocessor/cat.hpp>

    /* PRIVATE */

    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_ELEM_(z, n, tuple) \
        BOOST_PP_CAT(tuple, n)
    
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INIT_(z, n, tuplevar_values) \
        BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 0, tuplevar_values), n)( \
                BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 1, tuplevar_values), n))

    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_(z, n, type_qualifier_name) \
        BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 0, type_qualifier_name), n) \
            BOOST_PP_TUPLE_ELEM(3, 1, type_qualifier_name) \
            BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 2, type_qualifier_name), n) \
        ;

    #define BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_(z, n, tokens) \
        tokens
     
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_ARG_(z, n, name) \
        BOOST_PP_CAT(name, n)
    
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_FPARAM_(z, n, type_qualifier_name) \
        BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 0, type_qualifier_name), n) \
        BOOST_PP_TUPLE_ELEM(3, 1, type_qualifier_name) \
        BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(3, 2, type_qualifier_name), n)
    
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TPARAM_(z, n, name) \
        typename BOOST_PP_CAT(name, n)
#endif

/* PUBLIC */

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_COMMA(arity) \
        ,
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_COMMA(arity) \
        BOOST_PP_COMMA_IF(arity)
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA(arity) /* nothing */
#else
    #define BOOST_CONTRACT_DETAIL_NO_TVARIADIC_COMMA(arity) \
        BOOST_PP_COMMA_IF(arity)
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_SIZEOF(arity, name) sizeof...(name)
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_SIZEOF(arity, name) arity
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TPARAMS_Z(z, arity, name) \
        typename... name
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TPARAMS_Z(z, arity, name) \
        BOOST_PP_ENUM_ ## z(arity, BOOST_CONTRACT_DETAIL_TVARIADIC_TPARAM_, \
                name)
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_FPARAMS_Z( \
            z, arity, type, qualifier, name) \
        type qualifier ... name
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_FPARAMS_Z( \
            z, arity, type, qualifier, name) \
        BOOST_PP_ENUM_ ## z(arity, BOOST_CONTRACT_DETAIL_TVARIADIC_FPARAM_, \
                (type, qualifier, name))
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_ARGS_Z(z, arity, name) \
        name...
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_ARGS_Z(z, arity, name) \
        BOOST_PP_ENUM_ ## z(arity, BOOST_CONTRACT_DETAIL_TVARIADIC_ARG_, name)
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(z, arity, tokens) \
        /* nothing */
#else
    #define BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_Z(z, arity, tokens) \
        BOOST_PP_ENUM_ ## z(arity, BOOST_CONTRACT_DETAIL_NO_TVARIADIC_ENUM_, \
                tokens)
#endif

// Tuple.

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_Z( \
            z, arity, type, qualifier, name) \
        std::tuple<type qualifier ...> name;
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_Z( \
            z, arity, type, qualifier, name) \
        BOOST_PP_REPEAT_ ## z(arity, BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_, \
                (type, qualifier, name))
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INIT_Z(z, \
            arity, tuple, values) \
        tuple(values...)
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INIT_Z(z, \
            arity, tuple, values) \
        BOOST_PP_ENUM_ ## z(arity, BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INIT_,\
                (tuple, values))
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INDEXES_TPARAM(indexes) \
        int... indexes
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INDEXES_TPARAM(indexes) \
        /* nothing */
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INDEXES_FPARAM(_indexes) \
        boost::contract::detail::tvariadic_::indexes<_indexes...>
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INDEXES_FPARAM(_indexes) \
        /* nothing */
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INDEXES_OF(tuple_type) \
        typename boost::contract::detail::tvariadic_::indexes_of< \
                sizeof...(tuple_type)>::type()
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_INDEXES_OF(unused) \
        /* nothing */
#endif

#if BOOST_CONTRACT_DETAIL_TVARIADIC
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_ELEMS_Z( \
            z, arity, indexes, tuple) \
        std::get<indexes>(tuple)...
#else
    #define BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_ELEMS_Z( \
            z, arity, indexes, tuple) \
        BOOST_PP_ENUM_ ## z(arity, BOOST_CONTRACT_DETAIL_TVARIADIC_TUPLE_ELEM_,\
                tuple)
#endif

#endif // #include guard

