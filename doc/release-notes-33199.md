Fee Estimation
========================

- The Bitcoin Core fee estimator minimum fee rate bucket was updated from **1 sat/vB** to **0.1 sat/vB**,
  which matches the node’s default `minrelayfee`.
  This means that for a given confirmation target, if a sub-1 sat/vB fee rate bucket is the minimum tracked
  with sufficient data, its average value will be returned as the fee rate estimate.

- Note: Restarting a node with this change invalidates previously saved estimates in `fee_estimates.dat`, the fee estimator will start tracking fresh stats.
