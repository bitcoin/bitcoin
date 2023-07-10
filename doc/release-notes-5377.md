Updated RPCs
--------

- `protx diff` RPC returns a new field `quorumsCLSigs`.
This field is a list containing: a ChainLock signature and the list of corresponding quorum indexes in `newQuorums`.

`MNLISTDIFF` P2P message
--------

Starting with protocol version `70230`, the following fields are added to the `MNLISTDIFF` after `newQuorums`.

| Field              | Type                  | Size     | Description                                                         |
|--------------------|-----------------------|----------|---------------------------------------------------------------------|
| quorumsCLSigsCount | compactSize uint      | 1-9      | Number of quorumsCLSigs elements                                    |
| quorumsCLSigs      | quorumsCLSigsObject[] | variable | CL Sig used to calculate members per quorum indexes (in newQuorums) |

The content of `quorumsCLSigsObject`:

| Field         | Type             | Size     | Description                                                                                 |
|---------------|------------------|----------|---------------------------------------------------------------------------------------------|
| signature     | BLSSig           | 96       | ChainLock signature                                                                         |
| indexSetCount | compactSize uint | 1-9      | Number of quorum indexes using the same `signature` for their member calculation            |
| indexSet      | uint16_t[]       | variable | Quorum indexes corresponding in `newQuorums` using `signature` for their member calculation |

Note: The `quorumsCLSigs` field in both RPC and P2P will only be populated after the v20 activation.