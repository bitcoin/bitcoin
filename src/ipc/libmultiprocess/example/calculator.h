// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EXAMPLE_CALCULATOR_H
#define EXAMPLE_CALCULATOR_H

#include <string>

class Calculator
{
public:
    virtual ~Calculator() = default;
    virtual void solveEquation(const std::string& eqn) = 0;
};

#endif // EXAMPLE_CALCULATOR_H
