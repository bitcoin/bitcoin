#!/usr/bin/env python3
# Copyright (c) 2015-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Utilities for doing coverage analysis on the RPC interface.

Provides a way to track which RPC commands are exercised during
testing.
"""

import json
import os
import unittest
from typing import Optional

from .authproxy import AuthServiceProxy

REFERENCE_FILENAME = 'rpc_interface.txt'


class AuthServiceProxyWrapper():
    """
    An object that wraps AuthServiceProxy to record specific RPC calls.

    """
    def __init__(self, auth_service_proxy_instance: AuthServiceProxy, rpc_url: str, coverage_logfile: Optional[str]=None):
        """
        Kwargs:
            auth_service_proxy_instance: the instance being wrapped.
            rpc_url: url of the RPC instance being wrapped
            coverage_logfile: if specified, write each service_name
                out to a file when called.

        """
        self.auth_service_proxy_instance = auth_service_proxy_instance
        self.rpc_url = rpc_url
        self.coverage_logfile = coverage_logfile

    @property
    def ensure_ascii(self):
        return self.auth_service_proxy_instance.ensure_ascii

    @ensure_ascii.setter
    def ensure_ascii(self, value):
        self.auth_service_proxy_instance.ensure_ascii = value

    def __getattr__(self, name):
        return_val = getattr(self.auth_service_proxy_instance, name)
        if not isinstance(return_val, type(self.auth_service_proxy_instance)):
            # If proxy getattr returned an unwrapped value, do the same here.
            return return_val
        return AuthServiceProxyWrapper(return_val, self.rpc_url, self.coverage_logfile)

    def __call__(self, *args, **kwargs):
        """
        Delegates to AuthServiceProxy, then writes the particular RPC method
        called to a file.

        """
        return_val = self.auth_service_proxy_instance.__call__(*args, **kwargs)
        self._log_call()
        return return_val

    def _log_call(self):
        rpc_method = self.auth_service_proxy_instance._service_name

        if self.coverage_logfile:
            with open(self.coverage_logfile, 'a+') as f:
                f.write("%s\n" % rpc_method)

    def __truediv__(self, relative_uri):
        return AuthServiceProxyWrapper(self.auth_service_proxy_instance / relative_uri,
                                       self.rpc_url,
                                       self.coverage_logfile)

    def get_request(self, *args, **kwargs):
        self._log_call()
        return self.auth_service_proxy_instance.get_request(*args, **kwargs)


def get_filename(dirname, n_node):
    """
    Get a filename unique to the test process ID and node.

    This file will contain a list of RPC commands covered.
    """
    pid = str(os.getpid())
    return os.path.join(
        dirname, "coverage.pid%s.node%s.txt" % (pid, str(n_node)))


def write_all_rpc_commands(dirname: str, node: AuthServiceProxy) -> bool:
    """
    Write out a list of all RPC functions available in `bitcoin-cli` for
    coverage comparison. This will only happen once per coverage
    directory.

    Args:
        dirname: temporary test dir
        node: client

    Returns:
        if the RPC interface file was written.

    """
    filename = os.path.join(dirname, REFERENCE_FILENAME)

    if os.path.isfile(filename):
        return False

    help_output = node.help().split('\n')
    commands = set()

    for line in help_output:
        line = line.strip()

        # Ignore blanks and headers
        if line and not line.startswith('='):
            commands.add("%s\n" % line.split()[0])

    with open(filename, 'w') as f:
        f.writelines(list(commands))

    return True


class TestAuthServiceProxyWrapper(unittest.TestCase):
    class HTTPConnection:
        timeout = 60

        def __init__(self):
            self.postdata = None

        def request(self, method, path, postdata, headers):
            self.postdata = postdata

        def getresponse(self):
            return self

        status = 200
        reason = "OK"

        def getheader(self, name):
            return "application/json"

        def read(self):
            return b'{"jsonrpc":"2.0","result":null,"id":1}'

    def test_ensure_ascii(self):
        connection = self.HTTPConnection()
        proxy = AuthServiceProxyWrapper(
            AuthServiceProxy("http://user:pass@localhost", connection=connection),
            "http://user:pass@localhost",
        )

        for ensure_ascii in [True, False]:
            proxy.ensure_ascii = ensure_ascii
            self.assertEqual(proxy.auth_service_proxy_instance.ensure_ascii, ensure_ascii)

            for endpoint in [proxy, proxy / "wallet/test"]:
                for text in ["рыба", "𝅘𝅥𝅯"]:
                    endpoint.test(text)
                    escaped = json.dumps(text)[1:-1].encode()
                    utf8 = text.encode()
                    if ensure_ascii:
                        self.assertIn(escaped, connection.postdata)
                        self.assertNotIn(utf8, connection.postdata)
                    else:
                        self.assertIn(utf8, connection.postdata)
                        self.assertNotIn(escaped, connection.postdata)
