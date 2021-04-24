#pragma once
#include <vector>
#include <string>
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

    const std::vector<char> hexDigit = {'0', '1', '2', '3', '4','5','6','7','8','9','A','B','C','D','E','F'};

    std::string execute(unsigned char* blockHeader, std::string prevBlockHash, std::string merkleRoot);
    std::string getProgramString();
    void parseHex(std::string input, unsigned char* output);
    unsigned char decodeHex(char in);
    std::string makeHex(unsigned char* in, int len);
};
