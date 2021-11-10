/*
Copyright Charly Chevalier 2015
Copyright Joel Falcou 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_HARDWARE_SIMD_X86_AMD_VERSIONS_H
#define BOOST_PREDEF_HARDWARE_SIMD_X86_AMD_VERSIONS_H

#include <boost/predef/version_number.h>

/* tag::reference[]
= `BOOST_HW_SIMD_X86_AMD_*_VERSION`

Those defines represent x86 (AMD specific) SIMD extensions versions.

NOTE: You *MUST* compare them with the predef `BOOST_HW_SIMD_X86_AMD`.
*/ // end::reference[]


// ---------------------------------

/* tag::reference[]
= `BOOST_HW_SIMD_X86_AMD_SSE4A_VERSION`

https://en.wikipedia.org/wiki/SSE4##SSE4A[SSE4A] x86 extension (AMD specific).

Version number is: *4.0.0*.
*/ // end::reference[]
#define BOOST_HW_SIMD_X86_AMD_SSE4A_VERSION BOOST_VERSION_NUMBER(4, 0, 0)

/* tag::reference[]
= `BOOST_HW_SIMD_X86_AMD_FMA4_VERSION`

https://en.wikipedia.org/wiki/FMA_instruction_set#FMA4_instruction_set[FMA4] x86 extension (AMD specific).

Version number is: *5.1.0*.
*/ // end::reference[]
#define BOOST_HW_SIMD_X86_AMD_FMA4_VERSION BOOST_VERSION_NUMBER(5, 1, 0)

/* tag::reference[]
= `BOOST_HW_SIMD_X86_AMD_XOP_VERSION`

https://en.wikipedia.org/wiki/XOP_instruction_set[XOP] x86 extension (AMD specific).

Version number is: *5.1.1*.
*/ // end::reference[]
#define BOOST_HW_SIMD_X86_AMD_XOP_VERSION BOOST_VERSION_NUMBER(5, 1, 1)

/* tag::reference[]

*/ // end::reference[]

#endif
