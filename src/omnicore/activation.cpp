/**
 * @file activation.cpp
 *
 * This file contains feature activation utility code.
 *
 * Note main functions 'ActivateFeature()' and 'DeactivateFeature()' are consensus breaking and reside in rules.cpp
 */

#include <omnicore/activation.h>

#include <omnicore/log.h>
#include <omnicore/version.h>

#include <validation.h>
#include <ui_interface.h>

#include <boost/filesystem.hpp>

#include <stdint.h>
#include <string>
#include <vector>

namespace mastercore
{
//! Pending activations
std::vector<FeatureActivation> vecPendingActivations;
//! Completed activations
std::vector<FeatureActivation> vecCompletedActivations;

/**
 * Deletes pending activations with the given identifier.
 *
 * Deletion is not supported on the CompletedActivations vector.
 */
static void DeletePendingActivation(uint16_t featureId)
{
   for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ) {
       if ((*it).featureId == featureId) {
           it = vecPendingActivations.erase(it);
       } else {
           ++it;
       }
   }
}

/**
 * Moves an activation from PendingActivations to CompletedActivations when it is activated.
 *
 * A signal is fired to notify the UI about the status update.
 */
static void PendingActivationCompleted(const FeatureActivation& activation)
{
    DeletePendingActivation(activation.featureId);
    vecCompletedActivations.push_back(activation);
    uiInterface.OmniStateChanged();
}

/**
 * Adds a feature activation to the PendingActivations vector.
 *
 * If this feature was previously scheduled for activation, then the state pending objects are deleted.
 */
void AddPendingActivation(uint16_t featureId, int activationBlock, uint32_t minClientVersion, const std::string& featureName)
{
    DeletePendingActivation(featureId);

    FeatureActivation featureActivation;
    featureActivation.featureId = featureId;
    featureActivation.featureName = featureName;
    featureActivation.activationBlock = activationBlock;
    featureActivation.minClientVersion = minClientVersion;

    vecPendingActivations.push_back(featureActivation);

    uiInterface.OmniStateChanged();
}

/**
 * Checks if any activations went live in the block.
 */
void CheckLiveActivations(int blockHeight)
{
    std::vector<FeatureActivation> vecPendingActivations = GetPendingActivations();
    for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ++it) {
        const FeatureActivation& liveActivation = *it;
        if (liveActivation.activationBlock > blockHeight) {
            continue;
        }
        if (OMNICORE_VERSION < liveActivation.minClientVersion) {
            std::string msgText = strprintf("Shutting down due to unsupported feature activation (%d: %s)", liveActivation.featureId, liveActivation.featureName);
            PrintToLog(msgText);
            PrintToConsole(msgText);
            if (!gArgs.GetBoolArg("-overrideforcedshutdown", false)) {
                boost::filesystem::path persistPath = GetDataDir() / "MP_persist";
                if (boost::filesystem::exists(persistPath)) boost::filesystem::remove_all(persistPath); // prevent the node being restarted without a reparse after forced shutdown
                DoAbortNode(msgText, msgText);
            }
        }
        PendingActivationCompleted(liveActivation);
    }
}


/**
 * Returns the vector of pending activations.
 */
std::vector<FeatureActivation> GetPendingActivations()
{
    return vecPendingActivations;
}

/**
 * Returns the vector of completed activations.
 */
std::vector<FeatureActivation> GetCompletedActivations()
{
    return vecCompletedActivations;
}

/**
 * Removes all pending or completed activations.
 *
 * A signal is fired to notify the UI about the status update.
 */
void ClearActivations()
{
    vecPendingActivations.clear();
    vecCompletedActivations.clear();
    uiInterface.OmniStateChanged();
}

/**
 * Determines whether the sender is an authorized source for Omni Core feature activation.
 *
 * The option "-omniactivationallowsender=source" can be used to whitelist additional sources,
 * and the option "-omniactivationignoresender=source" can be used to ignore a source.
 *
 * To consider any activation as authorized, "-omniactivationallowsender=any" can be used. This
 * should only be done for testing purposes!
 */
