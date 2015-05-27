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

//! Stores alert messages for Omni Core
static std::string global_alert_message;

/**
 * Alert string including meta data.
 */
std::string mastercore::getMasterCoreAlertString()
{
    return global_alert_message;
}

/**
 * Human readable alert message.
 */
std::string mastercore::getMasterCoreAlertTextOnly()
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
void mastercore::setOmniCoreAlert(const std::string& alertMessage)
{
    global_alert_message = alertMessage;
}

/**
 * Expires any alerts that need expiring.
 */
bool mastercore::checkExpiredAlerts(unsigned int curBlock, uint64_t curTime)
{
    int32_t alertType = 0;
    uint64_t expiryValue = 0;
    uint32_t typeCheck = 0;
    uint32_t verCheck = 0;
    std::string alertMessage;
    if (!global_alert_message.empty()) {
        bool parseSuccess = parseAlertMessage(global_alert_message, &alertType, &expiryValue, &typeCheck, &verCheck, &alertMessage);
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
                    bool txSupported = isTransactionTypeAllowed(expiryValue + 1, OMNI_PROPERTY_MSC, typeCheck, verCheck);

                    // TODO: should be curBlock, not chainActive.Height()?
                    // TODO: should be curBlock, not chainActive.Height()?
                    // TODO: should be curBlock, not chainActive.Height()?

                    //check if we are at/past the live blockheight                    
                    bool txLive = (chainActive.Height()>(int64_t) expiryValue);

                    //testnet allows all types of transactions, so override this here for testing
                    //txSupported = false; //testing
                    //txLive = true; //testing

                    if ((!txSupported) && (txLive)) {
                        // we know we have transactions live we don't understand
                        // can't be trusted to provide valid data, shutdown
                        PrintToLog("DEBUG ALERT - Shutting down due to unsupported live TX - alert string %s\n", global_alert_message);
                        PrintToConsole("DEBUG ALERT - Shutting down due to unsupported live TX - alert string %s\n", global_alert_message); // echo to screen
                        if (!GetBoolArg("-overrideforcedshutdown", false)) {
                            std::string msgText = "Shutting down due to alert: " + getMasterCoreAlertTextOnly();
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
bool mastercore::parseAlertMessage(const std::string& rawAlertStr, int32_t* alertType, uint64_t* expiryValue, uint32_t* typeCheck, uint32_t* verCheck, std::string* alertMessage)
{
    std::vector<std::string> vstr;
    boost::split(vstr, rawAlertStr, boost::is_any_of(":"), boost::token_compress_on);
    if (5 == vstr.size()) {
        try {
            *alertType = boost::lexical_cast<int32_t>(vstr[0]);
            *expiryValue = boost::lexical_cast<uint64_t>(vstr[1]);
            *typeCheck = boost::lexical_cast<uint32_t>(vstr[2]);
            *verCheck = boost::lexical_cast<uint32_t>(vstr[3]);
        } catch (const boost::bad_lexical_cast& e) {
            PrintToLog("DEBUG ALERT - error in converting values from global alert string\n");
            return false;
        }
        *alertMessage = vstr[4];
        if ((*alertType < 1) || (*alertType > 4) || (*expiryValue == 0)) {
            PrintToLog("DEBUG ALERT ERROR - Something went wrong decoding the global alert string, values are not as expected.\n");
            return false;
        }
        return true;
    }
    return false;
}

