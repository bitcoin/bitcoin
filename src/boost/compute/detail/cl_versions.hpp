//---------------------------------------------------------------------------//
// Copyright (c) 2018 Jakub Szuppe <j.szuppe@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_CL_VERSION_HPP
#define BOOST_COMPUTE_DETAIL_CL_VERSION_HPP

#if defined(BOOST_COMPUTE_MAX_CL_VERSION)
#  if !defined(CL_USE_DEPRECATED_OPENCL_2_1_APIS) && BOOST_COMPUTE_MAX_CL_VERSION < 202
#    define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#  endif
#  if !defined(CL_USE_DEPRECATED_OPENCL_2_0_APIS) && BOOST_COMPUTE_MAX_CL_VERSION < 201
#    define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#  endif
#  if !defined(CL_USE_DEPRECATED_OPENCL_1_2_APIS) && BOOST_COMPUTE_MAX_CL_VERSION < 200
#    define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#  endif
#  if !defined(CL_USE_DEPRECATED_OPENCL_1_1_APIS) && BOOST_COMPUTE_MAX_CL_VERSION < 102
#    define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#  endif
#  if !defined(CL_USE_DEPRECATED_OPENCL_1_0_APIS) && BOOST_COMPUTE_MAX_CL_VERSION < 101
#    define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#  endif
#endif

#if defined(BOOST_COMPUTE_MAX_CL_VERSION) && !defined(CL_TARGET_OPENCL_VERSION)
#  if BOOST_COMPUTE_MAX_CL_VERSION == 201
#    define CL_TARGET_OPENCL_VERSION 210
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 200
#    define CL_TARGET_OPENCL_VERSION 200
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 102
#    define CL_TARGET_OPENCL_VERSION 120
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 101
#    define CL_TARGET_OPENCL_VERSION 110
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 100
#    define CL_TARGET_OPENCL_VERSION 100
#  else 
#    define CL_TARGET_OPENCL_VERSION 220
#  endif 
#endif 

#if defined(BOOST_COMPUTE_MAX_CL_VERSION) && defined(CL_TARGET_OPENCL_VERSION)
#  if BOOST_COMPUTE_MAX_CL_VERSION == 202 && CL_TARGET_OPENCL_VERSION != 220
#    error "Boost.Compute: CL_TARGET_OPENCL_VERSION definition does not match BOOST_COMPUTE_MAX_CL_VERSION"
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 201 && CL_TARGET_OPENCL_VERSION != 210
#    error "Boost.Compute: CL_TARGET_OPENCL_VERSION definition does not match BOOST_COMPUTE_MAX_CL_VERSION"
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 200 && CL_TARGET_OPENCL_VERSION != 200
#    error "Boost.Compute: CL_TARGET_OPENCL_VERSION definition does not match BOOST_COMPUTE_MAX_CL_VERSION"
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 102 && CL_TARGET_OPENCL_VERSION != 120
#    error "Boost.Compute: CL_TARGET_OPENCL_VERSION definition does not match BOOST_COMPUTE_MAX_CL_VERSION"
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 101 && CL_TARGET_OPENCL_VERSION != 110
#    error "Boost.Compute: CL_TARGET_OPENCL_VERSION definition does not match BOOST_COMPUTE_MAX_CL_VERSION"
#  elif BOOST_COMPUTE_MAX_CL_VERSION == 100 && CL_TARGET_OPENCL_VERSION != 100
#    error "Boost.Compute: CL_TARGET_OPENCL_VERSION definition does not match BOOST_COMPUTE_MAX_CL_VERSION"
#  endif 
#endif 

#endif // BOOST_COMPUTE_DETAIL_CL_VERSION_HPP
