### QoS (Quality of service) ###

This is a Linux bash script that will set up tc to limit the outgoing bandwidth for connections to the Widecoin network. It limits outbound TCP traffic with a source or destination port of 8553, but not if the destination IP is within a LAN.

This means one can have an always-on widecoind instance running, and another local widecoind/widecoin-qt instance which connects to this node and receives blocks from it.
