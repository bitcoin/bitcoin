//
// This file contains code from jedisct1/libsodium,
// originally released under the ISC License (see COPYING.ISC)
//
// Copyright (c) 2013 - 2022 Frank Denis
//

#ifndef MINIALLOC_EXPORT_H
#define MINIALLOC_EXPORT_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#if !defined(__clang__) && !defined(__GNUC__)
# ifdef __attribute__
#  undef __attribute__
# endif
# define __attribute__(a)
#endif

#ifdef LIBRARY_STATIC
# define LIBRARY_EXPORT
#else
# if defined(_MSC_VER)
#  ifdef LIBRARY_DLL_EXPORT
#   define LIBRARY_EXPORT __declspec(dllexport)
#  else
#   define LIBRARY_EXPORT __declspec(dllimport)
#  endif
# else
#  if defined(__SUNPRO_C)
#   ifndef __GNU_C__
#    define LIBRARY_EXPORT __attribute__ (visibility(__global))
#   else
#    define LIBRARY_EXPORT __attribute__ __global
#   endif
#  elif defined(_MSG_VER)
#   define LIBRARY_EXPORT extern __declspec(dllexport)
#  else
#   define LIBRARY_EXPORT __attribute__ ((visibility ("default")))
#  endif
# endif
#endif

#endif // _MINIALLOC_EXPORT_H_
