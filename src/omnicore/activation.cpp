/**
 * @file activation.cpp
 *
 * This file contains feature activation utility code.
 *
 * Note main function 'ActivateFeature()' is consensus breaking and resides in rules.cpp
 */

#include "omnicore/activation.h"
#include "omnicore/log.h"
#include "omnicore/version.h"

#include "main.h"
#include "ui_interface.h"

#include <stdint.h>
#include <string>
#include <vector>

/*
#include "omnicore/omnicore.h"
#include "omnicore/notifications.h"
#include "omnicore/utilsbitcoin.h"

#include "alert.h"
#include "chainparams.h"
#include "script/standard.h"
#include "uint256.h"

#include <openssl/sha.h>

#include <limits>
*/

namespace mastercore
{

/**
 * Pending activations
 */
std::vector<FeatureActivation> vecPendingActivations;

/**
 * Completed activations
 */
std::vector<FeatureActivation> vecCompletedActivations;

/**
 * Adds a feature activation to the PendingActivations vector.
 *
 */
void AddPendingActivation(uint16_t featureId, int activationBlock, uint32_t minClientVersion, const std::string& featureName)
{
    FeatureActivation featureActivation;

    featureActivation.featureId = featureId;
    featureActivation.featureName = featureName;
    featureActivation.activationBlock = activationBlock;
    featureActivation.minClientVersion = minClientVersion;

    vecPendingActivations.push_back(featureActivation);
}

/**
 * Deletes a pending activation from the PendingActivations vector.  Deletion is not supported on the CompletedActivations vector.
 *
 */
void DeletePendingActivation(uint16_t featureId)
{
   for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ) {
       if ((*it).featureId == featureId) {
           it = vecPendingActivations.erase(it);
       } else {
           it++;
       }
   }
}

/**
 * Checks if any activations went live in the block
 *
 */
void CheckLiveActivations(int blockHeight)
{
    std::vector<FeatureActivation> vecPendingActivations = GetPendingActivations();
    for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ++it) {
       if ((*it).activationBlock > blockHeight) continue;
       FeatureActivation liveActivation = *it;
       if (OMNICORE_VERSION < liveActivation.minClientVersion) {
           std::string msgText = strprintf("Shutting down due to unsupported feature activation (%d: %s)", liveActivation.featureId, liveActivation.featureName);
           PrintToLog(msgText);
           PrintToConsole(msgText);
           if (!GetBoolArg("-overrideforcedshutdown", false)) {
               AbortNode(msgText, msgText);
           }
       }
       PendingActivationCompleted(liveActivation.featureId);
       uiInterface.OmniStateChanged();
    }
}

/**
 * Moves an activation from PendingActivations to CompletedActivations when it is activated
 *
 */
void PendingActivationCompleted(uint16_t featureId)
{
   for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ) {
       if ((*it).featureId == featureId) {
           FeatureActivation completedActivation = *it;
           vecCompletedActivations.push_back(completedActivation);
           it = vecPendingActivations.erase(it);
       } else {
           it++;
       }
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

    // Mainnet
    whitelisted.insert("1OmniFoundation1111111111111111111"); // Omni Foundation <????@omni.foundation>

    // Testnet / Regtest
    // use -omniactivationallowsender for testing

    // Add manually whitelisted sources
    if (mapArgs.count("-omniactivationallowsender")) {
        const std::vector<std::string>& sources = mapMultiArgs["-omniactivationallowsender"];

        for (std::vector<std::string>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
            whitelisted.insert(*it);
        }
    }

    // Remove manually ignored sources
    if (mapArgs.count("-omniactivationignoresender")) {
        const std::vector<std::string>& sources = mapMultiArgs["-omniactivationignoresender"];

        for (std::vector<std::string>::const_iterator it = sources.begin(); it != sources.end(); ++it) {
            whitelisted.erase(*it);
        }
    }

    bool fAuthorized = (whitelisted.count(sender) ||
                        whitelisted.count("any"));

    return fAuthorized;
}

}
