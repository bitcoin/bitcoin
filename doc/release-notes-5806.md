RPC Changes
-----------

### `submitchainlock` RPC method updates
- **Return Value Addition**: The method now returns the height of the current best ChainLock, allowing clients to easily compare the provided ChainLock height with the current best one.
- **Error Handling**: Added a check for the provided block height. If the block height is not greater than the height of the current best ChainLock, the method will return the best ChainLock's height without attempting to process a new ChainLock.
- **Code Optimization**: Refactored the retrieval of the `LLMQContext` and the best ChainLock's height to occur before the signature validation. This change streamlines the logic and potentially reduces redundant computations when the provided block height is already known to be not the best.
