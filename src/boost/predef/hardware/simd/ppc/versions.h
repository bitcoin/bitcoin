/*
Copyright Charly Chevalier 2015
Copyright Joel Falcou 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_HARDWARE_SIMD_PPC_VERSIONS_H
#define BOOST_PREDEF_HARDWARE_SIMD_PPC_VERSIONS_H

#include <boost/predef/version_number.h>

/* tag::reference[]
= `BOOST_HW_SIMD_PPC_*_VERSION`

Those defines represent Power PC SIMD extensions versions.

NOTE: You *MUST* compare them with the predef `BOOST_HW_SIMD_PPC`.
*/ // end::reference[]

// ---------------------------------

/* tag::reference[]
= `BOOST_HW_SIMD_PPC_VMX_VERSION`

The https://en.wikipedia.org/wiki/AltiVec#VMX128[VMX] powerpc extension
version number.

Version number is: *1.0.0*.
*/ // end::reference[]
#define BOOST_HW_SIMD_PPC_VMX_VERSION BOOST_VERSION_NUMBER(1, 0, 0)

/* tag::reference[]
= `BOOST_HW_SIMD_PPC_VSX_VERSION`

The https://en.wikipedia.org/wiki/AltiVec#VSX[VSX] powerpc extension version
number.

Version number is: *1.1.0*.
*/ // end::reference[]
#define BOOST_HW_SIMD_PPC_VSX_VERSION BOOST_VERSION_NUMBER(1, 1, 0)

/* tag::reference[]
= `BOOST_HW_SIMD_PPC_QPX_VERSION`

The QPX powerpc extension version number.

Version number is: *2.0.0*.
*/ // end::reference[]
#define BOOST_HW_SIMD_PPC_QPX_VERSION BOOST_VERSION_NUMBER(2, 0, 0)

/* tag::reference[]

*/ // end::reference[]

#endif
