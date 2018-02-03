[Unit]
Description=Dash's distributed currency daemon
After=network.target

[Service]
User=chaincoincore
Group=chaincoincore

Type=forking
PIDFile=/var/lib/chaincoind/chaincoind.pid
ExecStart=/usr/bin/chaincoind -daemon -pid=/var/lib/chaincoind/chaincoind.pid \
-conf=/etc/chaincoincore/chaincoin.conf -datadir=/var/lib/chaincoind -disablewallet

Restart=always
PrivateTmp=true
TimeoutStopSec=60s
TimeoutStartSec=2s
StartLimitInterval=120s
StartLimitBurst=5

[Install]
WantedBy=multi-user.target
