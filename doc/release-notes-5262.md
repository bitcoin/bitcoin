Coinbase Changes
------------------------

Once v20 activates, Coinbase transactions in all mined blocks must be of version 3.

Version 3 of Coinbase will include the following two fields:
- bestCLHeightDiff (uint32)
- bestCLSignature (BLSSignature)

bestCLSignature will hold the best Chainlock signature known at the moment of mining.
bestCLHeightDiff is equal to the diff in heights from the mined block height to the actual Chainlocked block height.

Although miners are forced to include a version 3 Coinbase, the actual real-time best Chainlock signature isn't required as long as blocks contain a bestCLSignature equal to (or newer than) the previous block.

Note: Until the first bestCLSignature is included in a Coinbase once v20 activates, null bestCLSignature and bestCLHeightDiff values are perfectly valid.
