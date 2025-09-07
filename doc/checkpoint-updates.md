# Checkpoint Update Procedures

This project ships a small set of block checkpoints for each supported network.
Checkpoints provide a sanity check during header synchronization and help defend
against deep reorg attacks.

To update checkpoints for a new release:

1. Choose a block hash at a height that is considered final and widely
   accepted on the network.
2. Update `src/kernel/chainparams.cpp` and append the new height and hash to
   the `checkpointData` map for the relevant network.
3. Rebuild the project and run the test suite to ensure the new checkpoint is
   validated during header processing.
4. Mention the checkpoint update in the release notes to aid downstream
   maintainers.

These steps ensure future releases have up-to-date security checkpoints while
keeping the codebase auditable.
