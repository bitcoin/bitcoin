P2P and Network Changes
-----------------------

### Improved Onion Connectivity

To enhance censorship resistance and mitigate network partitioning risks, Dash Core now aims to maintain at least two
outbound onion connections and generally protects these connections from eviction. As a result of the low percentage
of gossiped addresses being onion nodes, it was often the case where, unless you specify `onlynet=onion`, a node
would rarely if ever establish any outbound onion connections. This change ensures that nodes which can access the
onion network maintain a few onion connections. As a result, network messages could continue to propagate across
the network even if non-onion IPv4 traffic is blocked, reducing the risk of partitioning. Additionally, this update improves
security by enabling p2p encryption for these peers. (#6147)