New RPCs
--------

- `quorum listextended` is the cousin of `quorum list` with more enriched reply. Like `quorum list` "count" parameter can be used. Will list active quorums if "count" is not specified.
This RPC returns the following data per quorum grouped per llmqTypes:
  - For each `quorumHash`:
    - `creationHeight`: Block height where its DKG started
    - `quorumIndex`: Returned only for rotated llmqTypes
    - `minedBlockHash`: Hash of the block containing the mined final commitment
