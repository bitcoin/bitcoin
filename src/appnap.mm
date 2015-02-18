#include "appnap.h"

#include <AvailabilityMacros.h>
#include <Foundation/NSProcessInfo.h>
#include <Foundation/Foundation.h>

class CAppNapInhibitorInt
{
public:
    id<NSObject> activityId;
};

CAppNapInhibitor::CAppNapInhibitor(const char* strReason) : priv(new CAppNapInhibitorInt)
{
#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) &&  MAC_OS_X_VERSION_MAX_ALLOWED >= 1090
    if ([[NSProcessInfo processInfo] respondsToSelector:@selector(beginActivityWithOptions:reason:)])
        priv->activityId = [[NSProcessInfo processInfo ] beginActivityWithOptions: NSActivityUserInitiatedAllowingIdleSystemSleep reason:@(strReason)];
#endif
}

CAppNapInhibitor::~CAppNapInhibitor()
{
#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) &&  MAC_OS_X_VERSION_MAX_ALLOWED >= 1090
    if ([[NSProcessInfo processInfo] respondsToSelector:@selector(endActivity:)])
        [[NSProcessInfo processInfo] endActivity:priv->activityId];
#endif
    delete priv;
}
