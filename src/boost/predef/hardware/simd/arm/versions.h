/*
Copyright Charly Chevalier 2015
Copyright Joel Falcou 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_HARDWARE_SIMD_ARM_VERSIONS_H
#define BOOST_PREDEF_HARDWARE_SIMD_ARM_VERSIONS_H

#include <boost/predef/version_number.h>

/* tag::reference[]
= `BOOST_HW_SIMD_ARM_*_VERSION`

Those defines represent ARM SIMD extensions versions.

NOTE: You *MUST* compare them with the predef `BOOST_HW_SIMD_ARM`.
*/ // end::reference[]

// ---------------------------------

/* tag::reference[]
= `BOOST_HW_SIMD_ARM_NEON_VERSION`

The https://en.wikipedia.org/wiki/ARM_architecture#Advanced_SIMD_.28NEON.29[NEON]
ARM extension version number.

Version number is: *1.0.0*.
*/ // end::reference[]
#define BOOST_HW_SIMD_ARM_NEON_VERSION BOOST_VERSION_NUMBER(1, 0, 0)

/* tag::reference[]

*/ // end::reference[]

#endif
