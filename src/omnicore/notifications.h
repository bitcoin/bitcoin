#ifndef OMNICORE_NOTIFICATIONS_H
#define OMNICORE_NOTIFICATIONS_H

#include <stdint.h>
#include <string>

namespace mastercore
{
/** Determines whether the sender is an authorized source for Omni Core alerts. */
bool CheckAlertAuthorization(const std::string& sender);

/** Alert string including meta data. */
std::string GetOmniCoreAlertString();

/** Human readable alert message. */
std::string GetOmniCoreAlertTextOnly();

/** Sets the alert string. */
void SetOmniCoreAlert(const std::string& alertMessage);

/** Expires any alerts that need expiring. */
bool CheckExpiredAlerts(unsigned int curBlock, uint64_t curTime);

/** Parses an alert string. */
bool ParseAlertMessage(const std::string& rawAlertStr, int32_t& alertType, uint64_t& expiryValue, uint32_t& typeCheck, uint32_t& verCheck, std::string& alertMessage);
}

#endif // OMNICORE_NOTIFICATIONS_H
