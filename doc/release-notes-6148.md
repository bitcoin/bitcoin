P2P and Network Changes
-----------------------

The DSQ message, starting in protocol version 70234, is broadcast using the inventory system, and not simply
relaying to all connected peers. This should reduce the bandwidth needs for all nodes, however, this affect will
be most noticeable on highly connected masternodes. (#6148)
