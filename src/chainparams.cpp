// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "tinyformat.h"
#include "ChainParamsUtil.h"

static CChainParams* g_pCurrentParams = 0;

void CChainParams::Initialize()
{
    // 重複除外.
    static int flag = 0;
    if (flag)return;
    flag = 1;

    // インスタンス明示初期化.
    void Init1();
    void Init2();
    void Init3();
    Init1();
    Init2();
    Init3();
}

CChainParams::CChainParams(NetworkType networkType)
{
    m_networkType = networkType;
    CChainParamsUtil::RegisterParamsOfNetworkType(m_networkType, this);
}

const CChainParams &Params() {
    assert(g_pCurrentParams);
    return *g_pCurrentParams;
}

CChainParams& Params(NetworkType chain)
{
    CChainParams* p = CChainParamsUtil::GetParamsOfNetworkType(chain);
    if (!p) {
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
    }
    return *p;
}

void SelectParams(NetworkType network)
{
    CChainParams::Initialize();
    SelectBaseParams(network);
    g_pCurrentParams = &Params(network);
}

