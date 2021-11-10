#if !defined(BOOST_PROTO_DONT_USE_PREPROCESSED_FILES)

    #include <boost/proto/detail/preprocessed/class_member_traits.hpp>

#elif !defined(BOOST_PP_IS_ITERATING)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 2, line: 0, output: "preprocessed/class_member_traits.hpp")
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    // class_member_traits.hpp
    // Contains specializations of the class_member_traits\<\> class template.
    //
    //  Copyright 2008 Eric Niebler. Distributed under the Boost
    //  Software License, Version 1.0. (See accompanying file
    //  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(preserve: 1)
    #endif

    #define BOOST_PP_ITERATION_PARAMS_1                                                             \
        (3, (0, BOOST_PROTO_MAX_ARITY, <boost/proto/detail/class_member_traits.hpp>))
    #include BOOST_PP_ITERATE()

    #if defined(__WAVE__) && defined(BOOST_PROTO_CREATE_PREPROCESSED_FILES)
        #pragma wave option(output: null)
    #endif

#else

    #define N BOOST_PP_ITERATION()

    template<typename T, typename U BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct class_member_traits<T (U::*)(BOOST_PP_ENUM_PARAMS(N, A))>
    {
        typedef U class_type;
        typedef T result_type;
    };

    template<typename T, typename U BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
    struct class_member_traits<T (U::*)(BOOST_PP_ENUM_PARAMS(N, A)) const>
    {
        typedef U class_type;
        typedef T result_type;
    };

    #undef N

#endif // BOOST_PROTO_DONT_USE_PREPROCESSED_FILES
