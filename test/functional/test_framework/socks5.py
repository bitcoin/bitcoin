#!/usr/bin/env python3
# Copyright (c) 2015-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Dummy Socks5 server for testing."""

import select
import socket
import threading
import queue
import logging

from .netutil import (
    format_addr_port
)

logger = logging.getLogger("TestFramework.socks5")

# Protocol constants
class Command:
    CONNECT = 0x01

class AddressType:
    IPV4 = 0x01
    DOMAINNAME = 0x03
    IPV6 = 0x04

# Utility functions
def recvall(s, n):
    """Receive n bytes from a socket, or fail."""
    rv = bytearray()
    while n > 0:
        d = s.recv(n)
        if not d:
            raise IOError('Unexpected end of stream')
        rv.extend(d)
        n -= len(d)
    return rv

def sendall(s, data):
    """Send all data to a socket, or fail."""
    sent = 0
    while sent < len(data):
        _, wlist, _ = select.select([], [s], [])
        if len(wlist) > 0:
            n = s.send(data[sent:])
            if n == 0:
                raise IOError('send() on socket returned 0')
            sent += n

def forward_sockets(a, b):
    """Forward data received on socket a to socket b and vice versa, until EOF is received on one of the sockets."""
    # Mark as non-blocking so that we do not end up in a deadlock-like situation
    # where we block and wait on data from `a` while there is data ready to be
    # received on `b` and forwarded to `a`. And at the same time the application
    # at `a` is not sending anything because it waits for the data from `b` to
    # respond.
    a.setblocking(False)
    b.setblocking(False)
    sockets = [a, b]
    done = False
    while not done:
        rlist, _, xlist = select.select(sockets, [], sockets)
        if len(xlist) > 0:
            raise IOError('Exceptional condition on socket')
        for s in rlist:
            data = s.recv(4096)
            if data is None or len(data) == 0:
                done = True
                break
            if s == a:
                sendall(b, data)
            else:
                sendall(a, data)

# Implementation classes
class Socks5Configuration():
    """Proxy configuration."""
    def __init__(self):
        self.addr = None # Bind address (must be set)
        self.af = socket.AF_INET # Bind address family
        self.unauth = False  # Support unauthenticated
        self.auth = False  # Support authentication
        self.keep_alive = False  # Do not automatically close connections
        # This function is called whenever a new connection arrives to the proxy
        # and it decides where the connection is redirected to. It is passed:
        # - the address the client requested to connect to
        # - the port the client requested to connect to
        # It is supposed to return an object like:
        # {
        #     "actual_to_addr": "127.0.0.1"
        #     "actual_to_port": 28276
        # }
        # or None.
        # If it returns an object then the connection is redirected to actual_to_addr:actual_to_port.
        # If it returns None, or destinations_factory itself is None then the connection is closed.
        self.destinations_factory = None

class Socks5Command():
    """Information about an incoming socks5 command."""
    def __init__(self, cmd, atyp, addr, port, username, password):
        self.cmd = cmd # Command (one of Command.*)
        self.atyp = atyp # Address type (one of AddressType.*)
        self.addr = addr # Address
        self.port = port # Port to connect to
        self.username = username
        self.password = password
    def __repr__(self):
        return 'Socks5Command(%s,%s,%s,%s,%s,%s)' % (self.cmd, self.atyp, self.addr, self.port, self.username, self.password)

class Socks5Connection():
    def __init__(self, serv, conn):
        self.serv = serv
        self.conn = conn

    def handle(self):
        """Handle socks5 request according to RFC1928."""
        try:
            # Verify socks version
            ver = recvall(self.conn, 1)[0]
            if ver != 0x05:
                raise IOError('Invalid socks version %i' % ver)
            # Choose authentication method
            nmethods = recvall(self.conn, 1)[0]
            methods = bytearray(recvall(self.conn, nmethods))
            method = None
            if 0x02 in methods and self.serv.conf.auth:
                method = 0x02 # username/password
            elif 0x00 in methods and self.serv.conf.unauth:
                method = 0x00 # unauthenticated
            if method is None:
                raise IOError('No supported authentication method was offered')
            # Send response
            self.conn.sendall(bytearray([0x05, method]))
            # Read authentication (optional)
            username = None
            password = None
            if method == 0x02:
                ver = recvall(self.conn, 1)[0]
                if ver != 0x01:
                    raise IOError('Invalid auth packet version %i' % ver)
                ulen = recvall(self.conn, 1)[0]
                username = str(recvall(self.conn, ulen))
                plen = recvall(self.conn, 1)[0]
                password = str(recvall(self.conn, plen))
                # Send authentication response
                self.conn.sendall(bytearray([0x01, 0x00]))

            # Read connect request
            ver, cmd, _, atyp = recvall(self.conn, 4)
            if ver != 0x05:
                raise IOError('Invalid socks version %i in connect request' % ver)
            if cmd != Command.CONNECT:
                raise IOError('Unhandled command %i in connect request' % cmd)

            if atyp == AddressType.IPV4:
                addr = recvall(self.conn, 4)
            elif atyp == AddressType.DOMAINNAME:
                n = recvall(self.conn, 1)[0]
                addr = recvall(self.conn, n)
            elif atyp == AddressType.IPV6:
                addr = recvall(self.conn, 16)
            else:
                raise IOError('Unknown address type %i' % atyp)
            port_hi,port_lo = recvall(self.conn, 2)
            port = (port_hi << 8) | port_lo

            # Send dummy response
            self.conn.sendall(bytearray([0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]))

            cmdin = Socks5Command(cmd, atyp, addr, port, username, password)
            self.serv.queue.put(cmdin)
            logger.debug('Proxy: %s', cmdin)

            requested_to_addr = addr.decode("utf-8")
            requested_to = format_addr_port(requested_to_addr, port)

            if self.serv.conf.destinations_factory is not None:
                dest = self.serv.conf.destinations_factory(requested_to_addr, port)
                if dest is not None:
                    logger.debug(f"Serving connection to {requested_to}, will redirect it to "
                                 f"{dest['actual_to_addr']}:{dest['actual_to_port']} instead")
                    with socket.create_connection((dest["actual_to_addr"], dest["actual_to_port"])) as conn_to:
                        forward_sockets(self.conn, conn_to)
                else:
                    logger.debug(f"Can't serve the connection to {requested_to}: the destinations factory returned None")
            else:
                logger.debug(f"Can't serve the connection to {requested_to}: no destinations factory")

            # Fall through to disconnect
        except Exception as e:
            logger.exception("socks5 request handling failed.")
            self.serv.queue.put(e)
        finally:
            if not self.serv.keep_alive:
                self.conn.close()
            else:
                logger.debug("Keeping client connection alive")

class Socks5Server():
    def __init__(self, conf):
        self.conf = conf
        self.s = socket.socket(conf.af)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s.bind(conf.addr)
        self.s.listen(5)
        self.running = False
        self.thread = None
        self.queue = queue.Queue() # report connections and exceptions to client
        self.keep_alive = conf.keep_alive

    def run(self):
        while self.running:
            (sockconn, _) = self.s.accept()
            if self.running:
                conn = Socks5Connection(self, sockconn)
                thread = threading.Thread(None, conn.handle)
                thread.daemon = True
                thread.start()

    def start(self):
        assert not self.running
        self.running = True
        self.thread = threading.Thread(None, self.run)
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        self.running = False
        # connect to self to end run loop
        s = socket.socket(self.conf.af)
        s.connect(self.conf.addr)
        s.close()
        self.thread.join()
