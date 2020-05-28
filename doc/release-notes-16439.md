Specify block by height in CLI or Debug Console
-----------------------------------------------

The ability to specify a block by a height instead of its hash
has been added to bitcoin-cli and the GUI debug console. This is a
client-side shortcut for first querying the hash via `getblockhash` or
`getbestblockhash`. The syntax is `getblockheader @123456` to refer to
the block at height 123456 or `getblockheader @best` to refer to the tip.
(#16439)
