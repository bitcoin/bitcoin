#include "bandwidthman.h"

#include "db.h"
#include "net.h"

#include <sstream>

using namespace std;

const time_t nZero = 0;

CBandwidthMan::CBandwidthMan()
{
}

void CBandwidthMan::LoadConfiguration()
{
    if (mapArgs.count("-bandwidthresettimes"))
    {
        try
        {
            ParseResetTimes(mapArgs["-bandwidthresettimes"]);
            PrintResetTimes();
        } catch (const char* ex) {
            printf("%s.\n", ex);
        }
    }

    if (mapArgs.count("-bandwidthsentmax"))
    {
        nMaxTotalSentBytes = ParseBandwidthValue(mapArgs["-bandwidthsentmax"]);
    } else {
        nMaxTotalSentBytes = (uint64) -1;
    }

    for (map<string, string>::iterator it = mapArgs.begin(); it != mapArgs.end(); ++it)
    {
        if (it->first.substr(0, 18) == "-bandwidthsentmax_")
            mapMaxCommandSentBytes[it->first.substr(18)] = ParseBandwidthValue(it->second);
    }
     
    
}

CBandwidthMan::SchedulingTestData CBandwidthMan::TestScheduling(string strCronLine, struct tm tmTestTime)
{
    SchedulingTestData stdTestData;

    LOCK(cs);

    // Save current data to be resored later
    ResetInformation riOldResetTimes = riResetTimes;
    time_t nOldTrackingStartTime = nTrackingStartTime;
    time_t nOldTrackingResetTime = nTrackingResetTime;

    // Test using passed in values
    ParseResetTimes(strCronLine);
    RecalculateStartResetTimes(&tmTestTime);

    // Save test data
    stdTestData.riResetTimes = riResetTimes;
    stdTestData.nTrackingStartTime = nTrackingStartTime;
    stdTestData.nTrackingResetTime = nTrackingResetTime;

    // Reset data to old values
    riResetTimes = riOldResetTimes;
    nTrackingStartTime = nOldTrackingStartTime;
    nTrackingResetTime = nOldTrackingResetTime;

    
    return stdTestData;
}

// Determine if we should be allowed to send command.
//   Returns true if allowed to be sent, data asummed to be send if allowed by the function.
bool CBandwidthMan::AllowSendCommand(string strCommand, int nSize)
{
    if (time(NULL) > nTrackingResetTime)
        ResetTrackingStatistics();

///// Temp
    int64 nStart = GetTimeMillis();

    CBandwidthDB bdb;
    bdb.Write(bandwidthman);

    printf("Flushed bandwidth info to bandwidth.dat  %"PRI64d"ms\n",
           GetTimeMillis() - nStart);
///// End temp
    
    if (nTotalSentBytes >= nMaxTotalSentBytes)
        return false;

    if (mapMaxCommandSentBytes.count(strCommand) && mapCommandSentBytes[strCommand] + nSize >= mapMaxCommandSentBytes[strCommand])
        return false;

    cout << strCommand << " was " << mapCommandSentBytes[strCommand] << " and now is " << mapCommandSentBytes[strCommand] + nSize << endl;
    nTotalSentBytes += nSize;
    mapCommandSentBytes[strCommand] += nSize;
    return true;
}


