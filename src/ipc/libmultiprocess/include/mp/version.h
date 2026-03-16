// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_VERSION_H
#define MP_VERSION_H

//! @file
//! @brief Major and minor version numbers
//!
//! Versioning uses a cruder form of SemVer where the major number is
//! incremented with all significant changes, regardless of whether they are
//! backward compatible, and the minor number is treated like a patch level and
//! only incremented when a fix or backport is applied to an old branch.

//! Major version number. Should be incremented after any release or external
//! usage of the library (like a subtree update) so the previous number
//! identifies that release. Should also be incremented before any change that
//! breaks backward compatibility or introduces nontrivial features, so
//! downstream code can use it to detect compatibility.
//!
//! Each time this is incremented, a new stable branch should be created. E.g.
//! when this is incremented to 8, a "v7" stable branch should be created
//! pointing at the prior merge commit. The /doc/versions.md file should also be
//! updated, noting any significant or incompatible changes made since the
//! previous version.
#define MP_MAJOR_VERSION 8

//! Minor version number. Should be incremented in stable branches after
//! backporting changes. The /doc/versions.md file should also be updated to
//! list the new minor version.
#define MP_MINOR_VERSION 0

#endif // MP_VERSION_H
