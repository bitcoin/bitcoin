#ifndef OMNICORE_VERSION_H
#define OMNICORE_VERSION_H

#include <string>

// Increase with every consensus affecting change
#define OMNICORE_VERSION_MAJOR   9

// Increase with every non-consensus affecting feature
#define OMNICORE_VERSION_MINOR   2

// Increase with every patch, which is not a feature or consensus affecting
#define OMNICORE_VERSION_PATCH   0

// Use "-dev" for development versions, switch to "-rel" for tags
#define OMNICORE_VERSION_TYPE    "-dev"


//! Omni Core client version
static const int OMNICORE_VERSION =
                        +  100000 * OMNICORE_VERSION_MAJOR
                        +     100 * OMNICORE_VERSION_MINOR
                        +       1 * OMNICORE_VERSION_PATCH;

//! Returns formatted Omni Core version, e.g. "0.0.9.1-dev"
const std::string OmniCoreVersion();

//! Returns formatted Bitcoin Core version, e.g. "0.10", "0.9.3"
const std::string BitcoinCoreVersion();

//! Returns build date
const std::string BuildDate();

//! Returns commit identifier, if available
const std::string BuildCommit();

#endif // OMNICORE_VERSION_H
