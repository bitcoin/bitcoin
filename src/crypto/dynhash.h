#pragma once
#include <map>
//

#include "dynprogram.h"



class CDynHash {

public:
    std::vector<CDynProgram*> programs;

    void load(std::string program);
    std::string calcBlockHeaderHash(uint32_t blockTime, unsigned char* blockHeader, std::string prevBlockHash, std::string merkleRoot);


};
