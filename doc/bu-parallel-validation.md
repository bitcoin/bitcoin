Parallel Block Validation
==========================================================


1. What is Parallel Block Validation?
-------------------------------------

Essentially Parallel Validation is a simple concept. Rather than validating each block within the main processing thread, we
instead create a separate thread to do the block validation.  If more than one block arrives to be processed then
we create yet another thread.  There are currently up to 4 parallel block processing threads available making a big block DDOS
attack impossible.  Furthermore, if any attacker were somehow able to jam all 4 processing threads and another block
arrived, then the processing for the largest block would be interrupted allowing the smaller block to proceed, unless the larger
block or blocks have most proof of work.  So only the most proof of work and smallest blocks will be allowed to finish in such
as case.

If there are multiple blocks processing at the same time, when one of the blocks wins the race to complete, then the other
threads of processing are interrupted and the winner will be able to update the UTXO and advance the chain tip.  However, the 
other blocks that were interrupted will still be stored on disk in the event of a re-org. The blocks that are allowed to finish
or are interrupted depend again on the rules of which block has the most proof of work.  See section: `Special Cases and possible attacks`
for a more detailed understanding of this potential problem.


2. The internals
----------------

The following describes the internal mechanisms used to achieve parallel validation.


2a) Script Check Queues:  A total of four script check queues are created with their own thread group which are used to validate
signatures.  Each new block that arrives will be assigned one of those queues during the validation process.

2b) Semaphores:  There one semaphore used for managing block validations which is sized equal to the number of script check queues. 

2c) Locking: The parallel processing is made possible by first having separate threads for validation, but then largely by managing 
the internal locks of `cs_main` in the `ConnectBlock()` function which is found in the `src/main.cpp` source file. During the 
process of `CheckInputs()` we do not have to maintain the lock on `cs_main` which allows other competing threads to continue their 
own validation; the reason for this is that each thread of validation uses it's own view of the UTXO and the scriptcheckqueue's and
UTXO cache's have their own internal locking mechanisms. Furthermore when it comes time to wait for the script threads to finish we
also do not need to maintain the `cs_main` locks during the `control.Wait()` where we wait for the script threads to finish up. 

The unlocking and locking of `cs_main` is not invoked during the mining process but only when we receive a new block from an external
source which needs validation.  In this way we always give priority to the miner's newly mined blocks.

2d) Quitting threads:  If multiple blocks are competing to win the validation race or if all 4 script queues are in use and another new block 
arrives there are several ways we use to stop one or all of the other competing threads.  We do this by first sending a message to quit 
the script threads which prevents them from completing their verification, followed by issuing a Quit message to the blockvalidation threads. Also, if one 
block has finished and has advanced the tip, the other concurrent threads may see that the tip has advanced and will exit their validation threads.

2e) Temp view cache:  Each processing thread has it's own temporary view of the UTXO which it can use to pre-validate the inputs and ensure 
that the block is valid (as each input is checked the UTXO must be updated before the next input can be checked because often the 
current input depends on some previous output in the same block). When and if a processing thread wins the validation race it will flush it's 
temporary and now updated view of the UTXO to the base view (in memory UTXO cache).  This is key to having several threads of 
validation running concurrently, since we can not have multiple threads all updating the same UTXO base view at the same time.

2f) nSequenceId: In order to have the correct `pindexMostWork` we must update the `nSequenceId` an additional time after the
winning block updates the `UTXO` and advances the chain tip. We can not only rely only on the `pindexMostWork` returned from the
`CBlockIndexWorkComparator()` as was previously the case.  That is because pindexMostWork returned from the comparator may not necessarily
point to the winning block.  The `nSequenceId` is swapped between the winning and losing blocks such that the winning block has the lowest
nSequenceId and the losing blocks nSequenceId's are bumped up one, while at the same time keeping their relative ordering.

2g) UTXO locking: Due to the need to remove the cs_main locks during the time where we are checking inputs we must have locking
enabled on the reads and writes to both the UTXO db cache as well as the in memory UTXO cache.  Having these more granular locks
in place will also allow us to implement multi-threading for transaction validation in a future release.


3. IBD and new blocks
---------------------- 

Parallel Validation is in effect at all times during both IBD and when new blocks arrive.


4. How is mining affected
--------------------------

Mining is not affected by Parallel Validation.  When new blocks are created locally they bypass parallel validation.  In other words, the `cs_main` locks 
are not unlocked and then locked, giving priority to mined blocks and allowing the validation process to be completed as quickly as possible.  Whether parallel validation
is invoked or not depends on the boolean `fParallel`.  When set to `true` then parallel validation is in effect, and when `false` , as in the case
of generating a new block, then it is turned off.
NOTE: Miners will still use parallel validation if a block arrives from an external source. It is only turned off when validating a block they
mine themself.


5. Special Cases and possible attacks
-------------------------------------

a) Parallel Validation when two separate chains are being mined.

`Occasionally and under normal conditions, two separate chains can be mined for a short while until one chain overtakes
the other. Under this condition we could have a situatation where a very large block is being validated on fork 1 which
is the chain active tip, however another block arrives on fork 2. While we would like to do a parallel validation on fork 2, it
is not actually possible unless we re-org to fork 2. And if we re-org to fork 2 then any blocks that are still validating on fork 1 will fail.
So all we can do in this situation is to allow the block on fork 1 to finish its validation, unless the
fork 2 chain receives yet another block and pulls ahead as being the most work chain. If fork 2 pulls ahead then we will 
initiate a re-org to fork 2 and begin validation on fork 2, while terminating any remaining block validations on fork 1.

To put it another way and more simply: The outcome of this scenario under PV will be the same as it is currently; there is no
change in terms how the situation is resolved.  The only difference being that any PV threads that are still running on fork 1 need
to be termintated before the reorg to fork 2.`

b) The following attack is outlined, however after some discussion, it was found not to be possible.  The reason being that
PV will only be happening on the chainactive tip which means that the blocks being validated will all have the same parent.
Under those circumstances PV blocks will have the same proof of work and we would not have a situation where one block
had less work than another:

`We have to account for the possiblity that someone may create a lower proof of work block at the same time as a higher one is mined. If the lower POW block 
were to win the validation race then we may end up mining on top of that block and therefore end up mining the wrong chain. To get around that, 
when there are two blocks validating at the same time and if the POW doesn't match and the lower POW block wins the race, then we have to wait 
for the other block to finish before advancing the tip. In this situation if the higher POW block finishes second but without errors then we it 
will be the one that advances the tip and the lower POW block will get orphaned.  But in general the first block to finish will advance the tip 
and then terminate any competing threads.`



