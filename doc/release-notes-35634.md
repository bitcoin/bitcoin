Indexes
-------

- The transaction output spender index (`-txospenderindex`) now uses less disk
  space. Existing indexes remain compatible and no action is required. To reclaim
  the space for data indexed before upgrading, stop the node, delete the
  `<datadir>/indexes/txospenderindex/` directory, and restart with
  `-txospenderindex` enabled to rebuild it. (#35634, #35568)
