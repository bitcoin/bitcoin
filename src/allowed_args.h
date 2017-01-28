// Copyright (c) 2017 Stephen McCarthy
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ALLOWED_ARGS_H
#define BITCOIN_ALLOWED_ARGS_H

#include <functional>

typedef std::function<void(const std::string& strArg)> CheckArgFunc;

class AllowedArgs
{
public:

    static void BitcoinCli(const std::string& strArg);
    static void Bitcoind(const std::string& strArg);
    static void BitcoinQt(const std::string& strArg);
    static void BitcoinTx(const std::string& strArg);
    static void ConfigFile(const std::string& strArg);
};

#endif // BITCOIN_ALLOWED_ARGS_H
