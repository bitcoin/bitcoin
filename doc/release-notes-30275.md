RPC
---

- The default mode for the `estimatesmartfee` RPC has been updated from `conservative` to `economical`.
  which is expected to reduce overestimation for many users, particularly if Replace-by-Fee is an option.
  For users that require high confidence in their fee estimates at the cost of potentially overestimating,
  the `conservative` mode remains available.
