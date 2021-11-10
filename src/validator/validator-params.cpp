//
// Created by James on 11/1/2021.
//

#include "validator-params.h"
#include <stdint.h>

ValidatorParams::ValidatorParams() {
    stakeInterestRate = 0.02;

    int len = sizeof(maxInt)/sizeof(maxInt[0]);

    for (int i = 0; i < len; i++) {
        maxInt[i] = UINT32_MAX;
    }
}