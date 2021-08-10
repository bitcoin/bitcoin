#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
A basic TCP server that responds to requests based on a given/custom criteria.
"""

import socket
import threading


class BasicServer:
    def __init__(self, *, bind, response_generator):
        """
        :param bind: Listen address-port tuple, for example ("127.0.0.1", 80).
        If the port is 0 then a random available one will be chosen and it
        can be found later in the listen_port property.

        :param response_generator: A callback function, supplied with the
        request. It returns a reply that is to be send to the client either as
        a string or as an array which is joined into a single string. If it
        returns None, then the connection is closed by the server.
        """
        # create_server() is only in Python 3.8. Replace the below 4 lines once only >=3.8 is supported.
        # https://docs.python.org/3/library/socket.html#socket.create_server
        # self.listen_socket = socket.create_server(address=bind, reuse_port=True)
        self.listen_socket = socket.socket(socket.AF_INET)
        self.listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listen_socket.bind(bind)
        self.listen_socket.listen()

        self.listen_port = self.listen_socket.getsockname()[1]

        self.response_generator = response_generator
        self.accept_loop_thread = threading.Thread(target=self.accept_loop, name="accept loop", daemon=True)
        self.accept_loop_thread.start()

    def accept_loop(self):
        try:
            while True:
                (accepted_socket, _) = self.listen_socket.accept()
                t = threading.Thread(target=self.process_incoming, args=(accepted_socket,), name="process incoming", daemon=True)
                t.start()
        except:
            # The accept() method raises when listen_socket is closed from the destructor.
            pass

    def process_incoming(self, accepted_socket):
        while True:
            request = accepted_socket.makefile().readline()
            response = self.response_generator(request=request.rstrip())
            response_separator = '\r\n'
            if response is None:
                # End of communication.
                accepted_socket.close()
                break
            elif isinstance(response, list):
                response_str = response_separator.join(response)
            else:
                response_str = response
            response_str += response_separator

            totalsent = 0
            while totalsent < len(response_str):
                sent = accepted_socket.send(bytes(response_str[totalsent:], 'utf-8'))
                if sent == 0:
                    accepted_socket.close()
                    raise RuntimeError("connection broken: socket.send() returned 0")
                totalsent += sent

    def __del__(self):
        self.listen_socket.close()
        self.accept_loop_thread.join()


def tor_control(*, request):
    """
    https://gitweb.torproject.org/torspec.git/tree/control-spec.txt
    """
    if request == 'PROTOCOLINFO 1':
        return ['250-PROTOCOLINFO 1',
                '250-AUTH METHODS=NULL',
                '250 OK'
               ]
    elif request == 'AUTHENTICATE':
        return '250 OK'
    elif request.startswith('ADD_ONION '):
        return ['250-ServiceID=2g5qfdkn2vvcbqhzcyvyiitg4ceukybxklraxjnu7atlhd22gdwywaid',
                '250-PrivateKey=123456',
                '250 OK',
               ]
    else:
        return None
