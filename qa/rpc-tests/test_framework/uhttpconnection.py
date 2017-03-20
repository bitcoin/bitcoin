#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Inspired by "HTTP on Unix sockets with Python"
# From: http://7bits.nl/blog/posts/http-on-unix-sockets-with-python
import http.client
import socket

def have_af_unix():
    '''Return True if UNIX sockets are available on this platform.'''
    try:
        socket.AF_UNIX
    except AttributeError:
        return False
    else:
        return True

class UHTTPConnection(http.client.HTTPConnection):
    """Subclass of Python library HTTPConnection that
       uses a unix-domain socket.
    """

    def __init__(self, path):
        http.client.HTTPConnection.__init__(self, 'localhost')
        self.path = path

    def connect(self):
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(self.path)
        self.sock = sock

