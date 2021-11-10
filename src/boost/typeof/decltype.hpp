// Copyright (C) 2006 Arkadiy Vertleyb
// Copyright (C) 2017 Daniela Engert
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_DECLTYPE_HPP_INCLUDED
# define BOOST_TYPEOF_DECLTYPE_HPP_INCLUDED

#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace boost { namespace type_of {
    template<typename T>
        using remove_cv_ref_t = typename remove_cv<typename remove_reference<T>::type>::type;
}}

#define BOOST_TYPEOF(expr) boost::type_of::remove_cv_ref_t<decltype(expr)>
#define BOOST_TYPEOF_TPL BOOST_TYPEOF

#define BOOST_TYPEOF_NESTED_TYPEDEF_TPL(name,expr) \
    struct name {\
        typedef BOOST_TYPEOF_TPL(expr) type;\
    };

#define BOOST_TYPEOF_NESTED_TYPEDEF(name,expr) \
    struct name {\
        typedef BOOST_TYPEOF(expr) type;\
    };

#define BOOST_TYPEOF_REGISTER_TYPE(x)
#define BOOST_TYPEOF_REGISTER_TEMPLATE(x, params)

#endif //BOOST_TYPEOF_DECLTYPE_HPP_INCLUDED

