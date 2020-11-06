// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/validation.h>

#include <util/check.h>
#include <util/time.h>
#include <validation.h>

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
