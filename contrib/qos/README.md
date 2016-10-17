### Qos ###

<<<<<<< HEAD
This is a Linux bash script that will set up tc to limit the outgoing bandwidth for connections to the Bitcoin network. It limits outbound TCP traffic with a source or destination port of 9999, but not if the destination IP is within a LAN (defined as 192.168.x.x).
=======
This is a Linux bash script that will set up tc to limit the outgoing bandwidth for connections to the Crowncoin network. It limits outbound TCP traffic with a source or destination port of 9340, but not if the destination IP is within a LAN (defined as 192.168.x.x).
>>>>>>> origin/dirty-merge-dash-0.11.0

This means one can have an always-on crowncoind instance running, and another local crowncoind/crowncoin-qt instance which connects to this node and receives blocks from it.
