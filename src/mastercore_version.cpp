#include "mastercore_version.h"

#include "clientversion.h"
#include "tinyformat.h"

#include <string>

#ifdef HAVE_BUILD_INFO
#    include "build.h"
#endif

#ifndef COMMIT_ID
#   ifdef GIT_ARCHIVE
#       define COMMIT_ID "$Format:%h$"
#   elif defined(BUILD_SUFFIX)
#       define COMMIT_ID STRINGIZE(BUILD_SUFFIX)
#   else
#       define COMMIT_ID ""
#   endif
#endif

#ifndef BUILD_DATE
#    ifdef GIT_COMMIT_DATE
#        define BUILD_DATE GIT_COMMIT_DATE
#    else
#        define BUILD_DATE __DATE__ ", " __TIME__
#    endif
#endif

//! Returns formatted Omni Core version, e.g. "0.0.9.1-dev"
const std::string OmniCoreVersion()
{
    // TODO: replace zeros at some point
    if (OMNICORE_VERSION_PATCH) {
        return strprintf("0.0.%d.%d.%d%s",
                OMNICORE_VERSION_MAJOR,
                OMNICORE_VERSION_MINOR,
                OMNICORE_VERSION_PATCH,
                OMNICORE_VERSION_TYPE);
    } else {
        return strprintf("0.0.%d.%d%s",
                OMNICORE_VERSION_MAJOR,
                OMNICORE_VERSION_MINOR,
                OMNICORE_VERSION_TYPE);
    }
}

//! Returns formatted Bitcoin Core version, e.g. "0.10", "0.9.3"
const std::string BitcoinCoreVersion()
{
    if (CLIENT_VERSION_BUILD) {
        return strprintf("%d.%d.%d.%d",
                CLIENT_VERSION_MAJOR,
                CLIENT_VERSION_MINOR,
                CLIENT_VERSION_REVISION,
                CLIENT_VERSION_BUILD);
    } else {
        return strprintf("%d.%d.%d",
                CLIENT_VERSION_MAJOR,
                CLIENT_VERSION_MINOR,
                CLIENT_VERSION_REVISION);
    }
}

//! Returns build date
const std::string BuildDate()
{
    return std::string(BUILD_DATE);
}

//! Returns commit identifier, if available
const std::string BuildCommit()
{
    return std::string(COMMIT_ID);
}
