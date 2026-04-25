#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""RPC connection helpers for the functional test framework."""
from typing import Optional

from .authproxy import AuthServiceProxy
from .coverage import AuthServiceProxyWrapper, get_filename

def get_rpc_proxy(url: str, node_number: int, *, timeout: Optional[int] = None, coveragedir: Optional[str] = None) -> AuthServiceProxyWrapper:
    """
    Args:
        url: URL of the RPC server to call
        node_number: the node number (or id) that this calls to

    Kwargs:
        timeout: HTTP timeout in seconds
        coveragedir: Directory

    Returns:
        AuthServiceProxy. convenience object for making RPC calls.

    """
    proxy_kwargs = {}
    if timeout is not None:
        proxy_kwargs['timeout'] = int(timeout)

    proxy = AuthServiceProxy(url, **proxy_kwargs)

    coverage_logfile = get_filename(coveragedir, node_number) if coveragedir else None

    return AuthServiceProxyWrapper(proxy, url, coverage_logfile)
