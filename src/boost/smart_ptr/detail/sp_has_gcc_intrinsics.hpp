#ifndef BOOST_SMART_PTR_DETAIL_SP_HAS_GCC_INTRINSICS_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_HAS_GCC_INTRINSICS_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif


// boost/smart_ptr/detail/sp_has_gcc_intrinsics.hpp
//
// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// Defines the BOOST_SP_HAS_GCC_INTRINSICS macro if the __atomic_*
// intrinsics are available.


#if defined( __ATOMIC_RELAXED ) && defined( __ATOMIC_ACQUIRE ) && defined( __ATOMIC_RELEASE ) && defined( __ATOMIC_ACQ_REL )

# define BOOST_SP_HAS_GCC_INTRINSICS

#endif

#endif // #ifndef BOOST_SMART_PTR_DETAIL_SP_HAS_GCC_INTRINSICS_HPP_INCLUDED
