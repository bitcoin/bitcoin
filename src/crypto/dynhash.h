#pragma once
#include <map>
#include <fs.h>
#include <util/system.h>
#include <streams.h>
#include <clientversion.h>

#include "dynprogram.h"
#include <uint256.h>


class CDynHash {

public:
    std::vector<CDynProgram*> programs;

    void load(std::string program);
    uint256 calcBlockHeaderHash(uint32_t blockTime, unsigned char* blockHeader, uint256 prevBlockHash, uint256 merkleRoot);

};
