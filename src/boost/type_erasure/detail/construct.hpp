// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#if !defined(BOOST_PP_IS_ITERATING)

#ifndef BOOST_TYPE_ERASURE_DETAIL_CONSTRUCT_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_DETAIL_CONSTRUCT_HPP_INCLUDED

#define BOOST_PP_FILENAME_1 <boost/type_erasure/detail/construct.hpp>
#define BOOST_PP_ITERATION_LIMITS (1, BOOST_TYPE_ERASURE_MAX_ARITY)
#include BOOST_PP_ITERATE()

#endif

#else

#define N BOOST_PP_ITERATION()

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

#define BOOST_TYPE_ERASURE_FORWARD_I(z, n, data) ::std::forward<BOOST_PP_CAT(U, n)>(BOOST_PP_CAT(u, n))
#define BOOST_TYPE_ERASURE_FORWARD(n) BOOST_PP_ENUM(n, BOOST_TYPE_ERASURE_FORWARD_I, ~)

#if N > 1

    template<
        class R
        BOOST_PP_ENUM_TRAILING_PARAMS(N, class A)
        BOOST_PP_ENUM_TRAILING_PARAMS(N, class U)
    >
    const table_type& _boost_type_erasure_extract_table(
        ::boost::type_erasure::constructible<R(BOOST_PP_ENUM_PARAMS(N, A))>*
        BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, U, &u))
    {
        return *::boost::type_erasure::detail::BOOST_PP_CAT(extract_table, N)(
            (R(*)(BOOST_PP_ENUM_PARAMS(N, A)))0,
            BOOST_PP_ENUM_PARAMS(N, u));
    }

    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(BOOST_PP_ENUM_BINARY_PARAMS(N, U, &&u))
      : table(
            _boost_type_erasure_extract_table(
                false? this->_boost_type_erasure_deduce_constructor(BOOST_TYPE_ERASURE_FORWARD(N)) : 0
                BOOST_PP_ENUM_TRAILING_PARAMS(N, u)
            )
        ),
        data(::boost::type_erasure::call(
            ::boost::type_erasure::detail::make(
                false? this->_boost_type_erasure_deduce_constructor(BOOST_TYPE_ERASURE_FORWARD(N)) : 0
            ), BOOST_TYPE_ERASURE_FORWARD(N))
        )
    {}

#endif

    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(const binding<Concept>& binding_arg BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, U, &&u))
      : table(binding_arg),
        data(
            ::boost::type_erasure::call(
                binding_arg,
                ::boost::type_erasure::detail::make(
                    false? this->_boost_type_erasure_deduce_constructor(BOOST_TYPE_ERASURE_FORWARD(N)) : 0
                )
                BOOST_PP_COMMA_IF(N)
                BOOST_TYPE_ERASURE_FORWARD(N)
            )
        )
    {}

    // disambiguate
    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(binding<Concept>& binding_arg BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, U, &&u))
      : table(binding_arg),
        data(
            ::boost::type_erasure::call(
                binding_arg,
                ::boost::type_erasure::detail::make(
                    false? this->_boost_type_erasure_deduce_constructor(BOOST_TYPE_ERASURE_FORWARD(N)) : 0
                )
                BOOST_PP_COMMA_IF(N)
                BOOST_TYPE_ERASURE_FORWARD(N)
            )
        )
    {}

    // disambiguate
    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(binding<Concept>&& binding_arg BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, U, &&u))
      : table(binding_arg),
        data(
            ::boost::type_erasure::call(
                binding_arg,
                ::boost::type_erasure::detail::make(
                    false? this->_boost_type_erasure_deduce_constructor(BOOST_TYPE_ERASURE_FORWARD(N)) : 0
                )
                BOOST_PP_COMMA_IF(N)
                BOOST_TYPE_ERASURE_FORWARD(N)
            )
        )
    {}

#undef BOOST_TYPE_ERASURE_FORWARD
#undef BOOST_TYPE_ERASURE_FORWARD_I

#else

#if N > 1

    template<
        class R
        BOOST_PP_ENUM_TRAILING_PARAMS(N, class A)
        BOOST_PP_ENUM_TRAILING_PARAMS(N, class U)
    >
    const table_type& _boost_type_erasure_extract_table(
        ::boost::type_erasure::constructible<R(BOOST_PP_ENUM_PARAMS(N, A))>*
        BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, U, &u))
    {
        return *::boost::type_erasure::detail::BOOST_PP_CAT(extract_table, N)(
            (R(*)(BOOST_PP_ENUM_PARAMS(N, A)))0,
            BOOST_PP_ENUM_PARAMS(N, u));
    }

    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(BOOST_PP_ENUM_BINARY_PARAMS(N, const U, &u))
      : table(
            _boost_type_erasure_extract_table(
                false? this->_boost_type_erasure_deduce_constructor(BOOST_PP_ENUM_PARAMS(N, u)) : 0
                BOOST_PP_ENUM_TRAILING_PARAMS(N, u)
            )
        ),
        data(::boost::type_erasure::call(
            ::boost::type_erasure::detail::make(
                false? this->_boost_type_erasure_deduce_constructor(BOOST_PP_ENUM_PARAMS(N, u)) : 0
            ), BOOST_PP_ENUM_PARAMS(N, u))
        )
    {}

    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(BOOST_PP_ENUM_BINARY_PARAMS(N, U, &u))
      : table(
            _boost_type_erasure_extract_table(
                false? this->_boost_type_erasure_deduce_constructor(BOOST_PP_ENUM_PARAMS(N, u)) : 0
                BOOST_PP_ENUM_TRAILING_PARAMS(N, u)
            )
        ),
        data(::boost::type_erasure::call(
            ::boost::type_erasure::detail::make(
                false? this->_boost_type_erasure_deduce_constructor(BOOST_PP_ENUM_PARAMS(N, u)) : 0
            ), BOOST_PP_ENUM_PARAMS(N, u))
        )
    {}

#endif

    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(const binding<Concept>& binding_arg BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, const U, &u))
      : table(binding_arg),
        data(
            ::boost::type_erasure::call(
                binding_arg,
                ::boost::type_erasure::detail::make(
                    false? this->_boost_type_erasure_deduce_constructor(BOOST_PP_ENUM_PARAMS(N, u)) : 0
                )
                BOOST_PP_ENUM_TRAILING_PARAMS(N, u)
            )
        )
    {}

    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(const binding<Concept>& binding_arg BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, U, &u))
      : table(binding_arg),
        data(
            ::boost::type_erasure::call(
                binding_arg,
                ::boost::type_erasure::detail::make(
                    false? this->_boost_type_erasure_deduce_constructor(BOOST_PP_ENUM_PARAMS(N, u)) : 0
                )
                BOOST_PP_ENUM_TRAILING_PARAMS(N, u)
            )
        )
    {}

    // disambiguate
    template<BOOST_PP_ENUM_PARAMS(N, class U)>
    any(binding<Concept>& binding_arg BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, U, &u))
      : table(binding_arg),
        data(
            ::boost::type_erasure::call(
                binding_arg,
                ::boost::type_erasure::detail::make(
                    false? this->_boost_type_erasure_deduce_constructor(BOOST_PP_ENUM_PARAMS(N, u)) : 0
                )
                BOOST_PP_ENUM_TRAILING_PARAMS(N, u)
            )
        )
    {}

#endif

#undef N

#endif