void CBandwidthMan::ParseResetTimes(string strResetTimes)
{
    const char nStartValues[]  = { 0,  0,  1,  1, 0};
    const char nModValues[]    = {60, 24, 31, 12, 7};
    const char nOffsetValues[] = { 0,  0,  0, -1, 0};

    // minutes      hours       dayOfMonth  Month   dayOfWeek
    // 0            0           1           *       *           // Resets each month
    // *            *           *           *       *           // Resets each minutes
    // */5          *           *           *       *           // Resets every 5 minutes
    istringstream isResetTimes(strResetTimes);

    for (int x = 0; x < 5; ++x)
    {
        string strResetTimesElement;
        isResetTimes >> strResetTimesElement;

        set<char>* psetCurrentUnit;
        switch (x)
        {
            case 0:
                psetCurrentUnit = &riResetTimes.setMinutes;
                break;
            case 1:
                psetCurrentUnit = &riResetTimes.setHours;
                break;
            case 2:
                psetCurrentUnit = &riResetTimes.setDaysOfMonth;
                break;
           case 3:
                psetCurrentUnit = &riResetTimes.setMonths;
                break;
           case 4:
                psetCurrentUnit = &riResetTimes.setDaysOfWeek;
                break;
        }

        int nFrom = -1;
        int nTo = -1;
        int nSkip = 0;
        
        for (size_t y = 0; y < strResetTimesElement.length(); ++y)
        {
            if (strResetTimesElement[y] == '*')
            {
                if (y != 0)
                    throw "Unable to parse reset times string1";

                nFrom = nStartValues[x];
                nTo = nStartValues[x] + nModValues[x] - 1;
                nSkip = 1;

            } else if (strResetTimesElement[y] >= '0' && strResetTimesElement[y] <= '9') {
                int nParsedNumber = 0;

                do
                {
                    nParsedNumber *= 10;
                    nParsedNumber += (int) (strResetTimesElement[y] - '0');

                    ++y;
                } while (y < strResetTimesElement.length() && strResetTimesElement[y] >= '0' && strResetTimesElement[y] <= '9');
                
                --y;
                nParsedNumber %= nModValues[x];

                if (nFrom < 0)
                    nFrom = nParsedNumber;
                else
                    nTo = nParsedNumber;
                
                nSkip = 1;

            } else if (strResetTimesElement[y] == '/') {
                printf("%d: / found.\n", x);
                if (y == strResetTimesElement.length() - 1 || strResetTimesElement[y+1] < '0' || strResetTimesElement[y+1] > '9')
                    throw "Unable to parse reset times string3";
                
                nSkip = 0;
                ++y;

                do
                {
                    nSkip *= 10;
                    nSkip += (int) (strResetTimesElement[y] - '0');

                    ++y;
                } while (y < strResetTimesElement.length() && strResetTimesElement[y] >= '0' && strResetTimesElement[y] <= '9');

                if (nSkip >= nStartValues[x] + nModValues[x])
                {
                    nSkip -= nStartValues[x];
                    nSkip %= nModValues[x];
                    nSkip += nStartValues[x];
                }
                printf("From %d, To %d, Skip %d\n", nFrom, nTo, nSkip);

                if (y != strResetTimesElement.length() && strResetTimesElement[y] != ',')
                    throw "Unable to parse reset times string4";

            } else if (strResetTimesElement[y] == '-') {
                if (nTo > 0 || y == strResetTimesElement.length() - 1)
                    throw "Unable to parse reset times string5";
            } else if (strResetTimesElement[y] == ',') {
                if (y == strResetTimesElement.length() - 1)
                    throw "Unable to parse reset times string9";

                AddResetTimesRange(nFrom, nTo, nSkip, psetCurrentUnit, nStartValues[x], nModValues[x], nOffsetValues[x]);

                nFrom = -1;
                nTo = -1;
                nSkip = 0;
            }
        }
        
        AddResetTimesRange(nFrom, nTo, nSkip, psetCurrentUnit, nStartValues[x], nModValues[x], nOffsetValues[x]);
    }
}

void CBandwidthMan::PrintResetTimes() const
{
    const set<char>* psetCurrentUnit[] = {&riResetTimes.setMinutes, &riResetTimes.setHours, &riResetTimes.setDaysOfMonth, &riResetTimes.setMonths, &riResetTimes.setDaysOfWeek};
    for (unsigned int x = 0; x < sizeof(psetCurrentUnit); ++x)
    {

        printf("Unit %d values:\n\t", x);
        for (set<char>::const_iterator it = psetCurrentUnit[x]->begin(); it != psetCurrentUnit[x]->end(); ++it)
            printf("%hhd, ", *it);
    
        printf("\n\n");
    }
}

void CBandwidthMan::PrintCurrentBandwidth() const
{
    printf("Bandwidth Caps (size=%d):\n", mapMaxCommandSentBytes.size());

    for (map<string, uint64>::const_iterator it = mapMaxCommandSentBytes.begin(); it != mapMaxCommandSentBytes.end(); ++it)
        printf("\t%s - %lld\n", it->first.c_str(), it->second);

    printf("\tTotal - %lld\n", nMaxTotalSentBytes);


    printf("Bandwidth map (size=%d):\n", mapCommandSentBytes.size());

    for (map<string, uint64>::const_iterator it = mapCommandSentBytes.begin(); it != mapCommandSentBytes.end(); ++it)
        printf("\t%s - %lld\n", it->first.c_str(), it->second);
    
    printf("\tTotal - %lld\n" , nTotalSentBytes);
    printf("\n");
}



