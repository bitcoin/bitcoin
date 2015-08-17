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
 * Deletes previously broadcast alerts from sender from the alerts vector
 *
 * Note cannot be used to delete alerts from other addresses, nor to delete system generated feature alerts
 */
void DeleteAlerts(const std::string& sender)
{
    for (std::vector<AlertData>::iterator it = currentOmniAlerts.begin(); it != currentOmniAlerts.end(); ) {
        AlertData alert = *it;
        if (sender == alert.alert_sender) {
            PrintToLog("Removing deleted alert (from:%s type:%d expiry:%d message:%s)\n", alert.alert_sender,
                alert.alert_type, alert.alert_expiry, alert.alert_message);
            it = currentOmniAlerts.erase(it);
            uiInterface.OmniStateChanged();
        } else {
            it++;
        }
    }
}

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

    // very basic sanity checks for broadcast alerts to catch malformed packets
    if (sender != "omnicore" && (alertType < ALERT_BLOCK_EXPIRY || alertType > ALERT_CLIENT_VERSION_EXPIRY)) {
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
            case ALERT_FEATURE_UNSUPPORTED:
                if (curBlock >= alert.alert_expiry) {
                    std::string msgText = strprintf("Shutting down due to alert: %s", alert.alert_message);
                    PrintToLog(msgText);
                    PrintToConsole(msgText);
                    if (!GetBoolArg("-overrideforcedshutdown", false)) {
                        AbortNode(msgText, msgText);
                    }
                    it = currentOmniAlerts.erase(it);
                } else {
                    it++;
                }
            break;
            case ALERT_BLOCK_EXPIRY:
                if (curBlock >= alert.alert_expiry) {
                    PrintToLog("Expiring alert (from %s: type:%d expiry:%d message:%s)\n", alert.alert_sender,
                        alert.alert_type, alert.alert_expiry, alert.alert_message);
                    it = currentOmniAlerts.erase(it);

                    // if expiring a feature activation because it's gone live, add another notification
                    // to say we're now live and expire it in 1024 blocks.
                    size_t replacePosition = alert.alert_message.find("') will go live at block ");
                    if (replacePosition != std::string::npos) {
                        std::string newAlertMessage = alert.alert_message.substr(0, replacePosition) + "') is now live.";
                        AddAlert("omnicore", 1, alert.alert_expiry + 1024, newAlertMessage);
                    }

                    uiInterface.OmniStateChanged();
                } else {
                    it++;
                }
            break;
            case ALERT_BLOCKTIME_EXPIRY:
                if (curTime > alert.alert_expiry) {
                    PrintToLog("Expiring alert (from %s: type:%d expiry:%d message:%s)\n", alert.alert_sender,
                        alert.alert_type, alert.alert_expiry, alert.alert_message);
                    it = currentOmniAlerts.erase(it);
                    uiInterface.OmniStateChanged();
                } else {
                    it++;
                }
            break;
            case ALERT_CLIENT_VERSION_EXPIRY:
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
