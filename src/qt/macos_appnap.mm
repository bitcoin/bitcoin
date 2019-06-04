// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "macos_appnap.h"

#include <AvailabilityMacros.h>
#include <Foundation/NSProcessInfo.h>
#include <Foundation/Foundation.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

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

    void enableIdleSleepPrevention()
    {
        if (m_assertion_id) return;

        CFStringRef reasonForActivity = CFSTR("Downloading blockchain");

        IOPMAssertionCreateWithName(
          kIOPMAssertionTypeNoIdleSleep,
          kIOPMAssertionLevelOn,
          reasonForActivity,
          &m_assertion_id);
    }

    void disableIdleSleepPrevention()
    {
        if (!m_assertion_id) return;

        IOPMAssertionRelease(m_assertion_id);
        m_assertion_id = 0;
    }

private:
    NSObject* activityId{nil};
    IOPMAssertionID m_assertion_id{0};
};

CAppNapInhibitor::CAppNapInhibitor() : impl(new CAppNapImpl()) {}

CAppNapInhibitor::~CAppNapInhibitor() = default;

void CAppNapInhibitor::setAppNapEnabled(bool enabled)
{
    if (enabled) {
        impl->enableAppNap();
        impl->enableIdleSleepPrevention();
    } else {
        impl->disableAppNap();
        impl->disableIdleSleepPrevention();
    }
}