void CBandwidthMan::ResetTrackingStatistics()
{
    // Clear sent bytes map
    mapCommandSentBytes.clear();
    nTotalSentBytes = 0;
    

    // Get current time to use in future calculations
    time_t nCurrentTime = time(NULL);
    struct tm tmCurrentTime;
    gmtime_r(&nCurrentTime, &tmCurrentTime);
    RecalculateStartResetTimes(&tmCurrentTime);
}

void CBandwidthMan::RecalculateStartResetTimes(struct tm* ptmCurrentTime)
{

    // Calculate previous reset time, this will be the new nTrackingStartTime
    {
        struct tm tmNewTrackingStartTime;
        gmtime_r(&nZero, &tmNewTrackingStartTime);
       
        // Set year to current year
        tmNewTrackingStartTime.tm_year = ptmCurrentTime->tm_year;

        // Set month, adjust year if needed
        SetPreviousMonth(tmNewTrackingStartTime, ptmCurrentTime->tm_mon);

        // Set day, adjust month and/or year if needed
        SetPreviousDay(tmNewTrackingStartTime, ptmCurrentTime, ptmCurrentTime->tm_mday);

        // Set hour, adjust day, month, and/or year if needed
        SetPreviousHour(tmNewTrackingStartTime, ptmCurrentTime, ptmCurrentTime->tm_hour);

        // Set minute, adjust hour, day, month, and/or year if needed
        SetPreviousMinute(tmNewTrackingStartTime, ptmCurrentTime, ptmCurrentTime->tm_min);

        nTrackingStartTime = timegm(&tmNewTrackingStartTime);
    }


    // Calculate next reset time, this will be the new nTrackingResetTime
    {
        struct tm tmNewTrackingResetTime;
        gmtime_r(&nZero, &tmNewTrackingResetTime);

        // Set minute, adjust hour, day, month, and/or year if needed
        SetNextMinute(tmNewTrackingResetTime, ptmCurrentTime->tm_min);

        // Set hour, adjust day, month, and/or year if needed
        SetNextHour(tmNewTrackingResetTime, ptmCurrentTime, ptmCurrentTime->tm_hour);
        
        // Set day, adjust month and/or year if needed
        SetNextDay(tmNewTrackingResetTime, ptmCurrentTime, ptmCurrentTime->tm_mon, ptmCurrentTime->tm_year, ptmCurrentTime->tm_mday);

        // Set month, adjust year if needed
        SetNextMonth(tmNewTrackingResetTime, ptmCurrentTime, ptmCurrentTime->tm_year, ptmCurrentTime->tm_mon);

        // Set year to current yea
        SetNextYear(tmNewTrackingResetTime, ptmCurrentTime);
    
        nTrackingResetTime = timegm(&tmNewTrackingResetTime);
    }
}


char CBandwidthMan::LastDayOfMonth(int nYearSince1900, char nMonth)
{
    const char nDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int nYear = nYearSince1900 + 1900;

    // Select the last valid day of the month
    char nLastDay = nDaysInMonth[(int) nMonth];

    // Handle leap day
    if (nMonth == 1 && (nYear % 400 == 0 || (nYear % 100 != 0 && nYear % 4 == 0)))
        ++nLastDay;

    return nLastDay;
}

bool CBandwidthMan::DayIsFiltered(int nYearSince1900, char nMonth, char nDay)
{
    char nDayOfWeek;
    int nYear = nYearSince1900 + 1900;

    // Use Zeller's algorithm to calculate day of week
    {
        // Shift year and month as required for algorithm
        if (nMonth < 2)
        {
            // Shift January and February by 13, subtract the year by one
            nMonth += 13;
            nYear -= 1;
        } else {
            // Shift other months by 1
            nMonth += 1;
        }
        
        char nYearLo = nYear % 100;
        int nYearHi = nYear / 100;

        // Calculate via Zeller's algorithm
        nDayOfWeek = ( nDay + (((nMonth + 1) * 13) / 5) + nYearLo + (nYearLo / 4) + (nYearHi / 4) + (5 * nYearHi)) % 7;

        // Shift day of week to start on Sunday instead of Saturday
        nDayOfWeek = (nDayOfWeek + 6) % 7;
    }

    return (riResetTimes.setDaysOfWeek.find(nDayOfWeek) == riResetTimes.setDaysOfWeek.end());
}

