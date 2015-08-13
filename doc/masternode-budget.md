Masternode Budget API
=======================

Dash now supports full decentralized budgets that are paid directly from the blockchain via superblocks once per month. 

Budgets go through a series of stages before being paid:
* prepare - create a special transaction that destroys coins in order to make a proposal
* submit - propagate transaction to peers on network
* voting - lobby for votes on your proposal
* get enough votes - make it into the budget
* finalization - at the end of each payment period, proposals are sorted then compiled into a finalized budget
* finalized budget voting - masternodes that agree with the finalization will vote on that budget
* payment - the winning finalized budget is paid


1. Prepare collateral transaction

mnbudget prepare proposal-name url payment_count block_start dash_address monthly_payment_dash [use_ix(true|false)]

Example: mnbudget prepare cool-project http://www.cool-project/one.json 12 100000 y6R9oN12KnB9zydzTLc3LikD9cCjjQzYG7 1200 true
Output : 464a0eb70ea91c94295214df48c47baa72b3876cfb658744aaf863c7b5bf1ff0 - This is the collateral hash, copy this output for the next step

In this transaction we prepare collateral for "cool-project". This proposal will pay 1200 DASh, 12 times over the course of a year totalling 24000 DASH. 

*Warning -- if you change any fields within this command, the collateral transaction will become invalid. 

2. Submit proposal to network

mnbudget submit cool-project http://www.cool-project/one.json 12 100000 y6R9oN12KnB9zydzTLc3LikD9cCjjQzYG7 1200 464a0eb70ea91c94295214df48c47baa72b3876cfb658744aaf863c7b5bf1ff0

Output : a2b29778ae82e45a973a94309ffa6aa2e2388b8f95b39ab3739f0078835f0491 - This is your proposal hash, which other nodes will use to vote on it

3. Lobby for votes

Double check your information: mnbudget getinfo cool-project
￼
{
    "Name" : "cool-project",
    "Hash" : "a2b29778ae82e45a973a94309ffa6aa2e2388b8f95b39ab3739f0078835f0491",
    "FeeHash" : "464a0eb70ea91c94295214df48c47baa72b3876cfb658744aaf863c7b5bf1ff0",
    "URL" : "http://www.cool-project/one.json",
    "BlockStart" : 100000,
    "BlockEnd" : 100625,
    "TotalPaymentCount" : 12,
    "RemainingPaymentCount" : 12,
    "PaymentAddress" : "y6R9oN12KnB9zydzTLc3LikD9cCjjQzYG7",
    "Ratio" : 0.00000000,
    "Yeas" : 0,
    "Nays" : 0,
    "Abstains" : 0,
    "TotalPayment" : 14400.00000000,
    "MonthlyPayment" : 1200.00000000,
    "IsValid" : true,
    "fValid" : true
}

If everything looks correct, you can ask for votes from other masternodes. To vote on a proposal, load a wallet with masternode.conf. You do not need to access your cold wallet to vote for proposals. 

* mnbudget vote a2b29778ae82e45a973a94309ffa6aa2e2388b8f95b39ab3739f0078835f0491 yes

4.  Make it inte the budget

After you get enough votes, execute "mnbudget projection" to see if you made it into the budget. If you the budget was finalized at this moment which proposals would be in it. Note: Proposals must be active at least 1 day on the network and receive 10% of the masternode network in yes votes in order to qualify (E.g. if there is 2500 masternodes, you will need 250 yes votes.)

mnbudget projection:￼
{
    "cool-project" : {
	    "Hash" : "a2b29778ae82e45a973a94309ffa6aa2e2388b8f95b39ab3739f0078835f0491",
	    "FeeHash" : "464a0eb70ea91c94295214df48c47baa72b3876cfb658744aaf863c7b5bf1ff0",
	    "URL" : "http://www.cool-project/one.json",
	    "BlockStart" : 100000,
	    "BlockEnd" : 100625,
	    "TotalPaymentCount" : 12,
	    "RemainingPaymentCount" : 12,
	    "PaymentAddress" : "y6R9oN12KnB9zydzTLc3LikD9cCjjQzYG7",
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

5. Finalized budget

"main" : {
        "FeeTX" : "d6b8de9a4cadfe148f91e8fe8eed407199f96639b482f956ae6f539b8339f87c",
        "Hash" : "6e8bbaba5113de592f6888f200f146448440b7e606fcf62ef84e60e1d5ac7d64",
        "BlockStart" : 100000,
        "BlockEnd" : 100000,
        "Proposals" : "cool-project",
        "VoteCount" : 46,
        "Status" : "OK"
    },

6. Get paid

When block 1000000 is reached you'll receive a payment for 1200 DASH. 

The following new RPC commands are supported:
* mnbudget "command"... ( "passphrase" )
*  prepare            - Prepare proposal for network by signing and creating tx
*  submit             - Submit proposal for network
*  vote-many          - Vote on a Dash initiative
*  vote-alias         - Vote on a Dash initiative
*  vote               - Vote on a Dash initiative/budget
*  getvotes           - Show current masternode budgets
*  getinfo            - Show current masternode budgets
*  show               - Show all budgets
*  projection         - Show the projection of which proposals will be paid the next cycle
*  check              - Scan proposals and remove invalid
￼
* mnfinalbudget "command"... ( "passphrase" )
*  vote-many   - Vote on a finalized budget
*  vote        - Vote on a finalized budget
*  show        - Show existing finalized budgets

