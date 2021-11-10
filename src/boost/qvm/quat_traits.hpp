#ifndef BOOST_QVM_QUAT_TRAITS
#define BOOST_QVM_QUAT_TRAITS

/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

namespace boost { namespace qvm {

template <class Q>
struct
quat_traits
    {
    typedef void scalar_type;
    };

namespace
is_quaternion_detail
    {
    template <class>
    struct
    is_void
        {
        static bool const value=false;
        };

    template <>
    struct
    is_void<void>
        {
        static bool const value=true;
        };
    }

template <class T>
struct
is_quat
    {
    static bool const value=!is_quaternion_detail::is_void<typename quat_traits<T>::scalar_type>::value;
    };

} }

#endif
