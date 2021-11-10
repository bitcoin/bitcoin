//
// Created by James on 10/31/2021.
//

#include "validator-pool.h"
#include "validator.h"
#include "validator-params.h"
#include "../consensus/amount.h"
#include "../chain.h"
#include <cmath>
#include "../crypto/sha256.h"
#include "../script/script.h"
#include "../boost/crc.hpp"
#include <float.h>
#include <vector>
#include <unordered_map>
#include "../script/script.h"

ValidatorPool::ValidatorPool(ValidatorParams* vp) {
    this->validatorParams = vp;
    totalStake = 0;
}

bool ValidatorPool::addValidator(Validator* validator, int nHeight, std::vector<std::string> rterror) {
    if (validatorExists(validator)) {
        rterror.push_back("Validator exists");
        return false;
    }

    validatorPool.push_back(validator);
    validatorPoolScript[validator->scriptPubKey.ToString()] = validator;
    return recalculateProbabilities(nHeight, rterror);
}

bool ValidatorPool::removeValidator(Validator* validator, int nHeight, std::vector<std::string> rterror) {
    if (validator->suspended) {
        rterror.push_back("Validator suspended");
        return false;
    }

    if (!validatorExists(validator)) {
        rterror.push_back("Validator does not exist");
        return false;
    }

    auto it = validatorPoolScript.find(validator->scriptPubKey.ToString());
    for (size_t i = 0; i < validatorPool.size(); i++) {
        if (it.second == validatorPool.at(i)) {
            validatorPool.erase(validatorPool.begin()+i);
            break;
        }
    }
    validatorPoolScript.erase(it);
    return recalculateProbabilities(nHeight, rterror);
}

bool ValidatorPool::recalculateProbabilities(int nHeight, std::vector<std::string> rterror) {
    totalStake = 0;
    for (Validator& v : validatorPool) {
        if (v.suspended) {
            int elapsedTime = nHeight - v.suspendedBlock;
            if (elapsedTime >= validatorParams->validatorSuspensionDuration) {
                unsuspendValidator(v, nHeight, rterror);

            }
        }

        if (!v.suspended){
            v.adjustedStake(nHeight, validatorParams);
            totalStake += v.adjustedStake;
        }
    }

    for (Validator& v : validatorPool) {
        v.calculateProbability(totalStake);
    }
    sort(nHeight);
    return true;
}

Validator* ValidatorPool::retrieveNextValidator(std::vector<std::string> rterror) {
    vData.empty();
    serialize();

    boost::crc_32_type hasher;
    hasher.process_bytes(vData.begin(), vData.begin() + vData.size());
    double result = abs((double) hasher.checksum());

    double selection = result / DBL_MAX;
    Validator* selectedValidator = nullptr;

    for (Validator& v : ValidatorPool) {
        if (selection >= v.probability) {
            selectedValidator = &v;
            break;
        }
    }

    if (!v) {
        rterror.push_back("No validator found");
        return nullptr;
    }

    return v;
}

void ValidatorPool::serialize() {
    for (Validator& v : ValidatorPool) {
        std::vector<unsigned char> s = ToByteVector(v->scriptPubKey);
        vData.insert(vData.back(), s.front(), s.back());
    }
}

void ValidatorPool::sort(int nHeight) {
    std::sort(validatorPool.begin(), validatorPool.end(), std::reverse());
}

bool ValidatorPool::validatorExists(Validator* v) {
    return validatorPoolScript.count(v->scriptPubKey.ToString()) == 1;
}

bool ValidatorPool::suspendValidator(Validator *v, int nHeight, std::vector <std::string> rterror) {
    if(!validatorExists(v)) {
        rterror.push_back("Validator does not exist");
        return false;
    }

    v->suspended = true;
    v->suspendedBlock = nHeight;
    v->adjustedStake = 0;
    recalculateProbabilities(nHeight, rterror);
}

bool ValidatorPool::unsuspendValidator(Validator *v, int nHeight, std::vector <std::string> rterror) {
    v->suspended = false;
    v->suspendedBlock = 0;
    v->lastBlockHeight = nHeight;
    return true;
}