/*
Copyright 2003 The Trustees of Indiana University

Authors: Jaakko Jarvi (jajarvi at osl.iu.edu)
         Jeremiah Willcock (jewillco at osl.iu.edu)
         Andrew Lumsdaine (lums at osl.iu.edu)

Copyright 2018 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_TT_ENABLE_IF_HPP_INCLUDED
#define BOOST_TT_ENABLE_IF_HPP_INCLUDED

#include <boost/config.hpp>

namespace boost {

template<bool B, class T = void>
struct enable_if_ {
    typedef T type;
};

template<class T>
struct enable_if_<false, T> { };

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
template<bool B, class T = void>
using enable_if_t = typename enable_if_<B, T>::type;
#endif

} /* boost */

#endif
