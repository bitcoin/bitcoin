//
// Created by James on 10/31/2021.
//

#include "validator.h"
#include "../consensus/amount.h"
#include "../uint256.h"
#include "../script/script.h"
#include "validator-params.h"

Validator::Validator() {
    originalStake = 0;
    adjustedStake = 0;
    scriptPubKey = nullptr;
    lastBlockHeight = -1;
    probability = 0.0;
    suspended = false;
    suspendedBlock = 0;
}

void Validator::addStake(CAmount add) {
    this->stake += add;
}

void Validator::calculateProbability(int totalStake) {
    probability = (adjustedStake * 1.0) / totalStake;
}

void Validator::adjustStake(int nHeight, ValidatorParams* validatorParams) {
    int elapsedBlocks = nHeight - lastBlockHeight;
    adjustedStake = originalStake + originalStake * exp((1 + validatorParams->stakeInterestRate), elapsedBlocks);
}


