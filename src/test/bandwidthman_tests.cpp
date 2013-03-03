#include <iostream>
#include <boost/test/unit_test.hpp>

#include "bandwidthman.h"

using namespace std;

const time_t nZero = 0;

void SetTime(struct tm& ptmTime, int nSecond, int nMinute, int nHour, int nDay, int nMonth, int nYear);

BOOST_AUTO_TEST_SUITE(bandwidthman_tests)

BOOST_AUTO_TEST_CASE(bandwidthman_trackingtimes_boundaries)
{
    CBandwidthMan bmTestManager;
    CBandwidthMan::SchedulingTestData testData;
    struct tm tmTestTime;
    struct tm tmExpectedStart;
    struct tm tmExpectedReset;

    gmtime_r(&nZero, &tmTestTime);
    gmtime_r(&nZero, &tmExpectedStart);
    gmtime_r(&nZero, &tmExpectedReset);

    // Reset every minute, current time is on the minute
    SetTime(tmTestTime,         0, 59, 23, 31, 12, 1999);
    SetTime(tmExpectedStart,    0, 59, 23, 31, 12, 1999);
    SetTime(tmExpectedReset,    0,  0,  0,  1,  1, 2000);

    testData = bmTestManager.TestScheduling("* * * * *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));
    
    // Reset every minute, current time is off the minute
    SetTime(tmTestTime,         30, 59, 23, 31, 12, 1999);
    SetTime(tmExpectedStart,     0, 59, 23, 31, 12, 1999);
    SetTime(tmExpectedReset,     0,  0,  0,  1,  1, 2000);

    testData = bmTestManager.TestScheduling("* * * * *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));



    // Reset every hour on the 30th minute, current time is on the hour's 30th minute
    SetTime(tmTestTime,         0, 29, 23, 31, 12, 1999);
    SetTime(tmExpectedStart,    0, 29, 23, 31, 12, 1999);
    SetTime(tmExpectedReset,    0, 29,  0,  1,  1, 2000);

    testData = bmTestManager.TestScheduling("29 * * * *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));

    // Reset every hour on the 30th minute, current time is off the hour's 30th minute
    SetTime(tmTestTime,         0,  0,  0,  1,  1, 2000);
    SetTime(tmExpectedStart,    0, 29, 23, 31, 12, 1999);
    SetTime(tmExpectedReset,    0, 29,  0,  1,  1, 2000);

    testData = bmTestManager.TestScheduling("29 * * * *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));



    // Reset every day on the 12 hours, current time is on the day's 12 hour
    SetTime(tmTestTime,         0, 0, 11, 31, 12, 1999);
    SetTime(tmExpectedStart,    0, 0, 11, 31, 12, 1999);
    SetTime(tmExpectedReset,    0, 0, 11,  1,  1, 2000);

    testData = bmTestManager.TestScheduling("0 11 * * *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));

    // Reset every day on the 12 hour, current time is off the day's 12 hour
    SetTime(tmTestTime,         0, 0,  0,  1,  1, 2000);
    SetTime(tmExpectedStart,    0, 0, 11, 31, 12, 1999);
    SetTime(tmExpectedReset,    0, 0, 11,  1,  1, 2000);

    testData = bmTestManager.TestScheduling("0 11 * * *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));



    // Reset every week on sunday, current time is on the week's sunday
    SetTime(tmTestTime,         0, 0, 0, 26, 12, 1999);
    SetTime(tmExpectedStart,    0, 0, 0, 26, 12, 1999);
    SetTime(tmExpectedReset,    0, 0, 0,  2,  1, 2000);

    testData = bmTestManager.TestScheduling("0 0 * * 0", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));

    // Reset every week on sunday, current time is off the week's sunday
    SetTime(tmTestTime,         0, 0, 0,  1,  1, 2000);
    SetTime(tmExpectedStart,    0, 0, 0, 26, 12, 1999);
    SetTime(tmExpectedReset,    0, 0, 0,  2,  1, 2000);

    testData = bmTestManager.TestScheduling("0 0 * * 0", tmTestTime);
    
    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));



    // Reset every month on the 15th day, current time is on the month's 15th day
    SetTime(tmTestTime,         0, 0, 0, 15, 12, 1999);
    SetTime(tmExpectedStart,    0, 0, 0, 15, 12, 1999);
    SetTime(tmExpectedReset,    0, 0, 0, 15,  1, 2000);

    testData = bmTestManager.TestScheduling("0 0 15 * *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));

    // Reset every month on the 15th day, current time is off the month's 15th day
    SetTime(tmTestTime,         0, 0, 0,  1,  1, 2000);
    SetTime(tmExpectedStart,    0, 0, 0, 15, 12, 1999);
    SetTime(tmExpectedReset,    0, 0, 0, 15,  1, 2000);

    testData = bmTestManager.TestScheduling("0 0 15 * *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));



    // Reset every year on the 6th month, current time is on the year's 6th month
    SetTime(tmTestTime,         0, 0, 0, 1, 6, 1999);
    SetTime(tmExpectedStart,    0, 0, 0, 1, 6, 1999);
    SetTime(tmExpectedReset,    0, 0, 0, 1, 6, 2000);

    testData = bmTestManager.TestScheduling("0 0 1 6 *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));

    // Reset every year on the 6th month, current time is off the year's 6th month
    SetTime(tmTestTime,         0, 0, 0,  1, 1, 2000);
    SetTime(tmExpectedStart,    0, 0, 0,  1, 6, 1999);
    SetTime(tmExpectedReset,    0, 0, 0,  1, 6, 2000);

    testData = bmTestManager.TestScheduling("0 0 1 6 *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));



    // Reset every year that the first day is a sunday, current time requires long reset fallforward
    SetTime(tmTestTime,         1, 0, 0, 1, 1, 1995);
    SetTime(tmExpectedStart,    0, 0, 0, 1, 1, 1995);
    SetTime(tmExpectedReset,    0, 0, 0, 1, 1, 2006);

    testData = bmTestManager.TestScheduling("0 0 1 1 0", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));

    // Reset every year that the first day is a sunday, current time requires long start fallback
    SetTime(tmTestTime,         59, 59, 23, 31, 12, 2005);
    SetTime(tmExpectedStart,     0,  0,  0,  1,  1, 1995);
    SetTime(tmExpectedReset,     0,  0,  0,  1,  1, 2006);

    testData = bmTestManager.TestScheduling("0 0 1 1 0", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));



    // Reset every leap-day, current time is leap day
    SetTime(tmTestTime,         0, 0, 0, 29, 2, 2000);
    SetTime(tmExpectedStart,    0, 0, 0, 29, 2, 2000);
    SetTime(tmExpectedReset,    0, 0, 0, 29, 2, 2004);

    testData = bmTestManager.TestScheduling("0 0 29 2 *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));

    // Reset every leap-day, current time is not leap-day
    SetTime(tmTestTime,         0, 0, 0, 29, 2, 1900);
    SetTime(tmExpectedStart,    0, 0, 0, 29, 2, 1896);
    SetTime(tmExpectedReset,    0, 0, 0, 29, 2, 1904);

    testData = bmTestManager.TestScheduling("0 0 29 2 *", tmTestTime);

    BOOST_CHECK_EQUAL(testData.nTrackingStartTime, timegm(&tmExpectedStart));
    BOOST_CHECK_EQUAL(testData.nTrackingResetTime, timegm(&tmExpectedReset));

}

BOOST_AUTO_TEST_SUITE_END()

void SetTime(struct tm& tmTime, int nSecond, int nMinute, int nHour, int nDay, int nMonth, int nYear)
{
    tmTime.tm_sec = nSecond;
    tmTime.tm_min = nMinute;
    tmTime.tm_hour = nHour;
    tmTime.tm_mday = nDay;
    tmTime.tm_mon = nMonth - 1;
    tmTime.tm_year = nYear - 1900;
}


