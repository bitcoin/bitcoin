#warning It is recommended you create a new checkpoints_def.cpp after downloading blocks using -saveblockcheckpointfile
/*
This is a placholder file that should be replaced by the output of bitcoind -saveblockcheckpointfileto
It will be filled with the least-significant 32 bits of block hashes in the best block chain. By checkpointing
the early, difficulty-1 blocks several potential denial-of-service attacks are prevented.
*/
const unsigned int LSBCheckpoints[] = {};
