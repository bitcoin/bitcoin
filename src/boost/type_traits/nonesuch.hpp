/*
Copyright 2017 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_NONESUCH_HPP_INCLUDED
#define BOOST_TT_NONESUCH_HPP_INCLUDED

#include <boost/config.hpp>

namespace boost {

#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)
struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    void operator=(const nonesuch&) = delete;
};
#else
class nonesuch {
    nonesuch();
    ~nonesuch();
    nonesuch(const nonesuch&);
    void operator=(const nonesuch&);
};
#endif

} /* boost */

#endif
