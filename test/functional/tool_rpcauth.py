#!/usr/bin/env python3
# Copyright (c) 2015-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test share/rpcauth/rpcauth.py
"""
import hmac
import importlib
import json
import os
import re
import sys
from io import StringIO
from unittest.mock import patch

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class RpcAuthTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 0  # No node/datadir needed

    def setup_network(self):
        pass

    def setUp(self):
        sys.path.insert(0, os.path.dirname(self.config["environment"]["RPCAUTH"]))
        self.rpcauth = importlib.import_module('rpcauth')

    def run_test(self):
        self.setUp()

        self.test_generate_salt()
        self.test_generate_password()
        self.test_check_password_hmac()
        self.test_cli_output()

    def test_generate_salt(self):
        for i in range(16, 32 + 1):
            assert_equal(len(self.rpcauth.generate_salt(i)), i * 2)

    def test_generate_password(self):
        """Test that generated passwords only consist of urlsafe characters."""
        r = re.compile(r"[0-9a-zA-Z_-]*")
        password = self.rpcauth.generate_password()
        assert r.fullmatch(password)

    def test_check_password_hmac(self):
        salt = self.rpcauth.generate_salt(16)
        password = self.rpcauth.generate_password()
        password_hmac = self.rpcauth.password_to_hmac(salt, password)

        m = hmac.new(salt.encode('utf-8'), password.encode('utf-8'), 'SHA256')
        expected_password_hmac = m.hexdigest()

        assert_equal(expected_password_hmac, password_hmac)

    def test_cli_output(self):
        generated_password = "generated_password_for_test"
        with patch.object(sys, "argv", ["rpcauth.py", "generated-user"]), \
                patch.object(self.rpcauth, "generate_password", return_value=generated_password), \
                patch.object(sys, "stdout", new=StringIO()) as generated_output:
            self.rpcauth.main()
        assert generated_password in generated_output.getvalue()

        supplied_password = "supplied_password_must_not_be_echoed"
        with patch.object(sys, "argv", ["rpcauth.py", "supplied-user", supplied_password]), \
                patch.object(sys, "stdout", new=StringIO()) as supplied_output:
            self.rpcauth.main()
        assert supplied_password not in supplied_output.getvalue()

        prompted_password = "prompted_password_must_not_be_echoed"
        with patch.object(sys, "argv", ["rpcauth.py", "prompted-user", "-"]), \
                patch.object(self.rpcauth, "getpass", return_value=prompted_password), \
                patch.object(sys, "stdout", new=StringIO()) as prompted_output:
            self.rpcauth.main()
        assert prompted_password not in prompted_output.getvalue()

        with patch.object(sys, "argv", ["rpcauth.py", "--json", "json-user", supplied_password]), \
                patch.object(sys, "stdout", new=StringIO()) as json_output:
            self.rpcauth.main()
        json_result = json.loads(json_output.getvalue())
        assert_equal(json_result["username"], "json-user")
        assert_equal(json_result["password"], supplied_password)
        assert json_result["rpcauth"].startswith("json-user:")


if __name__ == '__main__':
    RpcAuthTest(__file__).main()
