// Copyright (c) 2017 Block Mechanic 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef INTERBLOCKCHAIN_H
#define INTERBLOCKCHAIN_H

#include <netbase.h>
#include <serialize.h>
#include <uint256.h>
#include <version.h>
#include <util/system.h>
#include <map>

struct SChain
{
public:
    std::string sChainName;
    std::string sCurrencyCode;
    unsigned char pchMessageOne;
    unsigned char pchMessageTwo;
    unsigned char pchMessageThree;
    unsigned char pchMessageFour;
    int dport;

    SChain()
    {

    }

    SChain(std::string sName, std::string sCode, unsigned char cOne, unsigned char cTwo, unsigned char cThree, unsigned char cFour, int port)
    {
        sChainName = sName;
        sCurrencyCode = sCode;
        pchMessageOne = cOne;
        pchMessageTwo = cTwo;
        pchMessageThree = cThree;
        pchMessageFour = cFour;
        dport = port;
    }
};

class CIbtp
{
public:
    //std::map<std::string, std::vector<unsigned char[4]> > mapBlockchainMessageStart;
    std::vector<SChain> vChains;

    CIbtp()
    {
        LoadMsgStart();
    }

    void LoadMsgStart();
    bool IsIbtpChain(const unsigned char msgStart[], std::string& chainName);
};

#endif // INTERBLOCKCHAIN_H
