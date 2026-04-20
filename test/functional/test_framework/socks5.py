#!/usr/bin/env python3
# Copyright (c) 2015-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Dummy Socks5 server for testing."""

import select
import socket
import threading
import queue
import logging

from .netutil import (
    format_addr_port,
    set_ephemeral_port_range,
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

def forward_sockets(a, b, wakeup_socket, serv):
    """Forwards data between sockets a and b until EOF, error, or shutdown.

    Monitors wakeup_socket for a shutdown signal and checks serv.is_running()
    to exit gracefully when the server is stopping.
    """
    # Mark as non-blocking so that we do not end up in a deadlock-like situation
    # where we block and wait on data from `a` while there is data ready to be
    # received on `b` and forwarded to `a`. And at the same time the application
    # at `a` is not sending anything because it waits for the data from `b` to
    # respond.
    a.setblocking(False)
    b.setblocking(False)
    sockets = [a, b, wakeup_socket]
    done = False
    while not done:
        # Blocking select with timeout
        rlist, _, xlist = select.select(sockets, [], sockets, 2)
        if not serv.is_running():
            logger.debug("forward_sockets: Exit due to shutdown")
            return
        if len(xlist) > 0:
            raise IOError('Exceptional condition on socket')
        for s in rlist:
            data = s.recv(4096)
            if data is None or len(data) == 0:
                done = True
                break
            if s == a:
                sendall(b, data)
            elif s == b:
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
        # Socket-pair used to wake up blocking forwarding select
        # Note: a pipe could be used as well, but that does not work with select() on Windows
        self.wakeup_socket_pair = socket.socketpair()
        # Index of this handler (within the server)
        self.handler_index = None

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

            if self.serv.is_running():
                if self.serv.conf.destinations_factory is not None:
                    dest = self.serv.conf.destinations_factory(requested_to_addr, port)
                    if dest is not None:
                        logger.debug(f"Serving connection to {requested_to}, will redirect it to "
                                    f"{dest['actual_to_addr']}:{dest['actual_to_port']} instead")
                        with socket.create_connection((dest["actual_to_addr"], dest["actual_to_port"])) as conn_to:
                            forward_sockets(self.conn, conn_to, self.wakeup_socket_pair[1], self.serv)
                            conn_to.close()
                    else:
                        logger.debug(f"Can't serve the connection to {requested_to}: the destinations factory returned None")
                else:
                    logger.debug(f"Can't serve the connection to {requested_to}: no destinations factory")

            # Fall through to disconnect
        except Exception as e:
            logger.exception(f"socks5 request handling failed (running {self.serv.is_running()})")
            if self.serv.is_running():
                self.serv.queue.put(e)
        finally:
            if not self.serv.keep_alive:
                self.conn.close()
            else:
                logger.debug("Keeping client connection alive")
            s0 = self.wakeup_socket_pair[0]
            s1 = self.wakeup_socket_pair[1]
            self.wakeup_socket_pair = None
            try:
                s0.close()
                s1.close()
            except OSError:
                pass
            self.serv.remove_handler(self.handler_index)
            self.handler_index = None

    def wakeup(self):
        # Wake up the blocking forwarding select by writing to the wake-up socket
        try:
            socket_pair = self.wakeup_socket_pair
            if socket_pair is not None:
                socket_pair[0].send("CloseWakeup".encode())
                logger.debug("Waking up forwarding thread")
        except OSError as e:
            logger.warning(f"Error waking up forwarding thread: {e}")
            pass


# Wrapper for thread.join(), which may throw for daemon threads (in late stages of finalization).
# Return True if the thread is no longer active (join succeeded), False otherwise
# See PR #34863 for more details on using daemon threads.
def try_join_daemon_thread(thread, timeout=0) -> bool:
    try:
        thread.join(timeout=timeout)
        return not thread.is_alive()
    except Exception as e:
        logger.debug(f"Exception in thread.join, {e}")
        return True

class Socks5Server():
    def __init__(self, conf):
        self.conf = conf
        self.s = socket.socket(conf.af)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # When using dynamic port allocation (port=0), ensure we don't get a
        # port that conflicts with the test framework's static port range.
        if conf.addr[1] == 0:
            set_ephemeral_port_range(self.s)
        self.s.bind(conf.addr)
        # When port=0, the OS assigns an available port. Update conf.addr
        # to reflect the actual bound address so callers can use it.
        self.conf.addr = self.s.getsockname()
        self.s.listen(5)
        # Set to False when stop is initiated
        self._running = False
        self._running_lock = threading.Lock()
        self.thread = None
        self.queue = queue.Queue() # report connections and exceptions to client
        self.keep_alive = conf.keep_alive
        # Store the background handlers, needed for clean shutdown
        # Append-only array, completed handlers are set to None
        self._handlers = []
        self._handlers_lock = threading.Lock()

    def is_running(self) -> bool:
        with self._running_lock:
            return self._running

    def set_running(self, new_value: bool):
        with self._running_lock:
            self._running = new_value

    def run(self):
        while self.is_running():
            (sockconn, _) = self.s.accept()
            if self.is_running():
                conn = Socks5Connection(self, sockconn)
                # Use "daemon" threads, see PR #34863 for more discussion.
                thread = threading.Thread(None, conn.handle, daemon=True)
                with self._handlers_lock:
                    conn.handler_index = len(self._handlers)
                    self._handlers.append((thread, conn))
                    assert(conn.handler_index < len(self._handlers))
                thread.start()

    def remove_handler(self, handler_index):
        with self._handlers_lock:
            if handler_index < len(self._handlers):
                if self._handlers[handler_index] is not None:
                    self._handlers[handler_index] = None
                    logger.debug(f"Handler {handler_index} removed")

    def start(self):
        assert not self.is_running()
        self.set_running(True)
        self.thread = threading.Thread(None, self.run, daemon=True)
        self.thread.start()

    def stop(self):
        self.set_running(False)
        # connect to self to end run loop
        s = socket.socket(self.conf.af)
        s.connect(self.conf.addr)
        s.close()
        self.thread.join()
        # if there are active handlers, close them
        with self._handlers_lock:
            items = list(self._handlers)
        for i, item in enumerate(items):
            if item is None:
                continue
            thread, conn = item
            # check if thread is still active
            if not try_join_daemon_thread(thread, timeout=0):
                conn.wakeup()
                if try_join_daemon_thread(thread, timeout=2):
                    logger.debug(f"Stop(): Handler {i} thread joined")
                else:
                    logger.warning(f"Stop(): Handler thread {i} didn't finish after force close")