void CBandwidthMan::SetPreviousMonth(struct tm& tmNewTrackingStartTime, char nStartMonth)
{
    // Get the month after the current month
    set<char>::iterator itStartMonth = riResetTimes.setMonths.upper_bound(nStartMonth);
    
    // Loop iterator if at beginning, go to previous year and loop month to end
    if (itStartMonth == riResetTimes.setMonths.begin())
    {
        --tmNewTrackingStartTime.tm_year;
        itStartMonth = riResetTimes.setMonths.end();
    }

    // Move to the current month (if valid) or the closest previous valid month
    --itStartMonth;

    tmNewTrackingStartTime.tm_mon = *itStartMonth;
}

void CBandwidthMan::SetPreviousDay(struct tm& tmNewTrackingStartTime, const struct tm* ptmCurrentTime, char nStartDay)
{
    set<char>::iterator itStartDay;

    if (tmNewTrackingStartTime.tm_mon == ptmCurrentTime->tm_mon && tmNewTrackingStartTime.tm_year == ptmCurrentTime->tm_year)
    {
        // Get the day after the current day
        itStartDay = riResetTimes.setDaysOfMonth.upper_bound(nStartDay);
        
        // If iterator is not at beginning, use the current day or the previous valid
        if (itStartDay != riResetTimes.setDaysOfMonth.begin())
        {
            // Move to current day (if valid) or the closest previous valid day
            --itStartDay;
        } else {
            // Loop to the previous valid month, use last valid day (via fallthrough to next if statement below)
            SetPreviousMonth(tmNewTrackingStartTime, tmNewTrackingStartTime.tm_mon - 1);
        }
    }

    // Make separate if, instead of else, to allow fallthrough from previous if statement
    if (tmNewTrackingStartTime.tm_mon != ptmCurrentTime->tm_mon || tmNewTrackingStartTime.tm_year != ptmCurrentTime->tm_year)
    {
        // Get the first valid day after the last day of the month
        itStartDay = riResetTimes.setDaysOfMonth.upper_bound(LastDayOfMonth(tmNewTrackingStartTime.tm_year, tmNewTrackingStartTime.tm_mon));

        // Should never be at beginning of the set
        assert(itStartDay != riResetTimes.setDaysOfMonth.begin());

        --itStartDay;
        
    }


    while (DayIsFiltered(tmNewTrackingStartTime.tm_year, tmNewTrackingStartTime.tm_mon, *itStartDay))
    {
        if (itStartDay == riResetTimes.setDaysOfMonth.begin())
        {
            SetPreviousMonth(tmNewTrackingStartTime, tmNewTrackingStartTime.tm_mon - 1);
            itStartDay = riResetTimes.setDaysOfMonth.upper_bound(LastDayOfMonth(tmNewTrackingStartTime.tm_year, tmNewTrackingStartTime.tm_mon));
        }
        
        // Move to the current day (if valid) or the closest previous valid day
        --itStartDay;
    } 

    tmNewTrackingStartTime.tm_mday = *itStartDay;
}

void CBandwidthMan::SetPreviousHour(struct tm& tmNewTrackingStartTime, const struct tm* ptmCurrentTime, char nStartHour)
{
    set<char>::iterator itStartHour;
    
    if (tmNewTrackingStartTime.tm_mday == ptmCurrentTime->tm_mday && tmNewTrackingStartTime.tm_mon == ptmCurrentTime->tm_mon && tmNewTrackingStartTime.tm_year == ptmCurrentTime->tm_year)
    {
        // Get the hour after the current gour
        itStartHour = riResetTimes.setHours.upper_bound(nStartHour);

        // If iterator is not at beginning, use the current hour or the previous valid
        if (itStartHour != riResetTimes.setHours.begin())
        {
            // Move to current hour (if valid) or the closest previous valid hour
            --itStartHour;
        } else {
            // Loop to the previous valid day, use last valid hour (via fallthrough to next if statement below)
            SetPreviousDay(tmNewTrackingStartTime, ptmCurrentTime, tmNewTrackingStartTime.tm_mday - 1);
        }
    }

    // Make separate if, instead of else, to allow fallthrough from previous if statement
    if (tmNewTrackingStartTime.tm_mday != ptmCurrentTime->tm_mday || tmNewTrackingStartTime.tm_mon != ptmCurrentTime->tm_mon || tmNewTrackingStartTime.tm_year != ptmCurrentTime->tm_year)
    {
        // Get the last hour
        itStartHour = --(riResetTimes.setHours.end());
    }

    tmNewTrackingStartTime.tm_hour = *itStartHour;
}

