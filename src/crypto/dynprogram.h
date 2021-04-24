#pragma once
#include <vector>
#include <string>
#include <uint256.h>
#include <arith_uint256.h>
#include <crypto/common.h>
#include <crypto/ripemd160.h>
#include <crypto/sha256.h>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

class CDynProgram {

public:
    int32_t startingTime;
    std::vector<std::string> program;

    uint256 execute(unsigned char* blockHeader, uint256 prevBlockHash, uint256 merkleRoot);
};
