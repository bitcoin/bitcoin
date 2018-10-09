Minimum Fee Rate Policies
-------------------------

This release drops the default minimum fee rates required to relay
transactions across the network and for miners to consider the
transaction in their blocks to 2 bits per kilobyte (0.00000200 BTC/kB,
200 satoshis/kB), and the default additional fee rate required for
replacing a transaction to 1 bit per kilobyte (0.00000100 BTC/kB, 100
satoshis/kB). The previous default for all these values was 10 bits
per kilobyte. These defaults can be overridden using the minrelaytxfee,
blockmintxfee and incrementalrelayfee options.

Note that until these lower defaults are widely adopted across the
network, transactions created with lower fee rates may not propagate
and may not be mined. As a result, the wallet code continues to use a
default fee for low-priority transactions of 10 bits per kilobyte and
an incremental fee of 50 bits per kilobyte when replacing transactions.