void CBandwidthMan::SetPreviousMinute(struct tm& tmNewTrackingStartTime, const struct tm* ptmCurrentTime, char nStartMinute)
{
    set<char>::iterator itStartMinute;

    if (tmNewTrackingStartTime.tm_hour == ptmCurrentTime->tm_hour && tmNewTrackingStartTime.tm_mday == ptmCurrentTime->tm_mday && tmNewTrackingStartTime.tm_mon == ptmCurrentTime->tm_mon && tmNewTrackingStartTime.tm_year == ptmCurrentTime->tm_year)
    {
        // Get the day after the current day
        itStartMinute = riResetTimes.setMinutes.upper_bound(nStartMinute);

        // If iterator is not at beginning, use the current minute or the previous valid
        if (itStartMinute != riResetTimes.setMinutes.begin())
        {
            // Move to current minute (if valid) or the closest prevoius valid
            --itStartMinute;
        } else {
            // Loop to the previous valid hour, use last valid minute (via fallthrough to next if statement below)
            SetPreviousHour(tmNewTrackingStartTime, ptmCurrentTime, tmNewTrackingStartTime.tm_hour - 1);
        }
    }

    // Make separate if, instead of else, to allow fallthrough from previous if statement
    if (tmNewTrackingStartTime.tm_hour != ptmCurrentTime->tm_hour || tmNewTrackingStartTime.tm_mday != ptmCurrentTime->tm_mday || tmNewTrackingStartTime.tm_mon != ptmCurrentTime->tm_mon || tmNewTrackingStartTime.tm_year != ptmCurrentTime->tm_year)
    {
        // Get the last minute
        itStartMinute = --(riResetTimes.setMinutes.end());
    }

    tmNewTrackingStartTime.tm_min = *itStartMinute;
}



void CBandwidthMan::SetNextYear(struct tm& tmNewTrackingResetTime, const struct tm* ptmCurrentTime)
{
    int nStartYear;

    if (tmNewTrackingResetTime.tm_mon > ptmCurrentTime->tm_mon || (tmNewTrackingResetTime.tm_mon == ptmCurrentTime->tm_mon && (tmNewTrackingResetTime.tm_mday > ptmCurrentTime->tm_mday || tmNewTrackingResetTime.tm_hour > ptmCurrentTime->tm_hour || tmNewTrackingResetTime.tm_min > ptmCurrentTime->tm_min)))
    {
        nStartYear = ptmCurrentTime->tm_year;
    } else {
        nStartYear = ptmCurrentTime->tm_year + 1;

        // If our original guess wasn't right, try again with the new month
        SetNextMonth(tmNewTrackingResetTime, ptmCurrentTime, nStartYear, 0);
    }

    bool fIsValid = false;

    do
    {
        if (tmNewTrackingResetTime.tm_mon == -1)
        {
            ++nStartYear;
            SetNextMonth(tmNewTrackingResetTime, ptmCurrentTime, nStartYear, 0);
        } else {
            fIsValid = true;
        }
    } while (fIsValid == false);

    tmNewTrackingResetTime.tm_year = nStartYear;
}

