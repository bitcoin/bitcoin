[Unit]
Description=Bitcoin's distributed currency daemon
After=network.target

[Service]
User=bitcoin
Group=bitcoin

Type=forking
PIDFile=/var/lib/bitcoind/bitcoind.pid
ExecStart=/usr/bin/bitcoind -daemon -pid=/var/lib/bitcoind/bitcoind.pid \
-conf=/etc/bitcoin/bitcoin.conf -datadir=/var/lib/bitcoind -disablewallet

Restart=always
PrivateTmp=true
TimeoutStopSec=60s
TimeoutStartSec=2s
StartLimitInterval=120s
StartLimitBurst=5

[Install]
WantedBy=multi-user.target
