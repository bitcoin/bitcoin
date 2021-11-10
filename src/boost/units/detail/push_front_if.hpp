// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_PUSH_FRONT_IF_HPP
#define BOOST_UNITS_DETAIL_PUSH_FRONT_IF_HPP

namespace boost {

namespace units {

template<class T, class Next>
struct list;

namespace detail {

template<bool>
struct push_front_if;

template<>
struct push_front_if<true> {
    template<class L, class T>
    struct apply {
        typedef list<T, L> type;
    };
};

template<>
struct push_front_if<false> {
    template<class L, class T>
    struct apply {
        typedef L type;
    };
};

}

}

}

#endif