void CBandwidthMan::SetNextMonth(struct tm& tmNewTrackingResetTime, const struct tm* ptmCurrentTime, int nTestYear, char nStartMonth)
{
    set<char>::iterator itStartMonth;
    
    // Allow getting the same day if we have changed days and not looped or if we have changed minutes or days
    if (tmNewTrackingResetTime.tm_mday > ptmCurrentTime->tm_mday || (tmNewTrackingResetTime.tm_mday == ptmCurrentTime->tm_mday && (tmNewTrackingResetTime.tm_hour > ptmCurrentTime->tm_hour || tmNewTrackingResetTime.tm_min > ptmCurrentTime->tm_min)) || nTestYear != ptmCurrentTime->tm_year)
    {
        // Get the current day or next valid day
        itStartMonth = riResetTimes.setMonths.lower_bound(nStartMonth);
    } else {
        // Get the next valid day
        itStartMonth = riResetTimes.setMonths.upper_bound(nStartMonth); 
    }

    if (itStartMonth == riResetTimes.setMonths.end())
        itStartMonth = riResetTimes.setMonths.begin();

    // If our original guess wasn't right, try again with the new month
    if ((*itStartMonth != ptmCurrentTime->tm_mon || nTestYear != ptmCurrentTime->tm_year) && (tmNewTrackingResetTime.tm_mday == -1 || DayIsFiltered(tmNewTrackingResetTime.tm_mday, *itStartMonth, nTestYear)))
        SetNextDay(tmNewTrackingResetTime, ptmCurrentTime, *itStartMonth, nTestYear, 1);
    
    bool fIsValid = false;
    size_t nCount = 0;

    do
    {
        // If we failed to get a day last time or the day was invalid for this month
        if (tmNewTrackingResetTime.tm_mday == -1 || tmNewTrackingResetTime.tm_mday > LastDayOfMonth(nTestYear, *itStartMonth))
        {
            ++itStartMonth;
            if (itStartMonth == riResetTimes.setMonths.end())
                itStartMonth = riResetTimes.setMonths.begin();

            ++nCount;
            
            if (nCount == riResetTimes.setMonths.size())
            {
                itStartMonth = riResetTimes.setMonths.end();
                break;
            }

            SetNextDay(tmNewTrackingResetTime, ptmCurrentTime, *itStartMonth, nTestYear, 1);
        } else {
            fIsValid = true;
        }
    } while (!fIsValid);
   

    if (itStartMonth != riResetTimes.setMonths.end())
        tmNewTrackingResetTime.tm_mon = *itStartMonth;
    else
        tmNewTrackingResetTime.tm_mon = -1;
}

void CBandwidthMan::SetNextDay(struct tm& tmNewTrackingResetTime, const struct tm* ptmCurrentTime, char nTestMonth, int nTestYear, char nStartDay)
{
    set<char>::iterator itStartDay;

    // Allow getting the same day if we have changed hours and not looped or if we have changed minutes, months, or years
    if (tmNewTrackingResetTime.tm_hour > ptmCurrentTime->tm_hour || (tmNewTrackingResetTime.tm_hour == ptmCurrentTime->tm_hour && tmNewTrackingResetTime.tm_min > ptmCurrentTime->tm_min) || nTestYear != ptmCurrentTime->tm_year || nTestMonth != ptmCurrentTime->tm_mon)
    {
        // Get the current day or next valid day
        itStartDay = riResetTimes.setDaysOfMonth.lower_bound(nStartDay);
    } else {
        // Get the next valid day after the current day
        itStartDay = riResetTimes.setDaysOfMonth.upper_bound(nStartDay);
    }

    if (itStartDay == riResetTimes.setDaysOfMonth.end())
        itStartDay = riResetTimes.setDaysOfMonth.begin();


    size_t nCount = 0;

    while (DayIsFiltered(nTestYear, nTestMonth, *itStartDay))
    {
        ++itStartDay;
        ++nCount;
        
        if (itStartDay != riResetTimes.setDaysOfMonth.end() && *itStartDay > LastDayOfMonth(nTestYear, nTestMonth))
        {
            do
            {
                ++itStartDay;
                ++nCount;
            } while (itStartDay != riResetTimes.setDaysOfMonth.end());
        }

        // Loop back to beginning
        if (itStartDay == riResetTimes.setDaysOfMonth.end())
            itStartDay = riResetTimes.setDaysOfMonth.begin();

        if (nCount >= riResetTimes.setDaysOfMonth.size())
        {
            itStartDay = riResetTimes.setDaysOfMonth.end();
            break;
        }
    }

    if (itStartDay != riResetTimes.setDaysOfMonth.end())
        tmNewTrackingResetTime.tm_mday = *itStartDay;
    else
        tmNewTrackingResetTime.tm_mday = -1;
}

void CBandwidthMan::SetNextHour(struct tm& tmNewTrackingResetTime, const struct tm* ptmCurrentTime, char nStartHour)
{
    set<char>::iterator itStartHour;
    
    // Allow getting the same hour if we have changed minutes and not looped
    if (tmNewTrackingResetTime.tm_min > ptmCurrentTime->tm_min)
    {
        // Get the current hour or next valid hour
        itStartHour = riResetTimes.setHours.lower_bound(nStartHour);
    } else {
        // Get the next valid hour after the current hour
        itStartHour = riResetTimes.setHours.upper_bound(nStartHour);

    }

    // If the iterator at end, loop the start hour to the beginning
    if (itStartHour == riResetTimes.setHours.end())
        itStartHour = riResetTimes.setHours.begin();

    tmNewTrackingResetTime.tm_hour = *itStartHour;
}

