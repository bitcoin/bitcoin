#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getmerkleblocks RPC bloom-filter size prechecks.

Now that CBloomFilter deserialization is bounded (LIMITED_VECTOR on vData),
getmerkleblocks parses the leading CompactSize vData count by hand so that an
oversized filter still reproduces the historical RPC errors instead of a
generic length-limit failure (see the precheck in src/rpc/blockchain.cpp).

This pins that byte-level boundary behaviour:

  - a fully present oversized filter (all vData bytes plus the fixed trailer)
    historically deserialized and then failed IsWithinSizeConstraints(), so it
    must still raise RPC_INVALID_PARAMETER ("Filter is not within size
    constraints");
  - an oversized declaration with the vData bytes omitted, or with the trailing
    fixed fields short by even one byte, historically raised a DataStream
    end-of-data failure, so it must still surface that RPC_MISC_ERROR;
  - a well-formed in-bounds filter is accepted (the precheck falls through) and
    returns an array.
"""

from test_framework.messages import ser_compact_size
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

# Keep in sync with MAX_BLOOM_FILTER_SIZE in src/common/bloom.h.
MAX_BLOOM_FILTER_SIZE = 36000
# Wire size of the fixed fields serialized after vData:
# nHashFuncs (uint32) + nTweak (uint32) + nFlags (uint8).
FILTER_TRAILER_SIZE = 9


def raw_filter(declared_vdata_size, vdata_bytes, trailer_bytes):
    """Build a raw bloom-filter wire blob with an arbitrary declared vData
    CompactSize count and an arbitrary number of vData/trailer bytes actually
    present, so truncation boundaries can be exercised directly."""
    return (
        ser_compact_size(declared_vdata_size)
        + b"\x00" * vdata_bytes
        + b"\x00" * trailer_bytes
    ).hex()


class GetMerkleBlocksTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        genesis_hash = node.getblockhash(0)
        oversized = MAX_BLOOM_FILTER_SIZE + 1

        # Fully present oversized filter: deserialization would succeed, so the
        # historical response is the IsWithinSizeConstraints() rejection.
        complete = raw_filter(oversized, oversized, FILTER_TRAILER_SIZE)
        assert_raises_rpc_error(-8, "Filter is not within size constraints",
                                node.getmerkleblocks, complete, genesis_hash, 1)

        # Oversized declaration with the vData bytes omitted: reading vData hits
        # end-of-data.
        truncated_body = raw_filter(oversized, 0, 0)
        assert_raises_rpc_error(-1, "DataStream::read(): end of data",
                                node.getmerkleblocks, truncated_body, genesis_hash, 1)

        # All vData present but the trailer short by one byte: exercises the
        # FILTER_TRAILER_SIZE boundary of the precheck, still end-of-data.
        truncated_trailer = raw_filter(oversized, oversized, FILTER_TRAILER_SIZE - 1)
        assert_raises_rpc_error(-1, "DataStream::read(): end of data",
                                node.getmerkleblocks, truncated_trailer, genesis_hash, 1)

        # A well-formed in-bounds filter falls through the precheck and is
        # handled normally. Build one explicitly (3-byte all-zero vData,
        # nHashFuncs=1, nTweak=0, nFlags=0); it matches nothing, so the genesis
        # block yields no merkleblocks.
        valid = (
            ser_compact_size(3)
            + b"\x00\x00\x00"
            + (1).to_bytes(4, "little")   # nHashFuncs
            + (0).to_bytes(4, "little")   # nTweak
            + b"\x00"                      # nFlags
        ).hex()
        assert_equal(node.getmerkleblocks(valid, genesis_hash, 1), [])


if __name__ == '__main__':
    GetMerkleBlocksTest().main()
