// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "macos_appnap.h"

#include <AvailabilityMacros.h>
#include <Foundation/NSProcessInfo.h>
#include <Foundation/Foundation.h>

class CAppNapInhibitor::CAppNapImpl
{
public:
    ~CAppNapImpl()
    {
        if(activityId)
            enableAppNap();
    }

    void disableAppNap()
    {
        if (!activityId)
        {
            @autoreleasepool {
                const NSActivityOptions activityOptions =
                NSActivityUserInitiatedAllowingIdleSystemSleep &
                ~(NSActivitySuddenTerminationDisabled |
                NSActivityAutomaticTerminationDisabled);

                id processInfo = [NSProcessInfo processInfo];
                if ([processInfo respondsToSelector:@selector(beginActivityWithOptions:reason:)])
                {
                    activityId = [processInfo beginActivityWithOptions: activityOptions reason:@"Temporarily disable App Nap for bitcoin-qt."];
                    [activityId retain];
                }
            }
        }
    }

    void enableAppNap()
    {
        if(activityId)
        {
            @autoreleasepool {
                id processInfo = [NSProcessInfo processInfo];
                if ([processInfo respondsToSelector:@selector(endActivity:)])
                    [processInfo endActivity:activityId];

                [activityId release];
                activityId = nil;
            }
        }
    }

private:
    NSObject* activityId;
};

CAppNapInhibitor::CAppNapInhibitor() : impl(new CAppNapImpl()) {}

CAppNapInhibitor::~CAppNapInhibitor() = default;

void CAppNapInhibitor::disableAppNap()
{
    impl->disableAppNap();
}

void CAppNapInhibitor::enableAppNap()
{
    impl->enableAppNap();
}
