POP_PAYOUT_DELAY = 150
NETWORK_ID = 0x3ae6ca26ff
POP_BLOCK_VERSION_BIT = 0x80000
POW_PAYOUT = 50 # 50 because most of tests use original Bitcoin's payout curve
POP_ACTIVATION_HEIGHT = 200
POW_REWARD_PERCENTAGE = 50

# PopData p;
# p.version = 1;
# auto root = p.getMerkleRoot();
# ASSERT_EQ(HexStr(root), "eaac496a5eab315c9255fb85c871cef7fd87047adcd2e81ba7d55d6bdeb1737f");
EMPTY_POPDATA_ROOT_V1 = bytes.fromhex("eaac496a5eab315c9255fb85c871cef7fd87047adcd2e81ba7d55d6bdeb1737f")