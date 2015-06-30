#include "omnicore/notifications.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/version.h"

#include "main.h"
#include "util.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>
#include <string>
#include <vector>

namespace mastercore
{

//! Stores alert messages for Omni Core
static std::string global_alert_message;

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
 * Alert string including meta data.
 */
std::string GetOmniCoreAlertString()
{
    return global_alert_message;
}

/**
 * Human readable alert message.
 */
std::string GetOmniCoreAlertTextOnly()
{
    std::string message;
    std::vector<std::string> vstr;
    if (!global_alert_message.empty()) {
        boost::split(vstr, global_alert_message, boost::is_any_of(":"), boost::token_compress_on);
        // make sure there are 5 tokens, 5th is text message
        if (5 == vstr.size()) {
            message = vstr[4];
        } else {
            PrintToLog("DEBUG ALERT ERROR - Something went wrong decoding the global alert string, there are not 5 tokens\n");
        }
    }
    return message;
}

/**
 * Sets the alert string.
 */
void SetOmniCoreAlert(const std::string& alertMessage)
{
    global_alert_message = alertMessage;
}

/**
 * Expires any alerts that need expiring.
 */
bool CheckExpiredAlerts(unsigned int curBlock, uint64_t curTime)
{
    int32_t alertType = 0;
    uint64_t expiryValue = 0;
    uint32_t typeCheck = 0;
    uint32_t verCheck = 0;
    std::string alertMessage;
    if (!global_alert_message.empty()) {
        bool parseSuccess = ParseAlertMessage(global_alert_message, alertType, expiryValue, typeCheck, verCheck, alertMessage);
        if (parseSuccess) {
            switch (alertType) {
                case 1: //Text based alert only expiring by block number, show alert in UI and getalert_MP call, ignores type check value (eg use 0)
                    if (curBlock > expiryValue) {
                        //the alert has expired, clear the global alert string
                        PrintToLog("DEBUG ALERT - Expiring alert string %s\n", global_alert_message);
                        global_alert_message = "";
                        return true;
                    }
                    break;
                case 2: //Text based alert only expiring by block time, show alert in UI and getalert_MP call, ignores type check value (eg use 0)
                    if (curTime > expiryValue) {
                        //the alert has expired, clear the global alert string
                        PrintToLog("DEBUG ALERT - Expiring alert string %s\n", global_alert_message);
                        global_alert_message = "";
                        return true;
                    }
                    break;
                case 3: //Text based alert only expiring by client version, show alert in UI and getalert_MP call, ignores type check value (eg use 0)
                    if (OMNICORE_VERSION > expiryValue) {
                        //the alert has expired, clear the global alert string
                        PrintToLog("DEBUG ALERT - Expiring alert string %s\n", global_alert_message);
                        global_alert_message = "";
                        return true;
                    }
                    break;
                case 4: //Update alert, show upgrade alert in UI and getalert_MP call + use isTransactionTypeAllowed to verify client support and shutdown if not present
                    //check of the new tx type is supported at live block
                    bool txSupported = IsTransactionTypeAllowed(expiryValue + 1, OMNI_PROPERTY_MSC, typeCheck, verCheck);

                    //check if we are at/past the live blockheight                    
                    bool txLive = (curBlock > (int64_t) expiryValue);

                    //testnet allows all types of transactions, so override this here for testing
                    //txSupported = false; //testing
                    //txLive = true; //testing

                    if ((!txSupported) && (txLive)) {
                        // we know we have transactions live we don't understand
                        // can't be trusted to provide valid data, shutdown
                        PrintToLog("DEBUG ALERT - Shutting down due to unsupported live TX - alert string %s\n", global_alert_message);
                        PrintToConsole("DEBUG ALERT - Shutting down due to unsupported live TX - alert string %s\n", global_alert_message); // echo to screen
                        if (!GetBoolArg("-overrideforcedshutdown", false)) {
                            std::string msgText = "Shutting down due to alert: " + GetOmniCoreAlertTextOnly();
                            AbortNode(msgText, msgText); // show same message to user as that which is logged
                        }
                        return false;
                    }

                    if ((!txSupported) && (!txLive)) {
                        // warn the user this version does not support the new coming TX and they will need to update
                        // we don't actually need to take any action here, we leave the alert string in place and simply don't expire it
                    }

                    if (txSupported) {
                        // we can simply expire this update as this version supports the new coming TX
                        PrintToLog("DEBUG ALERT - Expiring alert string %s\n", global_alert_message);
                        global_alert_message = "";
                        return true;
                    }
                    break;
            }
        }
    }
    return false;
}

/**
 * Parses an alert string.
 */
bool ParseAlertMessage(const std::string& rawAlertStr, int32_t& alertType, uint64_t& expiryValue, uint32_t& typeCheck, uint32_t& verCheck, std::string& alertMessage)
{
    std::vector<std::string> vstr;
    boost::split(vstr, rawAlertStr, boost::is_any_of(":"), boost::token_compress_on);
    if (5 == vstr.size()) {
        try {
            alertType = boost::lexical_cast<int32_t>(vstr[0]);
            expiryValue = boost::lexical_cast<uint64_t>(vstr[1]);
            typeCheck = boost::lexical_cast<uint32_t>(vstr[2]);
            verCheck = boost::lexical_cast<uint32_t>(vstr[3]);
        } catch (const boost::bad_lexical_cast& e) {
            PrintToLog("DEBUG ALERT - error in converting values from global alert string: %s\n", e.what());
            return false;
        }
        alertMessage = vstr[4];
        if ((alertType < 1) || (alertType > 4) || (expiryValue == 0)) {
            PrintToLog("DEBUG ALERT ERROR - Something went wrong decoding the global alert string, values are not as expected.\n");
            return false;
        }
        return true;
    }
    return false;
}


} // namespace mastercore
