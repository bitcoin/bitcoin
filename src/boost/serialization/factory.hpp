#ifndef BOOST_SERIALIZATION_FACTORY_HPP
#define BOOST_SERIALIZATION_FACTORY_HPP

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

// factory.hpp: create an instance from an extended_type_info instance.

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstdarg> // valist
#include <cstddef> // NULL

#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/assert.hpp>

namespace std{
    #if defined(__LIBCOMO__)
        using ::va_list;
    #endif
} // namespace std

namespace boost {
namespace serialization {

// default implementation does nothing.
template<class T, int N>
T * factory(std::va_list){
    BOOST_ASSERT(false);
    // throw exception here?
    return NULL;
}

} // namespace serialization
} // namespace boost

#define BOOST_SERIALIZATION_FACTORY(N, T, A0, A1, A2, A3) \
namespace boost {                                         \
namespace serialization {                                 \
    template<>                                            \
    T * factory<T, N>(std::va_list ap){                   \
        BOOST_PP_IF(BOOST_PP_GREATER(N, 0)                \
            , A0 a0 = va_arg(ap, A0);, BOOST_PP_EMPTY())  \
        BOOST_PP_IF(BOOST_PP_GREATER(N, 1)                \
            , A1 a1 = va_arg(ap, A1);, BOOST_PP_EMPTY())  \
        BOOST_PP_IF(BOOST_PP_GREATER(N, 2)                \
            , A2 a2 = va_arg(ap, A2);, BOOST_PP_EMPTY())  \
        BOOST_PP_IF(BOOST_PP_GREATER(N, 3)                \
            , A3 a3 = va_arg(ap, A3);, BOOST_PP_EMPTY())  \
        return new T(                                     \
            BOOST_PP_IF(BOOST_PP_GREATER(N, 0)            \
                , a0, BOOST_PP_EMPTY())                   \
            BOOST_PP_IF(BOOST_PP_GREATER(N, 1))           \
                , BOOST_PP_COMMA, BOOST_PP_EMPTY)()       \
            BOOST_PP_IF(BOOST_PP_GREATER(N, 1)            \
                , a1, BOOST_PP_EMPTY())                   \
            BOOST_PP_IF(BOOST_PP_GREATER(N, 2))           \
                , BOOST_PP_COMMA, BOOST_PP_EMPTY)()       \
            BOOST_PP_IF(BOOST_PP_GREATER(N, 2)            \
                , a2, BOOST_PP_EMPTY())                   \
            BOOST_PP_IF(BOOST_PP_GREATER(N, 3))           \
                , BOOST_PP_COMMA, BOOST_PP_EMPTY)()       \
            BOOST_PP_IF(BOOST_PP_GREATER(N, 3)            \
                , a3, BOOST_PP_EMPTY())                   \
        );                                                \
    }                                                     \
}                                                         \
}   /**/

#define BOOST_SERIALIZATION_FACTORY_4(T, A0, A1, A2, A3) \
    BOOST_SERIALIZATION_FACTORY(4, T, A0, A1, A2, A3)

#define BOOST_SERIALIZATION_FACTORY_3(T, A0, A1, A2)     \
    BOOST_SERIALIZATION_FACTORY(3, T, A0, A1, A2, 0)

#define BOOST_SERIALIZATION_FACTORY_2(T, A0, A1)         \
    BOOST_SERIALIZATION_FACTORY(2, T, A0, A1, 0, 0)

#define BOOST_SERIALIZATION_FACTORY_1(T, A0)             \
    BOOST_SERIALIZATION_FACTORY(1, T, A0, 0, 0, 0)

#define BOOST_SERIALIZATION_FACTORY_0(T)                 \
namespace boost {                                        \
namespace serialization {                                \
    template<>                                           \
    T * factory<T, 0>(std::va_list){                     \
        return new T();                                  \
    }                                                    \
}                                                        \
}                                                        \
/**/

#endif // BOOST_SERIALIZATION_FACTORY_HPP
