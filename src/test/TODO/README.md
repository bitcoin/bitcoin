
[TODO] Tests to be ported
=========================

### transaction_tests
* *tx_valid*: data file to be ported "data/tx_valid.json"
* *tx_invalid*: data file to be ported "data/tx_invalid.json"
* *basic_transaction_tests*: DONE
* *test_Get*: DONE
* *test_IsStandard*: FAILING on OP_RETURN transactions

### alert_tests
The alert_tests validate the broadcasting and validation of centrally broadcasted alerts to be displayed by the node software.
Updating these tests requires posession of the secret alert keys.

### bloom_tests
Bloom filter testing, further investigation needed.

### miner_tests
The miner_tests validate the creation of blocks using proof-of-work.
To fix these tests, new nonces should be generated to initialize a valid blockchain.
Testing the creation of proof-of-stake blocks should also be considered.
