/*
Copyright Charly Chevalier 2015
Copyright Joel Falcou 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_HARDWARE_SIMD_ARM_H
#define BOOST_PREDEF_HARDWARE_SIMD_ARM_H

#include <boost/predef/version_number.h>
#include <boost/predef/hardware/simd/arm/versions.h>

/* tag::reference[]
= `BOOST_HW_SIMD_ARM`

The SIMD extension for ARM (*if detected*).
Version number depends on the most recent detected extension.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__ARM_NEON__+` | {predef_detection}
| `+__aarch64__+` | {predef_detection}
| `+_M_ARM+` | {predef_detection}
| `+_M_ARM64+` | {predef_detection}
|===

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__ARM_NEON__+` | BOOST_HW_SIMD_ARM_NEON_VERSION
| `+__aarch64__+` | BOOST_HW_SIMD_ARM_NEON_VERSION
| `+_M_ARM+` | BOOST_HW_SIMD_ARM_NEON_VERSION
| `+_M_ARM64+` | BOOST_HW_SIMD_ARM_NEON_VERSION
|===

*/ // end::reference[]

#define BOOST_HW_SIMD_ARM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#undef BOOST_HW_SIMD_ARM
#if !defined(BOOST_HW_SIMD_ARM) && (defined(__ARM_NEON__) || defined(__aarch64__) || defined (_M_ARM) || defined (_M_ARM64))
#   define BOOST_HW_SIMD_ARM BOOST_HW_SIMD_ARM_NEON_VERSION
#endif

#if !defined(BOOST_HW_SIMD_ARM)
#   define BOOST_HW_SIMD_ARM BOOST_VERSION_NUMBER_NOT_AVAILABLE
#else
#   define BOOST_HW_SIMD_ARM_AVAILABLE
#endif

#define BOOST_HW_SIMD_ARM_NAME "ARM SIMD"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_HW_SIMD_ARM, BOOST_HW_SIMD_ARM_NAME)
