#!/usr/bin/env python3
# Copyright (c) 2018-2019 The XBit Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests the includeconf argument

Verify that:

1. adding includeconf to the configuration file causes the includeconf
   file to be loaded in the correct order.
2. includeconf cannot be used as a command line argument.
3. includeconf cannot be used recursively (ie includeconf can only
   be used from the base config file).
4. multiple includeconf arguments can be specified in the main config
   file.
"""
import os

from test_framework.test_framework import XBitTestFramework

class IncludeConfTest(XBitTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def setup_chain(self):
        super().setup_chain()
        # Create additional config files
        # - tmpdir/node0/relative.conf
        with open(os.path.join(self.options.tmpdir, "node0", "relative.conf"), "w", encoding="utf8") as f:
            f.write("uacomment=relative\n")
        # - tmpdir/node0/relative2.conf
        with open(os.path.join(self.options.tmpdir, "node0", "relative2.conf"), "w", encoding="utf8") as f:
            f.write("uacomment=relative2\n")
        with open(os.path.join(self.options.tmpdir, "node0", "xbit.conf"), "a", encoding='utf8') as f:
            f.write("uacomment=main\nincludeconf=relative.conf\n")

    def run_test(self):
        self.log.info("-includeconf works from config file. subversion should end with 'main; relative)/'")

        subversion = self.nodes[0].getnetworkinfo()["subversion"]
        assert subversion.endswith("main; relative)/")

        self.log.info("-includeconf cannot be used as command-line arg")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(extra_args=["-includeconf=relative2.conf"], expected_msg="Error: Error parsing command line arguments: -includeconf cannot be used from commandline; -includeconf=relative2.conf")

        self.log.info("-includeconf cannot be used recursively. subversion should end with 'main; relative)/'")
        with open(os.path.join(self.options.tmpdir, "node0", "relative.conf"), "a", encoding="utf8") as f:
            f.write("includeconf=relative2.conf\n")
        self.start_node(0)

        subversion = self.nodes[0].getnetworkinfo()["subversion"]
        assert subversion.endswith("main; relative)/")
        self.stop_node(0, expected_stderr="warning: -includeconf cannot be used from included files; ignoring -includeconf=relative2.conf")

        self.log.info("-includeconf cannot contain invalid arg")

        # Commented out as long as we ignore invalid arguments in configuration files
        #with open(os.path.join(self.options.tmpdir, "node0", "relative.conf"), "w", encoding="utf8") as f:
        #    f.write("foo=bar\n")
        #self.nodes[0].assert_start_raises_init_error(expected_msg="Error: Error reading configuration file: Invalid configuration value foo")

        self.log.info("-includeconf cannot be invalid path")
        os.remove(os.path.join(self.options.tmpdir, "node0", "relative.conf"))
        self.nodes[0].assert_start_raises_init_error(expected_msg="Error: Error reading configuration file: Failed to include configuration file relative.conf")

        self.log.info("multiple -includeconf args can be used from the base config file. subversion should end with 'main; relative; relative2)/'")
        with open(os.path.join(self.options.tmpdir, "node0", "relative.conf"), "w", encoding="utf8") as f:
            # Restore initial file contents
            f.write("uacomment=relative\n")

        with open(os.path.join(self.options.tmpdir, "node0", "xbit.conf"), "a", encoding='utf8') as f:
            f.write("includeconf=relative2.conf\n")

        self.start_node(0)

        subversion = self.nodes[0].getnetworkinfo()["subversion"]
        assert subversion.endswith("main; relative; relative2)/")

if __name__ == '__main__':
    IncludeConfTest().main()
