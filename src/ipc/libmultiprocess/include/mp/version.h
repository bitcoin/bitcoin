// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_VERSION_H
#define MP_VERSION_H

//! Major version number. Should be incremented in the master branch before
//! changes that introduce major new features or break API compatibility, if
//! there are clients relying on the previous API. (If an API changes multiple
//! times and nothing uses the intermediate changes, it is sufficient to
//! increment this only once before the first change.)
//!
//! Each time this is incremented, a new stable branch should be created. E.g.
//! when this is incremented to 8, a "v7" stable branch should be created
//! pointing at the prior merge commit. The /doc/versions.md file should also be
//! updated, noting any significant or incompatible changes made since the
//! previous version, and backported to the stable branch before it is tagged.
#define MP_MAJOR_VERSION 7

//! Minor version number. Should be incremented in stable branches after
//! backporting changes. The /doc/versions.md file in the master and stable
//! branches should also be updated to list the new minor version.
#define MP_MINOR_VERSION 0

#endif // MP_VERSION_H
