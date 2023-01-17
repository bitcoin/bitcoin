Updated RPCs
--------

- `quorum info`: The new `previousConsecutiveDKGFailures` field will be returned for rotated LLMQs. This field will hold the number of previous consecutive DKG failures for the corresponding quorumIndex before the currently active one. Note: If no previous commitments were found then 0 will be returned for `previousConsecutiveDKGFailures`.
