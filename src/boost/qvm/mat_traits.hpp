#ifndef BOOST_QVM_TRAITS_HPP_INCLUDED
#define BOOST_QVM_TRAITS_HPP_INCLUDED

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

namespace boost { namespace qvm {

template <class M>
struct
mat_traits
    {
    static int const rows=0;
    static int const cols=0;
    typedef void scalar_type;
    };

template <class T>
struct
is_mat
    {
    static bool const value=mat_traits<T>::rows>0 && mat_traits<T>::cols>0;
    };

} }

#endif
