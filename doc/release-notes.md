HOdlcoin Core version 2.0.0 is now available from:

  <https://github.com/HOdlcoin/HOdlcoin/releases/>

This is a new major version release, bringing bug fixes, cross-platform voting,
and new block generation requirements. It is the first hard-fork and a required
upgrade for all clients!

Please report bugs using the issue tracker at github:

  <https://github.com/HOdlcoin/HOdlcoin/issues>

Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/HOdlcoin-Qt (on Mac) or
hodlcoind/hodlcoin-qt (on Linux).

Downgrade warning
------------------

Because this release issues a hard-fork of the network, downgrading to prior versions
will not be possible once the network reaches a block height of 86000.

Notable changes since 1.0.0
============================

Hard fork for new block generation requirements
--------------------------------------------------------

New requirements for block generation have been introduced, requiring generated
blocks to automatically enter into a term deposit for a period of one year.

1. This release will automatically generate a term deposit with a one year
   period for all generated blocks after the fork.

2. This release will no longer relay any new blocks that do not carry the
   required term deposit length.
   
3. Clients that generate (and attempt to relay) blocks that don't meet
   the new requirements will be DoS'd, resulting in network a network
   ban. Default client ban time is 24 hours, but individual clients
   can modify this at runtime.


For more information about the hard-forking change, please see
<https://www.reddit.com/r/Hodl/comments/4qpx9g/we_hodl_that_block_rewards_should_remain/?st=iq75y3k6&sh=a9c7bed1>

**Notice to miners:** HOdlcoin block generation will now automatically
create a term deposit for a one year length for the generated block.
This affects ALL newly generated blocks after a chain height of 86000.

- If you are solo mining, this will affect you the moment you upgrade
  HOdlcoin Core, which must be done prior to the network reaching a 
  height of 86000.

- If you are mining with the stratum mining protocol: this does not
  affect how you mine, however stratum operators will need to adjust
  their payout policies.

- If you are mining with the getblocktemplate protocol to a pool: this
  will affect you at the pool operator’s discretion, adjusting their
  payout policies to accomodate the new block generation requirements.

Nutocray consensus voting
----------------------------------------------------------------

Nutocray is an open source voting platform that works with HOdlcoin's
message signing system allowing HOdlers to vote on motions, or 
create their own motions to enact changes to the HOdlcoin client
or network.

Details can be found at: <https://nutocracy.herokuapp.com/>

2.0.0 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

- [a5662310450e76ae9dd8ce7b2f0339c370369678] `a566231` Fork at 86000 to in accordance with Nutocracy motion
- [4f9cc23ab5b97d1857e73edec2a0ee27795a052f] `4f9cc23` Only ask for password once when signing multiple statements
- [3eb0e09d10ed02cd7525ac0a951ef2dfbbca8f3e] `3eb0e09` Allow message signing using multiple addresses (separated by semicolons) at the same time
- [d59ac02280bbed979d93fa6f7f6f442f6d98d64a] `d59ac02` Allow number of outbound connections for churn nodes to be defined
- [c9dfe7fb25844daf4dc5fa9233a8ee3557d012e3] `c9dfe7f` Nutoshi words of wisdom randomization correction
- [890e9e4fb513a60d27fe266252d05a80de165922] `890e9e4` Matured, unproductive - this message has been updated
- [4b310cf921ad7aeb1a49f4fa20ab09347c1d31c5] `4b310cf` Update max transaction amount
- [39b7b66b0bc58e14d61019e975c019a26f1ef0fb] `39b7b66` Removed unused boost references

Credits
=======

Thanks to everyone who directly contributed to this release:

- FreeTrade
- Fuzzbawls
- SamSmith


And those who contributed additional code review and/or security research, as well as all pool operators and service providers.
