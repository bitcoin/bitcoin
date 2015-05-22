// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEMPLATES_H
#define BITCOIN_TEMPLATES_H

#include <assert.h>

template<typename T>
class Container
{
    T* pObj;
public:
    Container() : pObj(NULL) { };

    bool IsNull()
    {
        return pObj == NULL;
    }
    void Set(T* pObjIn)
    {
        if (pObj)
            delete(pObj);
        pObj = pObjIn;
    }
    const T& Get()
    {
        assert(pObj);
        return *pObj;
    }

    ~Container()
    {
        if (pObj)
            delete(pObj);
    }
};

#endif // BITCOIN_TEMPLATES_H
