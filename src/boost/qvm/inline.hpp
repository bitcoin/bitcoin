/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_QVM_FORCEINLINE
#   if defined(_MSC_VER)
#       define BOOST_QVM_FORCEINLINE __forceinline
#   elif defined(__GNUC__) && __GNUC__>3
#       define BOOST_QVM_FORCEINLINE inline __attribute__ ((always_inline))
#   else
#       define BOOST_QVM_FORCEINLINE inline
#   endif
#endif

#ifndef BOOST_QVM_INLINE
#define BOOST_QVM_INLINE inline
#endif

#ifndef BOOST_QVM_INLINE_TRIVIAL
#define BOOST_QVM_INLINE_TRIVIAL BOOST_QVM_FORCEINLINE
#endif

#ifndef BOOST_QVM_INLINE_CRITICAL
#define BOOST_QVM_INLINE_CRITICAL BOOST_QVM_FORCEINLINE
#endif

#ifndef BOOST_QVM_INLINE_OPERATIONS
#define BOOST_QVM_INLINE_OPERATIONS BOOST_QVM_INLINE
#endif

#ifndef BOOST_QVM_INLINE_RECURSION
#define BOOST_QVM_INLINE_RECURSION BOOST_QVM_INLINE_OPERATIONS
#endif
