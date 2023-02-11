// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/validation.h>

#include <util/check.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

void TestChainState::ResetIbd()
{
    m_cached_finished_ibd = false;
    assert(IsInitialBlockDownload());
}

void TestChainState::JumpOutOfIbd()
{
    Assert(IsInitialBlockDownload());
    m_cached_finished_ibd = true;
    Assert(!IsInitialBlockDownload());
}

void ValidationInterfaceTest::BlockConnected(CValidationInterface& obj, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
{
    obj.BlockConnected(block, pindex);
}
