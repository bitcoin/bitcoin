New RPCs
--------

- `quorum listextended` is the cousin of `quorum list` with more enriched reply. By using `height` parameter RPC will list active quorums at specified height (or at tip if `height` is not specified.)
This RPC returns the following data per quorum grouped per llmqTypes:
  - For each `quorumHash`:
    - `creationHeight`: Block height where its DKG started
    - `quorumIndex`: Returned only for rotated llmqTypes
    - `minedBlockHash`: Hash of the block containing the mined final commitment
    - `numValidMembers`: The total of valid members.
    - `healthRatio`: The ratio of healthy members to quorum size. Range [0.0 - 1.0].
