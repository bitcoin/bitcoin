#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Utilities for doing coverage analysis on the RPC interface.

Provides a way to track which RPC commands are exercised during
testing.
"""

import os


REFERENCE_FILENAME = 'rpc_interface.txt'


class AuthServiceProxyWrapper():
    """
    An object that wraps AuthServiceProxy to record specific RPC calls.

    """
    def __init__(self, auth_service_proxy_instance, coverage_logfile=None):
        """
        Kwargs:
            auth_service_proxy_instance (AuthServiceProxy): the instance
                being wrapped.
            coverage_logfile (str): if specified, write each service_name
                out to a file when called.

        """
        self.auth_service_proxy_instance = auth_service_proxy_instance
        self.coverage_logfile = coverage_logfile

    def __getattr__(self, name):
        return_val = getattr(self.auth_service_proxy_instance, name)
        if not isinstance(return_val, type(self.auth_service_proxy_instance)):
            # If proxy getattr returned an unwrapped value, do the same here.
            return return_val
        return AuthServiceProxyWrapper(return_val, self.coverage_logfile)

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
            with open(self.coverage_logfile, 'a+', encoding='utf8') as f:
                f.write("%s\n" % rpc_method)

    def __truediv__(self, relative_uri):
        return AuthServiceProxyWrapper(self.auth_service_proxy_instance / relative_uri,
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


def write_all_rpc_commands(dirname, node):
    """
    Write out a list of all RPC functions available in `litecoin-cli` for
    coverage comparison. This will only happen once per coverage
    directory.

    Args:
        dirname (str): temporary test dir
        node (AuthServiceProxy): client

    Returns:
        bool. if the RPC interface file was written.

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

    with open(filename, 'w', encoding='utf8') as f:
        f.writelines(list(commands))

    return True
