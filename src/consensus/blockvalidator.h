//
// Created by nicolasdorier on 16/07/13.
//

#ifndef BLOCKVALIDATOR_H
#define BLOCKVALIDATOR_H

#include "uint256.h"

class CConsensusFlags
{
public:
    unsigned int scriptFlags = 0;
    bool enforceBIP30 = false;
    bool enforceBIP34 = false;
    unsigned int locktimeFlags = 0;
};

#endif //BLOCKVALIDATOR_H
