#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that bitcoin-gui starts up and can be stopped via RPC."""

from test_framework.test_framework import (
    BitcoinTestFramework,
    SkipTest,
)


class GuiTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-server"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_gui()
        if self.is_qt_vcpkg():
            # vcpkg builds Qt with -opengl dynamic, making the "minimal"
            # platform plugin unusable due to internal Qt bugs. This matches
            # the VCPKG_TARGET_TRIPLET condition in src/qt/test/CMakeLists.txt
            # that sets QT_QPA_PLATFORM=windows for test_bitcoin-qt.
            raise SkipTest("minimal Qt platform plugin unusable with vcpkg Qt")

    def setup_nodes(self):
        self.extra_init = [{"use_gui": True}]
        super().setup_nodes()

    def run_test(self):
        self.log.info("Test that bitcoin-gui starts up and can be stopped via RPC")
        self.stop_node(0)


if __name__ == "__main__":
    GuiTest(__file__).main()
