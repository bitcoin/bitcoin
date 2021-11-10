/*!
@file
Defines macros for tracking the version of the library.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_VERSION_HPP
#define BOOST_HANA_VERSION_HPP

//! @internal
//! Transforms a (version, revision, patchlevel) triple into a number of the
//! form 0xVVRRPPPP to allow comparing versions in a normalized way.
//!
//! See http://sourceforge.net/p/predef/wiki/VersionNormalization.
#define BOOST_HANA_CONFIG_VERSION(version, revision, patch) \
    (((version) << 24) + ((revision) << 16) + (patch))

//! @ingroup group-config
//! Macro expanding to the major version of the library, i.e. the `x` in `x.y.z`.
#define BOOST_HANA_MAJOR_VERSION 1

//! @ingroup group-config
//! Macro expanding to the minor version of the library, i.e. the `y` in `x.y.z`.
#define BOOST_HANA_MINOR_VERSION 7

//! @ingroup group-config
//! Macro expanding to the patch level of the library, i.e. the `z` in `x.y.z`.
#define BOOST_HANA_PATCH_VERSION 1

//! @ingroup group-config
//! Macro expanding to the full version of the library, in hexadecimal
//! representation.
//!
//! Specifically, `BOOST_HANA_VERSION` expands to an hexadecimal number of the
//! form 0xVVRRPPPP, where `VV` is the major version of the library, `RR` is
//! the minor version and `PPPP` is the patch level. This allows the version
//! of the library to be compared:
//! @snippet example/version.cpp main
//!
//!
//! @note
//! The major, minor and patch versions of the library are also available
//! individually with the `BOOST_HANA_{MAJOR,MINOR,PATCH}_VERSION` macros.
#define BOOST_HANA_VERSION                                                  \
    BOOST_HANA_CONFIG_VERSION(BOOST_HANA_MAJOR_VERSION,                     \
                              BOOST_HANA_MINOR_VERSION,                     \
                              BOOST_HANA_PATCH_VERSION)                     \
/**/

#endif // !BOOST_HANA_VERSION_HPP
