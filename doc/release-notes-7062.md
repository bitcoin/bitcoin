# RPC changes

- `quorum dkginfo` will now require that nodes run either in watch-only mode (`-watchquorums`) or as an
  active masternode (i.e. masternode mode) as regular nodes do not have insight into network DKG activity.

- `quorum dkgstatus` will no longer emit the return values `time`, `timeStr` and `session` on nodes that
  do not run in either watch-only or masternode mode as regular nodes do not have insight into network
  DKG activity.
