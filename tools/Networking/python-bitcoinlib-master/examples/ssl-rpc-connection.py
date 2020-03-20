#!/usr/bin/env python3

# Copyright (C) 2014 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.


## Instructions

# This sets up SSL on a localhost connection. Not terribly useful but it will be iterated on.

#  Linux: cd ~/.bitcoin
#  Mac: cd ~/Library/Application\ Support/Bitcoin/
#  openssl genrsa -out server.pem 2048
#  openssl req -new -x509 -nodes -sha256 -days 3650 -key server.pem > server.cert
#  The prompts are optional, you can just hit enter

# Verify that your bitcoin.conf exists in the above directory and contains the following lines:
# server=1
# rpcssl=1
# rpcuser=CHANGETHIS
# rpcpassword=CHANGETHAT
# rpcsslciphers=TLSv1_2
# rpcsslprivatekeyfile=server.pem
# rpcsslcertificatechainfile=server.cert

import bitcoin.rpc

proxy_connection = bitcoin.rpc.Proxy()
print(proxy_connection.getnewaddress())
