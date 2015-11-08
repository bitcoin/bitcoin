// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#ifndef BITCOIN_UNLIMITED_H
#define BITCOIN_UNLIMITED_H

enum
{
    DEFAULT_MAX_GENERATED_BLOCK_SIZE = 1000000
};

extern uint64_t maxGeneratedBlock;

extern void UnlimitedSetup(void);


#endif
