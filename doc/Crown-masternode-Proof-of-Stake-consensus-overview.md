# Crown Masternode Proof of Stake Consensus System 

## History

The Crown Masternode Proof of Stake system was commissioned in Summer 2018 and delivered in Spring 2019 in the Crown v0.13.0 ("Jade") release . It was designed to overcome known attack vectors in existing Proof of Stake consensus mechanisms, and to provide a unique cold staking solution.

## Traditional Proof of Stake Attack Vectors and Mitigation

5 specific attack vectors were identified and mitigation techniques developed. Additionally, during development the much publicised "fake stake" attack vectors were addressed.

### 1. Long Range 

#### Description 

An attacker overtakes the main chain by producing a competing chain using past staking balances. For example, assume an attacker has a moderately large stake in the network then exchanges their stake for another cryptocurrency (or for fiat). They no longer have a stake in the network at the most recent block but their past stake is still viable from the perspective of some older blocks. This attacker may collude with other attackers who've followed the same steps until they reach 51% of the supply at that point in time. Now they have the ability to quickly generate a fork by forging previous time stamps until the height of the fork is equal to the height of the main chain. This can be done in such a way that the trust scores of the two chains are very close. Without additional safeguards, honest nodes would not be able to differentiate between the honest chain and the attacker's chain, thus potentially forcing a reorg. 

#### Solution

A maximum reorganization depth completely mitigates long range attacks on online validating nodes. The issue still exists for inactive nodes who are re-syncing with the network and new nodes who are syncing for the first time. A fixed checkpointing system is used to prevent syncing nodes from syncing with the attacker's chain. This is done by either directly encoding checkpoints into the source code (long interval checkpointing) or broadcasting checkpoints from a trusted source (short interval checkpointing). Although these solutions may introduce other issues, they do drastically reduce the likelihood of a long range attack.

### 2. Nothing at Stake 

#### Description

A distinction must be made between honest, rationally self interested, and malicious nodes. Honest nodes will always align their actions with what is best for the network. Rationally self interested nodes don't aim to compromise the network but their self interest may lead them in that direction if the incentive structure facilitates it. A malicious node's only goal is to compromise the network. Now imagine a chain with multiple competing forks. No node can be sure which fork will ultimately win, so in order to improve their expected value the rationally self interested nodes stake on every fork. This is an issue for two core reasons: 1. it can cause uncontrollable forking because the network can't reach a consensus on what the main chain is and 2. the stake threshold an attacker needs to launch a 51% attack is reduced because the rationally self interested nodes will effectively assist in the attack by staking on the attacker's chain. 

#### Solution

The double-block protection mechanism allows for the cancellation of the top block for double stakes. This partially mitigates the risk of the nothing at stake attack being successful. Additionally, the rationally self interested nodes may have an incentive to stake on as many chains as possible to increase returns but they also have an incentive to help the network reach consensus in a timely manner. Otherwise, they run the risk of losing their entire deposit due to the dysfunctional network. When considering these positive factors, the likelihood of the nothing at stake problem occurring in reality is very low from both an economic and technical standpoint.

### 3. Stake Grinding 

#### Description

Also known as a pre-computation attack. If there is no context based entropy included in the target calculation then the staker can calculate the future target values that their stake will produce. With this information the staker now has no incentive to remain online before they expect to successfully stake a block. This reduces the quantity of active verifying nodes and thus reduces the security of the network. If the nodes were unable to determine when they would successfully stake then they would have to remain online consistently. Another attack that is based on this same principle is pre-computing the outcome of different events and using the results of those computations to determine how to increase the likelihood of staking consecutive blocks. The act of computing all the possible outcomes is called Stake Grinding and essentially reduces proof of stake consensus to bitcoin's original proof of work consensus that proof of stake was meant to replace. 

#### Solution

The process of including chain based entropy into the target hash calculation is known as stake modification. The more the stake modifier can obfuscate future target hashes, the more difficult it becomes to successfully launch a stake grinding attack. Different stake modification schemes provide different degrees of obfuscation so it's crucial to use the best one available. The most advanced stake modification schemes include deterministic sampling of entropy from previous blocks within the modifier interval and change significantly from block to block. This makes pre-computation attacks unfeasible for any realistic amount of computing power.

### 4. Overcrowding/Undercrowding 

#### Description
In order for a node to be able to successfully stake a block they must provide the proof pointer to their most recent masternode or systemnode reward transaction. The proof pointer is only valid if the payment occurred at most some minimum number of blocks earlier in the chain. This proof pointer interval can be exploited under certain circumstances if not designed with enough flexibility. If the number of active masternodes increases substantially over time then the proportion of them who are able to stake may decrease. This essentially reduces the size of the validator node set and opens up the opportunity for an attacker who holds less than 51% stake in the network to launch an attack. This attack vector is called overcrowding. 

It's also possible to have the opposite problem of a substantial decrease in active masternodes. Consider a masternode that goes offline immediately after their most recent masternode payment. They are still eligible to stake for some amount of time after they go offline. From an incentive structure point of view, it's important that a large proportion of the nodes staking are active masternodes. Otherwise, a node can act as a masternode for a short amount of time (until they receive their first reward) because there aren't many masternodes in the queue. Then, they deactivate the masternode but are able to continue staking for a long period of time. This attack vector is called undercrowding. 

#### Solution

