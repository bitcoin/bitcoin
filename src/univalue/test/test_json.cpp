// Copyright 2014 BitPay Inc.
// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

// Test program that can be called by the JSON test suite at
// https://github.com/nst/JSONTestSuite.
//
// It reads JSON input from stdin and exits with code 0 if it can be parsed
// successfully. It also pretty prints the parsed JSON value to stdout.

#include <univalue.h>

#include <iostream>
#include <iterator>
#include <string>

using namespace std;

int main (int argc, char *argv[])
{
    UniValue val;
    if (val.read(string(istreambuf_iterator<char>(cin),
                        istreambuf_iterator<char>()))) {
        cout << val.write(1 /* prettyIndent */, 4 /* indentLevel */) << endl;
        return 0;
    } else {
        cerr << "JSON Parse Error." << endl;
        return 1;
    }
}
