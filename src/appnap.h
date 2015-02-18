// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_APPNAPINHIBITOR_H
#define BITCOIN_APPNAPINHIBITOR_H

class CAppNapInhibitorInt;

class CAppNapInhibitor
{
public:
    CAppNapInhibitor(const char* strReason);
    ~CAppNapInhibitor();
private:
    CAppNapInhibitorInt *priv;
};

#endif // BITCOIN_APPNAPINHIBITOR_H
