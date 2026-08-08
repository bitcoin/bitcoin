## Index

- The transaction index (`-txindex`) now stores less data on disk; a fully
  rebuilt index takes less than half the space. The index is backwards compatible,
  so existing users will not see the space saving unless the index is recreated.
  To do so, stop the node, delete the `<datadir>/indexes/txindex` directory, and
  restart; rebuilding can take up to a few hours depending on hardware. Progress
  can be monitored using the `getindexinfo` RPC. Once rebuilt, the index can no
  longer be read by previous releases, so downgrading will rebuild it again in
  the old format. When downgrading permanently, delete the
  `<datadir>/indexes/txindex` directory first, since previous releases do not
  reclaim the space used by entries in the new format. (#35531)
