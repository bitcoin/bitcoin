#pragma once
#include <vector>
#include "crypto/uint256_t.h"




class CDynVM
{
    std::vector<uint256_t> stack;
    std::vector<uint256_t> registers;
    std::vector<unsigned char> ram;


public:

    void exec();
};
