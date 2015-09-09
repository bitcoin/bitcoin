#ifndef OMNICORE_ACTIVATION_H
#define OMNICORE_ACTIVATION_H

#include "omnicore/rules.h"

#include <stdint.h>
#include <string>
#include <vector>

namespace mastercore
{
/** Determines whether the sender is an authorized source for Omni Core activations. */
bool CheckActivationAuthorization(const std::string& sender);
/** Returns the vector of pending activations */
std::vector<FeatureActivation> GetPendingActivations();
/** Returns the vector of completed activations */
std::vector<FeatureActivation> GetCompletedActivations();
/** Checks if any activations went live in the block */
void CheckLiveActivations(int blockHeight);
/** Adds a pending activation */
void AddPendingActivation(uint16_t featureId, int activationBlock, uint32_t minClientVersion, const std::string& featureName);
}

#endif // OMNICORE_ACTIVATION_H





