// Copyright (C) 2010 Peder Holt
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_UNSUPPORTED_HPP_INCLUDED
#define BOOST_TYPEOF_UNSUPPORTED_HPP_INCLUDED

namespace boost { namespace type_of {
    struct typeof_emulation_is_unsupported_on_this_compiler {};
}}

#define BOOST_TYPEOF(expr) boost::type_of::typeof_emulation_is_unsupported_on_this_compiler
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

#endif