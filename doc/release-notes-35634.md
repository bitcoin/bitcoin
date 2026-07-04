Indexes
-------

- The transaction output spender index (`-txospenderindex`) now stores one byte
  less per spender entry for newly indexed blocks. Existing indexes remain
  compatible and no action is required. To apply the same saving to entries
  indexed before upgrading, stop the node, delete the
  `<datadir>/indexes/txospenderindex/` directory, and restart with
  `-txospenderindex` enabled to rebuild it.
