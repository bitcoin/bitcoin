#include "omnicore/notifications.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/rules.h"
#include "omnicore/utilsbitcoin.h"
#include "omnicore/version.h"

#include "main.h"
#include "util.h"
#include "ui_interface.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>
#include <string>
#include <vector>

namespace mastercore
{

//! Vector of currently active Omni alerts
std::vector<AlertData> currentOmniAlerts;

/**
 * Adds a new alert to the alerts vector
 *
 */
void AddAlert(const std::string& sender, uint16_t alertType, uint32_t alertExpiry, const std::string& alertMessage)
{
    AlertData newAlert;
    newAlert.alert_sender = sender;
    newAlert.alert_type = alertType;
    newAlert.alert_expiry = alertExpiry;
    newAlert.alert_message = alertMessage;

    // very basic sanity checks to catch malformed packets
    if (alertType < 1 || alertType > 3) {
        PrintToLog("New alert REJECTED (alert type not recognized): %s, %d, %d, %s\n", sender, alertType, alertExpiry, alertMessage);
        return;
    }

    currentOmniAlerts.push_back(newAlert);
    PrintToLog("New alert added: %s, %d, %d, %s\n", sender, alertType, alertExpiry, alertMessage);
}

/**
 * Determines whether the sender is an authorized source for Omni Core alerts.
 *
 * The option "-omnialertallowsender=source" can be used to whitelist additional sources,
 * and the option "-omnialertignoresender=source" can be used to ignore a source.
 *
 * To consider any alert as authorized, "-omnialertallowsender=any" can be used. This
 * should only be done for testing purposes!
 */
bool CheckAlertAuthorization(const std::string& sender)
{
    std::set<std::string> whitelisted;

    // TODO: check email addresses

    // Mainnet
    whitelisted.insert("16Zwbujf1h3v1DotKcn9XXt1m7FZn2o4mj"); // Craig   <craig@omni.foundation>
    whitelisted.insert("1MicH2Vu4YVSvREvxW1zAx2XKo2GQomeXY"); // Michael <michael@omni.foundation>
    whitelisted.insert("1zAtHRASgdHvZDfHs6xJquMghga4eG7gy");  // Zathras <zathras@omni.foundation>
    whitelisted.insert("1dexX7zmPen1yBz2H9ZF62AK5TGGqGTZH");  // dexX7   <dexx@bitwatch.co>
    whitelisted.insert("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"); // Exodus (who has access?)

    // Testnet
    whitelisted.insert("mpDex4kSX4iscrmiEQ8fBiPoyeTH55z23j"); // Michael <michael@omni.foundation>
    whitelisted.insert("mpZATHupfCLqet5N1YL48ByCM1ZBfddbGJ"); // Zathras <zathras@omni.foundation>
    whitelisted.insert("mk5SSx4kdexENHzLxk9FLhQdbbBexHUFTW"); // dexX7   <dexx@bitwatch.co>

    // Add manually whitelisted sources
    if (mapArgs.count("-omnialertallowsender")) {
        const std::vector<std::string>& sources = mapMultiArgs["-omnialertallowsender"];

        for (std::vector<std::string>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
            whitelisted.insert(*it);
        }
    }

    // Remove manually ignored sources
    if (mapArgs.count("-omnialertignoresender")) {
        const std::vector<std::string>& sources = mapMultiArgs["-omnialertignoresender"];

        for (std::vector<std::string>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
            whitelisted.erase(*it);
        }
    }

    bool fAuthorized = (whitelisted.count(sender) ||
                        whitelisted.count("any"));

    return fAuthorized;
}

/**
 * Alerts including meta data.
 */
std::vector<AlertData> GetOmniCoreAlerts()
{
    return currentOmniAlerts;
}

/**
 * Human readable alert messages.
 */
std::vector<std::string> GetOmniCoreAlertMessages()
{
    std::vector<std::string> vstr;
    for (std::vector<AlertData>::iterator it = currentOmniAlerts.begin(); it != currentOmniAlerts.end(); it++) {
        vstr.push_back((*it).alert_message);
    }
    return vstr;
}

/**
 * Expires any alerts that need expiring.
 */
bool CheckExpiredAlerts(unsigned int curBlock, uint64_t curTime)
{
    for (std::vector<AlertData>::iterator it = currentOmniAlerts.begin(); it != currentOmniAlerts.end(); ) {
        AlertData alert = *it;
        switch (alert.alert_type) {
            case 1: // alert expiring by block number
                if (curBlock >= alert.alert_expiry) {
                    PrintToLog("Expiring alert (from %s: type:%d expiry:%d message:%s)\n", alert.alert_sender,
                        alert.alert_type, alert.alert_expiry, alert.alert_message);
                    it = currentOmniAlerts.erase(it);
                    uiInterface.OmniStateChanged();
                } else {
                    it++;
                }
            break;
            case 2: // alert expiring by block time
                if (curTime > alert.alert_expiry) {
                    PrintToLog("Expiring alert (from %s: type:%d expiry:%d message:%s)\n", alert.alert_sender,
                        alert.alert_type, alert.alert_expiry, alert.alert_message);
                    it = currentOmniAlerts.erase(it);
                    uiInterface.OmniStateChanged();
                } else {
                    it++;
                }
            break;
            case 3: // alert expiring by client version
                if (OMNICORE_VERSION > alert.alert_expiry) {
                    PrintToLog("Expiring alert (form: %s type:%d expiry:%d message:%s)\n", alert.alert_sender,
                        alert.alert_type, alert.alert_expiry, alert.alert_message);
                    it = currentOmniAlerts.erase(it);
                    uiInterface.OmniStateChanged();
                } else {
                    it++;
                }
            break;
            default: // unrecognized alert type
                    PrintToLog("Removing invalid alert (from:%s type:%d expiry:%d message:%s)\n", alert.alert_sender,
                        alert.alert_type, alert.alert_expiry, alert.alert_message);
                    it = currentOmniAlerts.erase(it);
                    uiInterface.OmniStateChanged();
            break;
        }
    }
    return true;
}

} // namespace mastercore
