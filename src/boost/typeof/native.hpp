// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_NATIVE_HPP_INCLUDED
#define BOOST_TYPEOF_NATIVE_HPP_INCLUDED

#ifndef MSVC_TYPEOF_HACK

#ifdef BOOST_NO_SFINAE

namespace boost { namespace type_of {

    template<class T> 
        T& ensure_obj(const T&);

}}

#else

#include <boost/type_traits/enable_if.hpp>
#include <boost/type_traits/is_function.hpp> 

namespace boost { namespace type_of {
# ifdef BOOST_NO_SFINAE
    template<class T> 
    T& ensure_obj(const T&);
# else
    template<typename T>
        typename enable_if_<is_function<T>::value, T&>::type
        ensure_obj(T&);

    template<typename T>
        typename enable_if_<!is_function<T>::value, T&>::type
        ensure_obj(const T&);
# endif
}}

#endif//BOOST_NO_SFINAE

#define BOOST_TYPEOF(expr) BOOST_TYPEOF_KEYWORD(boost::type_of::ensure_obj(expr))
#define BOOST_TYPEOF_TPL BOOST_TYPEOF

#define BOOST_TYPEOF_NESTED_TYPEDEF_TPL(name,expr) \
    struct name {\
        typedef BOOST_TYPEOF_TPL(expr) type;\
    };

#define BOOST_TYPEOF_NESTED_TYPEDEF(name,expr) \
    struct name {\
        typedef BOOST_TYPEOF(expr) type;\
    };

#endif//MSVC_TYPEOF_HACK

#define BOOST_TYPEOF_REGISTER_TYPE(x)
#define BOOST_TYPEOF_REGISTER_TEMPLATE(x, params)

#endif//BOOST_TYPEOF_NATIVE_HPP_INCLUDED