void CBandwidthMan::SetNextMinute(struct tm& tmNewTrackingResetTime, char nStartMinute)
{
    set<char>::iterator itStartMinute;

    // Get the minute after the current minute
    itStartMinute = riResetTimes.setMinutes.upper_bound(nStartMinute);

    // If iterator is at end, use the first valid minute
    if (itStartMinute == riResetTimes.setMinutes.end())
        itStartMinute = riResetTimes.setMinutes.begin();

    tmNewTrackingResetTime.tm_min = *itStartMinute;
}







/*      Private static functions     */

void CBandwidthMan::AddResetTimesRange(int nFrom, int nTo, int nSkip, set<char>* psetCurrentUnit, int nStartValue, int nModValue, int nOffsetValue)
{

    if (nSkip == 0)
    {
        if (nSkip == 0)
            throw "Unable to parse reset times string6";
    }

    if (nTo < 0)
        nTo = nFrom;

    if (nTo >= nStartValue + nModValue)
    {
        nTo -= nStartValue;
        nTo %= nModValue;
        nTo += nStartValue;
    }

    // Loop as long as we are not in an infinite loop
    set<char> setAddedValues;
    while (setAddedValues.find(nFrom) == setAddedValues.end())
    {
        psetCurrentUnit->insert(nFrom + nOffsetValue);
        setAddedValues.insert(nFrom);

        // Stop when nFrom reaches nTo
        if (nFrom == nTo)
            break;

        // Advance nFrom and wrap as needed
        nFrom += nSkip;
        if (nFrom >= nStartValue + nModValue)
        {
            nFrom -= nStartValue;
            nFrom %= nModValue;
            nFrom += nStartValue;
        }
    }
}

uint64 CBandwidthMan::ParseBandwidthValue(string strBandwidthValue)
{
    uint64 nBandwidthValue = 0;

    for (size_t x = 0; x < strBandwidthValue.length(); ++x)
    {
        bool fRequireLast = false;
        if (strBandwidthValue[x] >= '0' && strBandwidthValue[x] <= '9')
        {
            nBandwidthValue *= 10;
            nBandwidthValue += (int) (strBandwidthValue[x] - '0');
        } else if (strBandwidthValue[x] == 'b' || strBandwidthValue[x] == 'B') {
            fRequireLast = true;
        } else if (strBandwidthValue[x] == 'k' || strBandwidthValue[x] == 'K') {
            nBandwidthValue *= (uint64) 1024;
            fRequireLast = true;
        } else if (strBandwidthValue[x] == 'm' || strBandwidthValue[x] == 'M') {
            nBandwidthValue *= (uint64) 1024 * 1024;
            fRequireLast = true;
        } else if (strBandwidthValue[x] == 'g' || strBandwidthValue[x] == 'G') {
            nBandwidthValue *= (uint64) 1024 * 1024 * 1024;
            fRequireLast = true;
        } else if (strBandwidthValue[x] == 't' || strBandwidthValue[x] == 'T') {
            nBandwidthValue *= (uint64) 1024 * 1024 * 1024 * 1024;
            fRequireLast = true;
        } else if (strBandwidthValue[x] == 'p' || strBandwidthValue[x] == 'P') {
            nBandwidthValue *= (uint64) 1024 * 1024 * 1024 * 1024 * 1024;
            fRequireLast = true;
        } else if (strBandwidthValue[x] == ' ') {
        } else {
            return (uint64) -1;
        }

        if (fRequireLast)
        {
            // Allow 'b' behind any of the non-'b' multipliers
            if (x == (strBandwidthValue.length() - 2) && (strBandwidthValue[x] != 'b' && strBandwidthValue[x] != 'B') && 
                                                         (strBandwidthValue[x + 1] == 'b' || strBandwidthValue[x + 1] =='B'))
                continue;

            // If it's not the last value, fail
            if (x != (strBandwidthValue.length() - 1))
                return (uint64) -1;
        }
    }
    return nBandwidthValue;
}
