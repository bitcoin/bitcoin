#ifndef BITCOIN_BANDWIDTHMAN_H
#define BITCOIN_BANDWIDTHMAN_H

#include <map>
#include <set>
#include <string>
#include <time.h>

#include "util.h"
#include "sync.h"

class CBandwidthMan
{
private:
    struct ResetInformation
    {
        std::set<char> setMinutes;
        std::set<char> setHours;
        std::set<char> setDaysOfMonth;
        std::set<char> setMonths;
        std::set<char> setDaysOfWeek;
    };

    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    ResetInformation riResetTimes;

    std::map<std::string, uint64> mapMaxCommandSentBytes;
    uint64 nMaxTotalSentBytes;

    // map to track bandwidth usage of each command
    std::map<std::string, uint64> mapCommandSentBytes;

    // uint64 to track total bandwidth usage
    uint64 nTotalSentBytes;

    // start time of usage tracking
    time_t nTrackingStartTime;

    // next reset time for usage tracking
    time_t nTrackingResetTime;

    void ParseResetTimes(std::string strResetTimes);
    
    void PrintResetTimes() const;
    void PrintCurrentBandwidth() const;
    
    void ResetTrackingStatistics();
    void RecalculateStartResetTimes(struct tm* ptmCurrentTime);

    bool DayIsFiltered(int nYearSince1900, char nMonth, char nDay);
    char LastDayOfMonth(int nYearSince1900, char nMonth);

    void SetPreviousMonth(struct tm& tmNewTrackingStartTime, char nStartMonth);
    void SetPreviousDay(struct tm& tmNewTrackingStartTime, const struct tm* ptmCurrentTime, char nStartDay);
    void SetPreviousHour(struct tm& tmNewTrackingStartTime, const struct tm* ptmCurrentTime, char nStartHour);
    void SetPreviousMinute(struct tm& tmNewTrackingStartTime, const struct tm* ptmCurrentTime, char nStartMinute);


    void SetNextYear(struct tm& tmNewTrackingResetTime, const struct tm* ptmCurrentTime);
    void SetNextMonth(struct tm& tmNewTrackingResetTime, const struct tm* ptmCurrentTime, int nTestYear, char nStartMonth);
    void SetNextDay(struct tm& tmNewTrackingResetTime, const struct tm* ptmCurrentTime, char nTestMonth, int nTestYear, char nStartDay);
    void SetNextHour(struct tm& tmNewTrackingResetTime, const struct tm* ptmCurrentTime, char nStartHour);
    void SetNextMinute(struct tm& tmNewTrackingResetTime, char nStartMinute);


    static void AddResetTimesRange(int nFrom, int nTo, int nSkip, std::set<char>* psetCurrentUnit, int nStartValue, int nModValue, int nOffsetValue);
    static uint64 ParseBandwidthValue(std::string strBandwidthValue);

public:

    CBandwidthMan();

    void LoadConfiguration();
    
    struct SchedulingTestData
    {
       ResetInformation riResetTimes;
       time_t nTrackingStartTime;
       time_t nTrackingResetTime;
    };

    // Function to test scheduling functionality
    SchedulingTestData TestScheduling(std::string strCronLine, struct tm tmTestTime);

    // Determine if we should be allowed to send command.
    //   Returns true if allowed to be sent, data asummed to be send if allowed by the function.
    bool AllowSendCommand(std::string strCommand, int nSize);

    IMPLEMENT_SERIALIZE
    (({
        // serialized format:
        // * version byte (currently 0)
        // * current statistics start time
        // * current statistics reset time
        // * total sent bytes
        // * mapCommandSentBytes
        //
        {
            LOCK(cs);
            unsigned char nVersion = 0;
            READWRITE(nVersion);
            READWRITE(nTrackingStartTime);
            READWRITE(nTrackingResetTime);
            READWRITE(nTotalSentBytes);
            READWRITE(mapCommandSentBytes);
        }
    });)
    
};

#endif
