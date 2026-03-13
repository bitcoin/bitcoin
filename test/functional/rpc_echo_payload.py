#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Ensure echo with a payload will either be rejected or handled, but will never time out."""

from concurrent.futures import ThreadPoolExecutor
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, JSONRPCException
import random


class RpcSlaTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-rpcworkqueue=2", "-rpcthreads=2"]]
        # enough to possibly fill the running threads as well as the queue:
        self.num_threads = 6

    def run_test(self):
        node = self.nodes[0]
        data = random.randbytes(1_999_000).hex()

        def check_results(rpc):
            self.log.info("Starting thread ...")
            for i in range(200):
                payload = data[: random.randrange(0, len(data))]
                try:
                    if random.getrandbits(1):
                        assert_equal(payload, rpc.echo(payload)[0])
                    else:
                        rpc.sendrawtransaction(payload)
                except JSONRPCException as e:
                    msg = e.error["message"]
                    if msg not in [
                        "TX decode failed. Make sure the tx has at least one input.",
                        "non-JSON HTTP response with '503 Service Unavailable' from server: Work queue depth exceeded",
                    ]:
                        raise AssertionError(f"Unexpected msg: {msg}")

        rpcs = [node.create_new_rpc_connection() for _ in range(self.num_threads)]
        self.log.info("Starting threadpool ...")
        with ThreadPoolExecutor(max_workers=len(rpcs)) as threads:
            list(threads.map(check_results, rpcs))


if __name__ == "__main__":
    RpcSlaTest(__file__).main()
