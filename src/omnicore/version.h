#ifndef BITCOIN_OMNICORE_VERSION_H
#define BITCOIN_OMNICORE_VERSION_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#else

//
// Omni Core version information are also to be defined in configure.ac.
//
// During the configuration, this information are used for other places.
//

// Increase with every consensus affecting change
#define OMNICORE_VERSION_MAJOR       0

// Increase with every non-consensus affecting feature
#define OMNICORE_VERSION_MINOR       11

// Increase with every patch, which is not a feature or consensus affecting
#define OMNICORE_VERSION_PATCH       0

// Non-public build number/revision (usually zero)
#define OMNICORE_VERSION_BUILD       0

#endif // HAVE_CONFIG_H

#if !defined(WINDRES_PREPROC)

//
// *-res.rc includes this file, but it cannot cope with real c++ code.
// WINDRES_PREPROC is defined to indicate that its pre-processor is running.
// Anything other than a define should be guarded below:
//

#include <string>

//! Omni Core client version
static const int OMNICORE_VERSION =
                    +100000000000 * OMNICORE_VERSION_MAJOR
                    +    10000000 * OMNICORE_VERSION_MINOR
                    +        1000 * OMNICORE_VERSION_PATCH
                    +           1 * OMNICORE_VERSION_BUILD;

static const int OMNI_USERAGENT_VERSION =
                           1000000 * OMNICORE_VERSION_MAJOR
                         +   10000 * OMNICORE_VERSION_MINOR
                         +     100 * OMNICORE_VERSION_PATCH
                         +       1 * OMNICORE_VERSION_BUILD;

extern const std::string OMNI_CLIENT_NAME;

//! Returns formatted Omni Core version, e.g. "1.2.0"
const std::string OmniCoreVersion();

//! Returns formatted Bitcoin Core version, e.g. "0.10", "0.9.3"
const std::string BitcoinCoreVersion();


#endif // WINDRES_PREPROC

#endif // BITCOIN_OMNICORE_VERSION_H
