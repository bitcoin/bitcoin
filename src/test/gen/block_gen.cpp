// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <test/gen/block_gen.h>

#include <rapidcheck/Gen.h>

/** Generator for the primitives of a block header */
rc::Gen<BlockHeaderTup> BlockHeaderPrimitives()
{
    return rc::gen::tuple(rc::gen::arbitrary<int32_t>(),
        rc::gen::arbitrary<uint256>(), rc::gen::arbitrary<uint256>(),
        rc::gen::arbitrary<uint32_t>(), rc::gen::arbitrary<uint32_t>(),
        rc::gen::arbitrary<uint32_t>());
}
