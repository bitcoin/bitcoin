<pre>
  DIP: 0022
  Title: Multi-Quorum ChainLocks
  Author(s): sidhujag, UdjinM6, dustinface
  Special-Thanks: Thephez
  Comments-Summary: No comments yet.
  Status: Proposed
  Type: Standard
  Created: 2021-12-15
  License: MIT License
  Requires: 8
</pre>

## Table of Contents

1. [Abstract](#abstract)
1. [Motivation](#motivation)
1. [Prior Work](#prior-work)
1. [Used LLMQ type](#used-llmq-type)
1. [P2P message structure](#p2p-message-structure)
1. [Signing blocks](#signing-blocks)
1. [Handling of signed blocks](#handling-of-signed-blocks)
1. [Conflicting signed blocks](#conflicting-signed-blocks)
1. [Calculations](#calculations)
1. [Security Considerations](#security-considerations)
1. [Copyright](#copyright)

## Abstract

This DIP expands the idea of ChainLocks introduced in DIP0008 by presenting
a way to use multiple LLMQs to sign the chain tip simultaneously, without a
need to rely on a two step process.

## Motivation

Original ChainLocks were introduced to reduce uncertainty when receiving
funds and remove the possibility of 51% mining attacks. It was achieved by
deterministically picking two quorums - one for signing attempts and one for
finalizing these attempts and locking the chain tip. The base assumption in the
original proposal was that it's very hard to obtain any significant number of
masternodes to have any feasible chance to even withhold signing at either of
these steps.

While this assumption still holds, we do see a rise of various masternode
hosting services which accumulate significant number of nodes under their
control. It's not in their direct interests to attack the network but
compromising security of such a service by a malicious actor poses a real
risk for the whole network that must be addressed.

This DIP proposes a new method of creating ChainLocks by using multiple LLMQs
which eliminates the risk described above.

## Prior work

* [DIP 0006: Long Living Masternode Quorums](https://github.com/dashpay/dips/blob/master/dip-0006.md)
* [DIP 0007: LLMQ Signing Requests / Sessions](https://github.com/dashpay/dips/blob/master/dip-0007.md)
* [DIP 0008: ChainLocks](https://github.com/dashpay/dips/blob/master/dip-0008.md)

## Used LLMQ type

All signing sessions/requests involved in ChainLocks must use the `LLMQ_400_60`
LLMQ type.

## P2P message structure

Multi-Quorum ChainLocks are relayed through the P2P network via a `CLSIG`
message which has the following structure:

| Field | Type | Size | Description |
|--|--|--|--|
| version | uint8 | 1 | Version of the message |
| height | int32 | 4 | Height of the signed block |
| blockHash | uint256 | 32 | Hash of the signed block |
| sig | BLSSig | 96 | Recovered BLS signature |
| signersSize | compactSize uint | 1-9 | Bit size of the signers bitvector. Must be equal to the number of active quorums. |
| signers | byte[] | (signersSize + 7) / 8 | Bitset representing the aggregated signers of the BLS signature.</br>The first bit represents the participation of the most recent LLMQ and the last bit the participation of the oldest LLMQ with the following meaning of a bit: <ul><li>`0` Quorum didn't provide a valid `CLSIG`</li><li>`1` Quorum provided a valid `CLSIG`</li></ul> |

## Signing blocks

Signing is done by invoking the [DIP0007 `SignIfMember` operation](https://github.com/dashpay/dips/blob/master/dip-0007.md#void-signifmemberuint256-id-uint256-msghash-int-activellmqs).

The request id to sign a block must be calculated deterministically for
each `blockHeight`.

The message hash is always the hash of the signed block.

The sign hash for the BLS signature is calculated as
`SHA256(llmqType, quorumHash, requestId, blockHash)`, where `llmqType` and
`quorumHash` are taken from the corresponding LLMQ.

The request id for each LLMQ is calculated as
`SHA256("clsig", blockHeight, quorumHash)`.

To ensure the convergence of block signing we require honest quorums to form a
signing queue. The first quorum to sign is the quorum with an index in the
active quorum set equal to `signed_height % size_of_quorum_set`, the next one
is `(signed_height + 1) % size_of_quorum_set` and so on. Each quorum besides
the first one must wait for a `CLSIG` message from the previous quorum before
trying to sign the block itself. When there is a race of two or more blocks,
the signing quorum should prefer the block signed by the previous quorum. If
there is no `CLSIG` from the previous quorum after some time, it's recommended
to look for a `CLSIG` from a quorum before the previous one and so on. When
there is no `CLSIG` from any previous quorum after some extended timeout, the
signing quorum should simply sign the best known block.

After a LLMQ member has successfully recovered the final ChainLocks signature
for its LLMQ, it must create a `CLSIG` P2P message and propagate it to non-SPV
nodes through the inventory system.

## Handling of signed blocks

The node should ensure that the fully signed block is locally accepted as the
next block. If the correct block is already present locally, its chain should
be activated as the new active chain tip. If the correct block is not known
locally, the node must wait for this block to arrive and request it from other
nodes if necessary. If the correct block is already present locally and
an alternative block for the same height is received, the alternative block
must not be added to the currently active chain.

If a block has been received locally and no `CLSIG` message has been received
yet, it should be handled the same way it was handled before the introduction
of ChainLocks. This means the most-work and first-seen/longest-chain rules must be
applied. When the `CLSIG` message for this (or another) block is later
received, the below logic must be applied.

If the `signers` bitset has exactly 1 bit set, each node must perform the
following verification before announcing it to other nodes:

  1. Based on the deterministic masternode list at the given height, the set
  of active quorums must be calculated.
  2. The request id and sign hash must be calculated.
  3. The BLS signature must verify against the quorum public key and the sign
  hash.

The node must wait for the majority of LLMQs to sign the same block hash and
then a new `CLSIG` message with an aggregated `sig` and a combined `signers`
field should be formed and relayed to other nodes, including SPV ones.

If the `signers` bitset has at least `signersSize / 2 + 1` bits set to `1`,
each node must perform the following verification before announcing it to other
nodes:

  1. Based on the deterministic masternode list at the given height, the set
  of active quorums must be calculated.
  2. The request id and sign hash of all quorums which successfully
  participated in the aggregated signature must be calculated.
  3. The BLS signature must verify against the set of quorum public keys and
  the set of corresponding sign hashes of all quorums which successfully
  participated in the aggregated signature.

The first seen aggregated `CLSIG` for a known block is the one that should be
used to lock the chain.

## Conflicting signed blocks

It is not possible for a malicious quorum to double-sign two different blocks
and to produce two valid conflicting recovered signatures. `CLSIG` messages
from LLMQs are signable on a first-come-first-serve basis meaning for a height/quorum hash pair only the first blockhash is allowed to be signed. This differs from dip-0022 of Dashpay but makes a tradeoff around securely forming a chainlock rather than having an intermittent assumption that majority consensus will resolve a chainlock. Instead of having the risk of multiple chainlocks signed at differing blockhashes for a certain height we allow for only the first one to be signed at any blockhash for a certain height and disallow subsequent attempts. This increases the chance of no chainlocks forming as if there are conflicts the majority of quorums will likely not be in agreement on the chainlock however there is a decreased risk of invalid or conflicting chainlocks from establishing and creating forks amongst miners. In the case where no majority is
formed the nodes fall back to the first-seen and most-work/longest-chain rules. Note that
there must be an even number of active quorums for this to work as described.

## Calculations

We consider the scenario where an attacker has assumed control of a number of
masternodes. To withhold a ChainLock an attacker would need to disrupt at least
`signersSize / 2` quorums at the same time and to create a malicious ChainLock
he would need to control at least `signersSize / 2 + 1` of them.

For multi-quorum calculations we use [Python Script](quorum_attack.py).

The success probabilities for an attacker that controls `m` out of the `N`
total masternodes and quorum overlap ratio `r` would then be as follows:

| N | m | r | Withholding a ChainLock | Creating a malicious ChainLock |
|---|-----|---|---------- |---|
|2500|500 | 0 | 4.09e-284 | 0 |
|2500|500 | 0.1 | 1.76e-262 | 0 |
|2500|500 | 0.5 | 2.02e-210 | 0 |
|2500|500 | 1.0 | 5.91e-173 | 4.55e-259 |
|2500|1000| 0 | 1.82e-59  | 8.74e-141 |
|2500|1000| 0.1 | 1.36e-56  | 1.50e-127 |
|2500|1000| 0.5 | 1.46e-46  | 2.79e-87 |
|2500|1000| 1.0 | 9.18e-37  | 8.80e-55 |
|2500|1500| 0 | 8.81e-06  | 2.02e-22 |
|2500|1500| 0.1 | 5.02e-05  | 1.28e-18 |
|2500|1500| 0.5 | 1.46e-46 | 1.75e-07 |
|2500|1500| 1.0 | 0.27  | 0.14 |

## Security Considerations

All security consideration from the original proposal regarding persuading
masternode hosting services to provide transparency, requiring users to check
digital signatures of the downloaded software, etc. still apply.

## Copyright

Copyright (c) 2021 Syscoin Core. [Licensed under the MIT License](https://opensource.org/licenses/MIT)