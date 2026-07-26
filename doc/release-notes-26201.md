RPC
---

- 'taproot' has been removed from 'getdeploymentinfo' because its historical
  activation height is no longer used anywhere in the codebase.
  Applications that rely on `deployments.taproot` to detect Taproot support
  should either assume Taproot to be always active (since Bitcoin Core v24.0),
  or check for `TAPROOT` in the `script_flags` array returned by
  `getdeploymentinfo` (since Bitcoin Core v31.0). (#26201)
