#ifndef OMNICORE_NOTIFICATIONS_H
#define OMNICORE_NOTIFICATIONS_H

#include <stdint.h>
#include <string>
#include <vector>

//! Alert types
#define ALERT_FEATURE_UNSUPPORTED     0 // not allowed via alert broadcast
#define ALERT_BLOCK_EXPIRY            1
#define ALERT_BLOCKTIME_EXPIRY        2
#define ALERT_CLIENT_VERSION_EXPIRY   3

namespace mastercore
{
/** A structure for alert messages.
 */
struct AlertData
{
    //! Alert sender
    std::string alert_sender;
    //! Alert type
    uint16_t alert_type;
    //! Alert expiry
    uint32_t alert_expiry;
    //! Alert message
    std::string alert_message;
};

/** Determines whether the sender is an authorized source for Omni Core alerts. */
bool CheckAlertAuthorization(const std::string& sender);

/** Deletes previously broadcast alerts from the sender. */
void DeleteAlerts(const std::string& sender);
/** Removes all active alerts. */
void ClearAlerts();

/** Adds a new alert to the alerts vector. */
void AddAlert(const std::string& sender, uint16_t alertType, uint32_t alertExpiry, const std::string& alertMessage);

/** Alert string including meta data. */
std::vector<AlertData> GetOmniCoreAlerts();
/** Human readable alert messages. */
std::vector<std::string> GetOmniCoreAlertMessages();

/** Expires any alerts that need expiring. */
bool CheckExpiredAlerts(unsigned int curBlock, uint64_t curTime);
}

#endif // OMNICORE_NOTIFICATIONS_H
