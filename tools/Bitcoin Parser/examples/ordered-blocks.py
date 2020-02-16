import sys
sys.path.append('..')
from blockchain_parser.blockchain import Blockchain

# Instantiate the Blockchain by giving the path to the directory 
# containing the .blk files created by bitcoind
blockchain = Blockchain(sys.argv[1])

# To get the blocks ordered by height, you need to provide the path of the
# `index` directory (LevelDB index) being maintained by bitcoind. It contains
# .ldb files and is present inside the `blocks` directory
for block in blockchain.get_ordered_blocks(sys.argv[1] + '/index', end=1000):
    print("height=%d block=%s" % (block.height, block.hash))