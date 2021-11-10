/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_QVM_VEC_TRAITS_HPP_INCLUDED
#define BOOST_QVM_VEC_TRAITS_HPP_INCLUDED

namespace boost { namespace qvm {

template <class V>
struct
vec_traits
    {
    static int const dim=0;
    typedef void scalar_type;
    };

template <class T>
struct
is_vec
    {
    static bool const value=vec_traits<T>::dim>0;
    };

} }

#endif
