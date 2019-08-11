P2P Changes
-----------
- The default value for the -peerbloomfilters configuration option (and, thus, NODE_BLOOM support) has been changed to false.
  This resolves well-known DoS vectors in Bitcoin Core, especially for nodes with spinning disks. It is not anticipated that
  this will result in a significant lack of availability of NODE_BLOOM-enabled nodes in the coming years, however, clients
  which rely on the availability of NODE_BLOOM-supporting nodes on the P2P network should consider the process of migrating
  to a more modern (and less trustful and privacy-violating) alternative over the coming years.
