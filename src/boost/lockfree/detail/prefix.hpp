//  Copyright (C) 2009, 2016 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LOCKFREE_PREFIX_HPP_INCLUDED
#define BOOST_LOCKFREE_PREFIX_HPP_INCLUDED

/* this file defines the following macros:
   BOOST_LOCKFREE_CACHELINE_BYTES: size of a cache line
   BOOST_LOCKFREE_PTR_COMPRESSION: use tag/pointer compression to utilize parts
                                   of the virtual address space as tag (at least 16bit)
*/

#if defined(__s390__) || defined(__s390x__)
    #define BOOST_LOCKFREE_CACHELINE_BYTES 256
#elif defined(powerpc) || defined(__powerpc__) || defined(__ppc__)
    #define BOOST_LOCKFREE_CACHELINE_BYTES 128
#else
    #define BOOST_LOCKFREE_CACHELINE_BYTES 64
#endif

#include <boost/predef.h>

#if BOOST_ARCH_X86_64 || (BOOST_ARCH_ARM >= BOOST_VERSION_NUMBER(8,0,0))
#define BOOST_LOCKFREE_PTR_COMPRESSION 1
#endif

#endif /* BOOST_LOCKFREE_PREFIX_HPP_INCLUDED */
