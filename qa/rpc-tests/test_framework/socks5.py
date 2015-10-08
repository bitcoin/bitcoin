# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Dummy Socks5 server for testing.
'''
from __future__ import print_function, division, unicode_literals
import socket, threading, Queue
import traceback, sys

### Protocol constants
class Command:
    CONNECT = 0x01

class AddressType:
    IPV4 = 0x01
    DOMAINNAME = 0x03
    IPV6 = 0x04

### Utility functions
def recvall(s, n):
    '''Receive n bytes from a socket, or fail'''
    rv = bytearray()
    while n > 0:
        d = s.recv(n)
        if not d:
            raise IOError('Unexpected end of stream')
        rv.extend(d)
        n -= len(d)
    return rv

### Implementation classes
class Socks5Configuration(object):
    '''Proxy configuration'''
    def __init__(self):
        self.addr = None # Bind address (must be set)
        self.af = socket.AF_INET # Bind address family
        self.unauth = False  # Support unauthenticated
        self.auth = False  # Support authentication

class Socks5Command(object):
    '''Information about an incoming socks5 command'''
    def __init__(self, cmd, atyp, addr, port, username, password):
        self.cmd = cmd # Command (one of Command.*)
        self.atyp = atyp # Address type (one of AddressType.*)
        self.addr = addr # Address
        self.port = port # Port to connect to
        self.username = username
        self.password = password
    def __repr__(self):
        return 'Socks5Command(%s,%s,%s,%s,%s,%s)' % (self.cmd, self.atyp, self.addr, self.port, self.username, self.password)

class Socks5Connection(object):
    def __init__(self, serv, conn, peer):
        self.serv = serv
        self.conn = conn
        self.peer = peer

    def handle(self):
        '''
        Handle socks5 request according to RFC1928
        '''
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
            (ver,cmd,rsv,atyp) = recvall(self.conn, 4)
            if ver != 0x05:
                raise IOError('Invalid socks version %i in connect request' % ver)
            if cmd != Command.CONNECT:
                raise IOError('Unhandled command %i in connect request' % cmd)

            if atyp == AddressType.IPV4:
                addr = recvall(self.conn, 4)
            elif atyp == AddressType.DOMAINNAME:
                n = recvall(self.conn, 1)[0]
                addr = str(recvall(self.conn, n))
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
            print('Proxy: ', cmdin)
            # Fall through to disconnect
        except Exception,e:
            traceback.print_exc(file=sys.stderr)
            self.serv.queue.put(e)
        finally:
            self.conn.close()

class Socks5Server(object):
    def __init__(self, conf):
        self.conf = conf
        self.s = socket.socket(conf.af)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s.bind(conf.addr)
        self.s.listen(5)
        self.running = False
        self.thread = None
        self.queue = Queue.Queue() # report connections and exceptions to client

    def run(self):
        while self.running:
            (sockconn, peer) = self.s.accept()
            if self.running:
                conn = Socks5Connection(self, sockconn, peer)
                thread = threading.Thread(None, conn.handle)
                thread.daemon = True
                thread.start()
    
    def start(self):
        assert(not self.running)
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

