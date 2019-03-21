# Governance

Unlike blockchain projects that rely on donations and pre-mined endowments, Crown reserves 25% of each block reward to fund its own development. This is achieved by the use of the governance system to distribute the reserved funds monthly to various projects that community voted for. The payment itself happens approximately once a month (more precisely, once every 43200 blocks). This payment occurs through a special block, called a “superblock.” This superblock is essentially a regular block with a coinbase transaction added which pays money to the proposals selected for budgeting in the current voting cycle.

## Monthly Budget Cycle

Budgets go through a series of stages before being paid:
1. Proposal submission, which itself is a two-step process:
   * prepare - create a special transaction that destroys coins in order to make a proposal.
   * submit - propagate transaction to peers on network.
2. Voting - lobby for votes on your proposal and, hopefully, get enough votes to make it into the budget.
3. Finalization - at the end of each payment period, proposals are sorted then compiled into a finalized budget:
   * finalized budget submission - finalized budget is submitted into the network.
   * finalized budget voting - masternodes that agree with the finalization will vote on that budget.
4. Payment - the winning finalized budget is paid at the beginning of the next cycle.


## Prepare Collateral Transaction

Proposal preparation is done via console command:
```
mnbudget prepare proposal-name url payment_count block_start crown_address monthly_payment_crown
```

Example: ```mnbudget prepare cool-project http://www.cool-project/one.json 12 1296000 CRWy6R9oN12KnB9zydzTLc3LikD9cCjjQzYG7 1200```

Output: ```464a0eb70ea91c94295214df48c47baa72b3876cfb658744aaf863c7b5bf1ff0``` - This is the collateral hash, copy this output for the next step.

In this transaction we prepare collateral for "_cool-project_". This proposal will pay _1200_ CRW, _12_ times over the course of a year totaling _24000_ CRW. 

**Warning -- if you change any fields within this command, the collateral transaction will become invalid.** 

## Submit Proposal to Network

Proposal submission is done via console command:
```
mnbudget submit prepare proposal-name url payment_count block_start crown_address monthly_payment_crown collateral_hash
```

Example: ```mnbudget submit cool-project http://www.cool-project/one.json 12 1296000 CRWy6R9oN12KnB9zydzTLc3LikD9cCjjQzYG7 1200 464a0eb70ea91c94295214df48c47baa72b3876cfb658744aaf863c7b5bf1ff0```

Output : ```a2b29778ae82e45a973a94309ffa6aa2e2388b8f95b39ab3739f0078835f0491``` - 
This is your proposal hash, which other nodes will use to vote on it.

## Lobby for votes

Double check your information: ```mnbudget getinfo cool-project```
￼
```
{
    "Name" : "cool-project",
    "Hash" : "a2b29778ae82e45a973a94309ffa6aa2e2388b8f95b39ab3739f0078835f0491",
    "FeeHash" : "464a0eb70ea91c94295214df48c47baa72b3876cfb658744aaf863c7b5bf1ff0",
    "URL" : "http://www.cool-project/one.json",
    "BlockStart" : 1296000,
    "BlockEnd" : 100625,
    "TotalPaymentCount" : 12,
    "RemainingPaymentCount" : 12,
    "PaymentAddress" : "CRWy6R9oN12KnB9zydzTLc3LikD9cCjjQzYG7",
    "Ratio" : 0.00000000,
    "Yeas" : 0,
    "Nays" : 0,
    "Abstains" : 0,
    "TotalPayment" : 14400.00000000,
    "MonthlyPayment" : 1200.00000000,
    "IsValid" : true,
    "fValid" : true
}
```

If everything looks correct, you can ask for votes from other masternodes. To vote on a proposal, load a wallet with _masternode.conf_ file.

```mnbudget vote a2b29778ae82e45a973a94309ffa6aa2e2388b8f95b39ab3739f0078835f0491 yes```

Since version 0.12.4 you can also vote in the Masternode tab in GUI wallet.

## Make It into the Budget

After you get enough votes, execute ```mnbudget projection``` to see if you made it into the budget. This shows which proposals would be in the budget if it was finalized at this moment . 

Note: Proposals must be active at least 1 day on the network and receive at least 5% of the masternode network in yes votes in order to qualify. (e.g. if there is 2500 masternodes, you will need 125 yes votes)

```mnbudget projection```:￼
```
{
    "cool-project" : {
	    "Hash" : "a2b29778ae82e45a973a94309ffa6aa2e2388b8f95b39ab3739f0078835f0491",
	    "FeeHash" : "464a0eb70ea91c94295214df48c47baa72b3876cfb658744aaf863c7b5bf1ff0",
	    "URL" : "http://www.cool-project/one.json",
	    "BlockStart" : 1296000,
	    "BlockEnd" : 100625,
	    "TotalPaymentCount" : 12,
	    "RemainingPaymentCount" : 12,
	    "PaymentAddress" : "CRWy6R9oN12KnB9zydzTLc3LikD9cCjjQzYG7",
	    "Ratio" : 1.00000000,
	    "Yeas" : 33,
	    "Nays" : 0,
	    "Abstains" : 0,
	    "TotalPayment" : 14400.00000000,
	    "MonthlyPayment" : 1200.00000000,
	    "IsValid" : true,
	    "fValid" : true
	}
}
```

## Finalized budget

In order to see finalized budgets execute `mnfinalbudget show`:

```
"main" : {
        "FeeTX" : "d6b8de9a4cadfe148f91e8fe8eed407199f96639b482f956ae6f539b8339f87c",
        "Hash" : "6e8bbaba5113de592f6888f200f146448440b7e606fcf62ef84e60e1d5ac7d64",
        "BlockStart" : 1296000,
        "BlockEnd" : 1296000,
        "Proposals" : "cool-project",
        "VoteCount" : 46,
        "Status" : "OK"
    },
```

## Get paid

When block ```1296000``` is reached you'll receive a payment for ```1200``` CRW if your proposal passed (number of Yes votes exceeded number of No/Abstain votes by at least 10% of the number of masternodes). 

## Governance-Related Console commands

The following new RPC commands are supported:

Command `mnbudget "command"... ( "passphrase" )`:
- `prepare` - Prepare proposal for network by signing and creating tx
- `submit` - Submit proposal for network
- `vote-many` - Vote on a Crown initiative
- `vote-alias` - Vote on a Crown initiative
- `vote` - Vote on a Crown initiative/budget 
- `getvotes` - Show current masternode budgets
- `getinfo` - Show current masternode budgets
- `show` - Show all budgets
- `projection` - Show the projection of which proposals will be paid the next cycle
- `check` - Scan proposals and remove invalid
￼
Command `mnfinalbudget "command"... ( "passphrase" )`
- `vote-many` - Vote on a finalized budget
- `vote` - Vote on a finalized budget
- `show` - Show existing finalized budgets
