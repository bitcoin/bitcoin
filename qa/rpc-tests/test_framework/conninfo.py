#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import os

from .uhttpconnection import UHTTPConnection

# UNIX socket name for RPC connection (in data directory)
RPC_SOCKET_NAME = "rpc_socket"

class RPCConnectInfo:
    '''
    Base class for RPC connection info. This class encapsulates both information
    required to connect to RPC, as to configure bitcoind to listen on that transport.
    '''
    def make_connection(self):
        '''
        Returns the 'connection' argument to pass to the RPCAuthService.
        Can be None to let that class figure it out itself.
        '''
        raise NotImplementedError

    @property
    def bitcoind_args(self):
        '''
        Return arguments for setting up bitcoind.
        '''
        raise NotImplementedError

class RPCConnectInfoTCP:
    '''RPC connection info for connecting over TCP'''
    def __init__(self, node_number, auth, rpchost, rpcport):
        self.node_number = node_number # node number is just for informational purposes
        self.auth = auth
        self.host = '127.0.0.1'
        self.port = rpcport
        if rpchost: # "rpchost" can override both host and port
            parts = rpchost.split(':')
            self.host = parts[0]
            if len(parts) == 2:
                self.port = int(parts[1])
        self.url = "http://%s:%s@%s:%d" % (auth[0], auth[1], self.host, self.port)

    def make_connection(self):
        # This is done inside the AuthServiceProxy
        return None

    @property
    def bitcoind_args(self):
        # rpchost is ignored here because the test will take care of passing in the
        # appropriate rpcbind arguments.
        return [("rpcuser", self.auth[0]),
                ("rpcpassword", self.auth[1]),
                ("rpcport", str(self.port))]

class RPCConnectInfoUNIX:
    '''RPC connection info for connecting over UNIX socket'''
    def __init__(self, node_number, auth, dirname):
        self.node_number = node_number
        self.auth = auth
        self.sockname = os.path.join(dirname, RPC_SOCKET_NAME)
        # use "localhost" as fake hostname. It doesn't matter.
        self.url = "http://%s:%s@localhost" % auth

    def make_connection(self):
        return UHTTPConnection(self.sockname)

    @property
    def bitcoind_args(self):
        return [("rpcuser", self.auth[0]),
                ("rpcpassword", self.auth[1]),
                ("rpcbind", ":unix:"+self.sockname),
                ("rpcconnect", ":unix:"+self.sockname)]

