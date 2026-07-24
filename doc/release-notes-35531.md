## Index

- The transaction index (`-txindex`) now stores less data on disk, more than
  halving the size of a fully rebuilt index. The index is backwards compatible,
  so existing users will not see the space saving unless the index is recreated.
  To do so, stop the node, delete the `<datadir>/indexes/txindex` directory, and
  restart; rebuilding can take up to a few hours depending on hardware. Once
  rebuilt, the index can no longer be read by previous releases, so downgrading
  will require rebuilding it again. (#35531)
