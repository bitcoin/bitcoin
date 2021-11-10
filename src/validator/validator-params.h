//
// Created by James on 10/31/2021.
//

#ifndef RESERVE_VALIDATOR_PARAMS_H
#define RESERVE_VALIDATOR_PARAMS_H
class ValidatorParams
{
public:
    double stakeInterestRate; // Interest rate for adjusting a validator's stake
    int validatorTimeout; // Timeout for a Validator to validate a block before it is skipped
    int validatorSuspensionDuration; // Number of blocks a validator is suspended for
    int validatorWithdrawalWaitingPeriod; // Number of blocks a validator must wait before redeeming a withdrawn stake

public:
    ValidatorParams();
};

#endif //RESERVE_VALIDATOR_PARAMS_H
