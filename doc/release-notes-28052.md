Blockstorage
============

Block files are now XOR'd by default with a key stored in the blocksdir.
Previous releases of Bitcoin Core or previous external software will not be able to read the blocksdir with a non-zero XOR-key.
Refer to the `-blocksxor` help for more details.