bool CheckActivationAuthorization(const std::string& sender)
{
    std::set<std::string> whitelisted;

    // Mainnet - 4 out of 5 signatures required from developers & board members
    /*
    "address": "3Fc5gWzEQh1YGeqVXH6E4GDEGgbZJREJQ3",
    "scriptPubKey": "a91498a2d17e08ac677dc220b92e0b79406f2f441c2487",
    "sigsrequired": 4,
    "addresses": [
      "1883ZMsRJfzKNozUBJBTCxQ7EaiNioNDWz", // Zathras - zathras@omni.foundation - Project maintainer, developer
      "1HHv91gRxqBzQ3gydMob3LU8hqXcWoLfvd", // dexx - dexx@bitwatch.co - Project maintainer, developer
      "1oyvGmABkeFRUECn2t8DEZPes6F7Gsc9T", // J.R. Willett - jr@omni.foundation - Founder and board member
      "17xr7sbehYY4YSZX9yuJe6gK9rrdRrZx26", // Craig Sellars - craig@omni.foundation - Technologist and board member
      "16oDZYCspsczfgKXVj3xyvsxH21NpEj94F" // Adam Chamely - adam@omni.foundation - Project maintainer, developer
    ],
    */
    whitelisted.insert("3Fc5gWzEQh1YGeqVXH6E4GDEGgbZJREJQ3");

    // Alternatively: 3 out of 3 signatures required
    /*
    {
      "address": "33jx18123xJS59hnHwCgsr6zvYPfiCussh",
      "scriptPubKey": "a914167e39ef2efa6612405c9175c5a37a2af1c8cef287",
      "hex": "5321036a4caa95ec1d55f1b75a8b6c7345f22b4efc9e25d38ab058ef7d6f60b3b744f74104b7a3d7f7ccdf211dfd180815b87332b4773cc40bff72a4d0bb60f3a85409d19f99709331c6b11c976fe274a86d789a1cf2b3b0be29fe5fc55c93ad9e08459c4f4104e65b098558d637cfcf3194214637f8838338b141259b698d2a027b069d405b6502ad4a4e9aa75094fa431a6c9af580f5917834a6d4cec946054df33194b2967853ae",
      "addresses": [
        "1HHv91gRxqBzQ3gydMob3LU8hqXcWoLfvd", // dexx - dexx@bitwatch.co - Project maintainer, developer
        "17xr7sbehYY4YSZX9yuJe6gK9rrdRrZx26", // Craig Sellars - craig@omni.foundation - Technologist and board member
        "16oDZYCspsczfgKXVj3xyvsxH21NpEj94F"  // Adam Chamely - adam@omni.foundation - Project maintainer, developer
      ],
      "sigsrequired": 3,
    }
    */
    whitelisted.insert("33jx18123xJS59hnHwCgsr6zvYPfiCussh");

    // Testnet / Regtest
    // use -omniactivationallowsender for testing

    // Add manually whitelisted sources
    if (gArgs.IsArgSet("-omniactivationallowsender")) {
        const std::vector<std::string>& sources = gArgs.GetArgs("-omniactivationallowsender");

        for (std::vector<std::string>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
            whitelisted.insert(*it);
        }
    }

    // Remove manually ignored sources
    if (gArgs.IsArgSet("-omniactivationignoresender")) {
        const std::vector<std::string>& sources = gArgs.GetArgs("-omniactivationignoresender");

        for (std::vector<std::string>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
            whitelisted.erase(*it);
        }
    }

    bool fAuthorized = (whitelisted.count(sender) ||
                        whitelisted.count("any"));

    return fAuthorized;
}

/**
 * Determines whether the sender is an authorized source to deactivate features.
 *
 * The custom options "-omniactivationallowsender=source" and "-omniactivationignoresender=source" are also applied to deactivations.
 */
bool CheckDeactivationAuthorization(const std::string& sender)
{
    std::set<std::string> whitelisted;

    // Mainnet - 3 out of 5 signatures required from developers & board members
    /*
    "address" : "34kwkVRSvFVEoUwcQSgpQ4ZUasuZ54DJLD",
    "script" : "multisig",
    "sigsrequired" : 3,
    "addresses" : [
      "1883ZMsRJfzKNozUBJBTCxQ7EaiNioNDWz", // Zathras - zathras@omni.foundation - Project maintainer, developer
      "1HHv91gRxqBzQ3gydMob3LU8hqXcWoLfvd", // dexx - dexx@bitwatch.co - Project maintainer, developer
      "1oyvGmABkeFRUECn2t8DEZPes6F7Gsc9T", // J.R. Willett - jr@omni.foundation - Founder and Board Member
      "17xr7sbehYY4YSZX9yuJe6gK9rrdRrZx26", // Craig Sellars - craig@omni.foundation - Technologist and Board Member
      "16oDZYCspsczfgKXVj3xyvsxH21NpEj94F" // Adam Chamely - adam@omni.foundation - Project maintainer, developer
    ],
    */
    whitelisted.insert("34kwkVRSvFVEoUwcQSgpQ4ZUasuZ54DJLD");

    // Testnet / Regtest
    // use -omniactivationallowsender for testing

    // Add manually whitelisted sources - custom sources affect both activation and deactivation
    if (gArgs.IsArgSet("-omniactivationallowsender")) {
        const std::vector<std::string>& sources = gArgs.GetArgs("-omniactivationallowsender");

        for (std::vector<std::string>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
            whitelisted.insert(*it);
        }
    }

    // Remove manually ignored sources - custom sources affect both activation and deactivation
    if (gArgs.IsArgSet("-omniactivationignoresender")) {
        const std::vector<std::string>& sources = gArgs.GetArgs("-omniactivationignoresender");

        for (std::vector<std::string>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
            whitelisted.erase(*it);
        }
    }

    bool fAuthorized = (whitelisted.count(sender) ||
                        whitelisted.count("any"));

    return fAuthorized;
}

} // namespace mastercore
