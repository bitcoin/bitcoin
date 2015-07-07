// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEMPLATES_H
#define BITCOIN_TEMPLATES_H

#include <assert.h>
#include <cstddef>

template<typename T>
class Container
{
    T* pObj;
public:
    Container() : pObj(NULL) {}
    Container(T* pObjIn) : pObj(pObjIn) {}
    ~Container()
    {
        if (!IsNull())
            delete(pObj);
    }

    bool IsNull() const
    {
        return pObj == NULL;
    }
    const T& Get() const
    {
        assert(!IsNull());
        return *pObj;
    }
    void Set(T* pObjIn)
    {
        if (!IsNull())
            delete(pObj);
        pObj = pObjIn;
    }
};

#endif // BITCOIN_TEMPLATES_H
