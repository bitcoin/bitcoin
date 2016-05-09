Protocol Documentation - 0.12.1
=====================================

This document describes the protocol extensions for all additional functionality build into the Dash protocol. This doesn't include any of the Bitcoin procotol, which has been left in tact in the Dash project. For more information about the core protocol, please see https://en.bitcoin.it/w/index.php?title#Protocol_documentation&action#edit

## Common Structures

### Simple types

uint256  => char[32]

CScript => uchar[]

### COutpoint

Bitcoin Input

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 32 | hash | uint256 | Hash of transactional output which is being referenced
| 4 | n | uint32_t | Index of transaction which is being referenced


### CTXIn

Bitcoin Input

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 36 | prevout | COutPoint | The previous output from an existing transaction, in the form of an unspent output
| 1+ | script length | var_int | The length of the signature script
| ? | script | CScript | The script which is validated for this input to be spent
| 4 | nSequence | uint_32t | Transaction version as defined by the sender. Intended for "replacement" of transactions when information is updated before inclusion into a block.

### CPubkey

Bitcoin Public Key

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 33-65 | vch | char[] | Encapcilated public key of masternode in serialized varchar form

### Masternode Winner

When a new block is found on the network, a masternode quorum will be determined and those 10 selected masternodes will issue a masternode winner command to pick the next winning node. 

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 41+ | vinMasternode | CTXIn | The unspent output of the masternode which is signing the message
| 4 | nBlockHeight | int | The blockheight which the payee should be paid
| ? | payeeAddress | CScript | The address to pay to
| 71-73 | sig | char[] | Signature of the masternode)

## Message Types

### Masternode Winner

When a new block is found on the network, a masternode quorum will be determined and those 10 selected masternodes will issue a masternode winner command to pick the next winning node. 

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 41+ | vinMasternode | CTXIn | The unspent output of the masternode which is signing the message
| 4 | nBlockHeight | int | The blockheight which the payee should be paid
| ? | payeeAddress | CScript | The address to pay to
| 71-73 | sig | char[] | Signature of the masternode)

### Governance Vote

Masternodes use governance voting in response to new proposals, contracts, settings or finalized budgets.

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 41+ | Unspent Output | CTXIn | Unspent output for the masternode which is voting
| 32 | nParentHash | uint256 | Object which we're voting on (proposal, contract, setting or final budget)
| 4 | nVote | int | Yes (1), No(2) or Abstain(0)
| 8 | nTime | int_64t | Time which the vote was created
| 71-73 | vchSig | char[] | Signature of the masternode

### Governance Object

A proposal, contract or setting.

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 1-20 | strName | std::string | Name of the governance object
| 1-64 | strURL | std::string | URL where detailed information about the governance object can be found
| 8 | nTime | int_64t | Time which this object was created
| 4 | nBlockStart | int | Starting block, which the first payment will occur
| 4 | nBlockEnd | int | Ending block, which the last payment will occur
| 8 | nAmount | int_64t | The amount in satoshi's that will be paid out each time
| ? | payee | CScript | Address which will be paid out to
| 32 | nFeeTXHash | uint256 | Hash of the collateral fee transaction

### Finalized Budget

Contains a finalized list of the order in which the next budget will be paid. 

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 41+ | strBudgetName | CTXIn | The unspent output of the masternode which is signing the message
| 4 | nBlockStart | int | The blockheight which the payee should be paid
| ? | vecBudgetPayments | CScript | The address to pay to
| 32 | nFeeTXHash | uint256 | Hash of the collateral fee transaction

### Masternode Announce

Whenever a masternode comes online or a client is syncing, they will send this message which describes the masternode entry and how to validate messages from it. 

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 41+ | vin | CTXIn | The unspent output of the masternode which is signing the message
| # | addr | CService | Address of the main 1000 DASH unspent output
| 33-65 | pubkey | CPubkey | CPubKey of the main 1000 DASH unspent output
| 33-65 | pubkey2 | CPubkey | CPubkey of the secondary signing key (For all other messaging other than announce message)
| 71-73 | sig | char[] | Signature of this message
| 8 | sigTime | int_64t | Time which the signature was created
| 4 | protocolVersion | int | The protocol version of the masternode
| # | lastPing | CMastenrodePing | The last time the masternode pinged the network
| 8 | nLastDsq | int_64t | The last time the masternode sent a DSQ message (for darksend mixing)

### Masternode Ping

CMastenrodePing

Every few minutes, masternodes ping the network with a message that propagates the whole network.

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 41+ | vin | CTXIn | The unspent output of the masternode which is signing the message
| 32 | blockHash | uint256 | Current chaintip blockhash minus 12
| 8 | sigTime | int_64t | Signature time for this ping
| 71-73 | vchSig | char[] | Signature of this message by masternode (verifiable via pubkey2)

### Masternode DSTX

Masternodes can broadcast subsidised transactions without fees for the sake of security in Darksend. This is done via the DSTX message.

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| # | tx | CTransaction | The unspent output of the masternode which is signing the message
| 41+ | vin | CTXIn | Masternode unspent output
| 71-73 | vchSig | char[] | Signature of this message by masternode (verifiable via pubkey2)
| 8 | sigTime | int_64_t | Time this message was signed

### DSSTATUSUPDATE - DSSU

Darksend pool status update

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 4 | sessionID | int | The unspent output of the masternode which is signing the message
| 4 | GetState | int | Masternode unspent output
| 4 | GetEntriesCount | int | Number of entries
| 4 | Status | int | Status of the mixing process
| 4 | errorID | int | Error ID if any

### DSSTATUSUPDATE - DSQ

Asks users to sign final Darksend tx message.

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 4 | vDenom | int | Which denominations are allowed in this mixing session
| 4 | vin | int | unspend output from masternode which is hosting this session
| 4 | time | int | the time this DSQ was created
| 4 | ready | int | if the mixing pool is ready to be executed
| 71-73 | vchSig | char[] | Signature of this message by masternode (verifiable via pubkey2)

### DSSTATUSUPDATE - DSA

Response to DSQ message which allows the user to join a Darksend mixing pool

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| 4 | sessionDenom | int | denomination that will be exclusively used when submitting inputs into the pool
| 4 | txCollateral | int | unspend output from masternode which is hosting this session

### DSSTATUSUPDATE - DSS

User's signed inputs for a group transaction in a Darksend session

| Field Size | Description | Data type | Comments |
| ---------- | ----------- | --------- | -------- |
| # | inputs | CTXIn[] | signed inputs for Darksend session