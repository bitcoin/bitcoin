#ifndef OMNICORE_NOTIFICATIONS_H
#define OMNICORE_NOTIFICATIONS_H

#include <stdint.h>
#include <string>

namespace mastercore
{
/** Alert string including meta data. */
std::string getMasterCoreAlertString();

/** Human readable alert message. */
std::string getMasterCoreAlertTextOnly();

/** Sets the alert string. */
void setOmniCoreAlert(const std::string& alertMessage);

/** Expires any alerts that need expiring. */
bool checkExpiredAlerts(unsigned int curBlock, uint64_t curTime);

/** Parses an alert string. */
bool parseAlertMessage(const std::string& rawAlertStr, int32_t* alertType, uint64_t* expiryValue, uint32_t* typeCheck, uint32_t* verCheck, std::string* alertMessage);
}

#endif // OMNICORE_NOTIFICATIONS_H
