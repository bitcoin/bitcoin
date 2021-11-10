// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2007-2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_DETAIL_ORDINAL_HPP_INCLUDED
#define BOOST_UNITS_DETAIL_ORDINAL_HPP_INCLUDED

#include <boost/mpl/less.hpp>
#include <boost/mpl/bool.hpp>

namespace boost {
namespace units {

namespace detail {

struct ordinal_tag {};

}

template<int N>
struct ordinal {
    typedef detail::ordinal_tag tag;
    BOOST_STATIC_CONSTEXPR long value = N;
};

template<int N>
BOOST_CONSTEXPR_OR_CONST long ordinal<N>::value;

}

namespace mpl {

template<>
struct less_impl<units::detail::ordinal_tag, units::detail::ordinal_tag> {
    template<class T1, class T2>
    struct apply : bool_<(T1::value) < (T2::value)> {};
};

}

}

#endif
