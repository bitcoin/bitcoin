//
// Created by James on 10/31/2021.
//
#include "validator.h"
#include "validator-params.h"
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include "../consensus/amount.h"
#include <string>
#include "../script/script.h"

#ifndef RESERVE_POOL_H
#define RESERVE_POOL_H

class ValidatorPool {
private:
    ValidatorParams* validatorParams;
    CAmount totalStake;
    std::vector<Validator> validatorPool;
    std::unordered_map<std::string , Validator> validatorPoolScript;
    std::vector<uint8_t> vData;

    void sort(int nHeight);

public:
    ValidatorPool() delete;
    ValidatorPool(ValidatorParams* vp);
    bool addValidator(Validator* v, int nHeight, std::vector<std::string> rterror);
    bool removeValidator(Validator* v, int nHeight, std::vector<std::string> rterror);
    bool recalculateProbabilities(int nHeight, std::vector<std::string> rterror);
    Validator* retrieveNextValidator(std::vector<std::string> rterror);
    void serialize();
    bool validatorExists(Validator* v);
    bool suspendValidator(Validator* v, int nHeight, std::vector<std::string> rterror);
    bool unsuspendValidator(Validator* v, int nHeight, std::vector<std::string> rterror);
};
#endif //RESERVE_POOL_H
