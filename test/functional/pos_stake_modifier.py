#!/usr/bin/env python3
"""Unit tests for stake modifier generation and caching."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import hash256
from test_framework.util import assert_equal

STAKE_MODIFIER_INTERVAL = 60 * 60


class StakeModifierManager:
    def __init__(self):
        self.modifier = b"\x00" * 32
        self.last = 0

    def compute(self, prev_hash, prev_height, prev_time):
        data = (
            self.modifier
            + bytes.fromhex(prev_hash)[::-1]
            + prev_height.to_bytes(4, "little")
            + prev_time.to_bytes(4, "little")
        )
        return hash256(data)

    def get(self, prev_hash, prev_height, prev_time, ntime):
        if self.last == 0 or ntime - self.last >= STAKE_MODIFIER_INTERVAL:
            self.modifier = self.compute(prev_hash, prev_height, prev_time)
            self.last = ntime
        return self.modifier


class PosStakeModifierTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 0

    def setup_chain(self):
        pass

    def setup_network(self):
        pass

    def run_test(self):
        mgr = StakeModifierManager()
        prev_hash1 = "11" * 32
        prev_height1 = 100
        prev_time1 = 1000000000
        ntime1 = prev_time1 + 16
        mod1 = mgr.get(prev_hash1, prev_height1, prev_time1, ntime1)
        exp1 = hash256(
            b"\x00" * 32
            + bytes.fromhex(prev_hash1)[::-1]
            + prev_height1.to_bytes(4, "little")
            + prev_time1.to_bytes(4, "little")
        )
        assert_equal(mod1, exp1)

        # Within the interval, the modifier stays constant even as block data changes
        prev_hash2 = "22" * 32
        prev_height2 = 101
        prev_time2 = prev_time1 + 16
        ntime2 = ntime1 + 32
        mod2 = mgr.get(prev_hash2, prev_height2, prev_time2, ntime2)
        assert_equal(mod2, mod1)

        # After the interval, a new modifier is computed incorporating previous value
        ntime3 = ntime1 + STAKE_MODIFIER_INTERVAL + 1
        mod3 = mgr.get(prev_hash2, prev_height2, prev_time2, ntime3)
        exp3 = hash256(
            mod1
            + bytes.fromhex(prev_hash2)[::-1]
            + prev_height2.to_bytes(4, "little")
            + prev_time2.to_bytes(4, "little")
        )
        assert_equal(mod3, exp3)
        assert mod3 != mod1


if __name__ == "__main__":
    PosStakeModifierTest(__file__).main()