This issue necessitates a dynamic but deterministically generated proof pointer interval based on the network's current knowledge of the masternode set size. Because it requires knowledge of the masternode set size, this model assumes some minimum threshold of consistency across the network in terms of what each node perceives to be the active masternode set. To fulfil this requirement a large sampling interval can be chosen to determine the distribution of masternode rewards for specific public keys. This information can be used to construct the likely size of the set in a deterministic way. The size can then be used to update the proof pointer interval every n blocks.

### 5. Masternode Set Disagreement 

#### Description

The issue of disagreeing on the state of the masternode set here is similar to the causes behind the overcrowding problem; except in this case it isn't just the size of the set that's important, it's also the ordering of masternodes within the queue. The order of the queue determines which masternode will receive a reward next, and any newly proposed block that does not include the correct masternode reward will be rejected by the network. This works as intended the majority of the time but if there's significant disagreement on the queue ordering then this validation requirement can cause uncontrollable forking. A hard fork is a good example of a situation that can lead to disagreement on the queue ordering and the composition of the masternode set. 

#### Solution

The spork system can be utilized to temporarily change consensus rules during especially contentious periods on the network. Spork 8 in the existing spork system (MASTERNODE_PAYMENT_ENFORCEMENT) can be used to disable checks on the masternode payments included in blocks. This would eliminate the issue of uncontrollable forking. The problem is that if an attacker were able to stake many blocks during this period of disabled masternode payment enforcement, they can significantly influence the masternode reward distribution. The distribution is used to deterministically calculate the proof pointer interval so the attacker would be able to manipulate the future behaviour of the network. A couple of different strategies can be employed to mitigate this. One option is to reduce the amount of time the spork is disabled as much as possible so that an attacker wouldn't be able to cause a noticeable change in distribution. If this isn't possible then protocol level solutions need to be implemented. For example, a larger sampling interval would reduce the attacker's impact, but this may produce unwanted externalities under certain conditions. Another approach is to mark newly created blocks and ignore them in future sampling processes.

## System Overview 

The proof of stake consensus system for Crown allows both masternodes and systemnodes to create new blocks and add them to the blockchain. Users do not need to run hot wallets to participate in the staking process.

Masternodes and systemnodes are required to have an amount of CRW “locked” (called node collateral) from use in order to be considered an active node and receive node rewards. In a traditional proof of stake consensus system a stake holder would consume their stake input (coins/UTXO) and be issued fresh coins that include the staking reward plus the value of the stake input that was consumed. Since being an active masternode or systemnode requires that node to keep their collateral untouched, this model is modified for Crown’s system. 

### Stake Pointers 

Crown’s consensus system uses a new concept called stake pointers. A stake pointer can be thought of as a stakeable token that is given to masternodes and systemnodes, and is the input that is consumed by the proof of stake transaction. Stake pointers are “given” to masternodes and systemnodes when they receive masternode and systemnode rewards. Stake pointers are time sensitive (expire 2 days after being issued) and are what are used to determine whether a masternode or systemnode is at least "recently active". Since the list of active masternodes and systemnodes is not deterministic (not stored on the blockchain and is based on each peer’s interpretation of which have been validated) it means that simply checking if a node is considered active is something that could lead to unintentional forks because of disagreements from peers over which node may or may not be considered active. By using the stake pointer system, a peer is able to determine that the staking node was at least active within a recent period of time. 

Stake pointers can only be used once. When a node stakes a block with their stake pointer, the pointer is marked as used, and the node will not be able to use that pointer again. If the node has multiple stake pointers, it will still be able to stake any of the unused pointers before they expire. 

At the most technical level, a stake pointer is simply the transaction hash and the output position that a masternode or systemnode was paid their rewards in. 

### Stake Selection 

Like traditional proof systems such as proof of work and most implementations of proof of stake, Crown uses a target difficulty for each block and the proof hash must be below the target value. A proof hash is done by hashing together the following inputs:

- Stakepointer Hash: The transaction hash that the node reward was paid. 
- Stakepointer Output Index: The position in the vout vector that the reward was paid. 
- Stake Modifier: The block hash that is 100 blocks before the stakepointer.
- Timestamp of Stakepointer: The time that the stakepointer was created. 
- Timestamp of the New Stake: The time that the new stake was created. 

By hashing the above inputs, a 256 bit proof hash is created. The value of the proof hash is then compared to the difficulty target value multiplied by the value of collateral that the node is holding (10,000 for masternode, 500 for systemnode). If the proof hash is less than the new target, then the stake is considered valid and can be used to package a block and and it to the blockchain. By multiplying the target value by the collateral amount, this means that the target is easier to hit for masternodes than it is for systemnodes. 

### Finding a Valid Proof Hash (Stake Minting)

Every input that is in the proof hash, except for the timestamp of the new stake, is constant for a given stake pointer. What this means is that if someone had two different computers with identical wallets and stakepointers, it would give no advantage to their ability to find a valid proof hash because they would produce the exact same hashes. Given the same set of inputs and using the same timestamp for the stake that is being created, the proof hash will always come out the same. What this means is that each stakepointer can simply create a new proof hash for every second that passes, and see if it is less than the difficulty target. If the proof hash is less than the target, the stake is packaged into a block and the node has successfully staked. If the proof hash is more than the target, the node waits another second and then tries creating a new proof hash. This process is continually repeated while a node has valid stake pointers.