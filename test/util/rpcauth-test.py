#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test share/rpcauth/rpcauth.py
"""
import re
import configparser
import hmac
import importlib
import os
import sys
import unittest

class TestRPCAuth(unittest.TestCase):
    def setUp(self):
        config = configparser.ConfigParser()
        config_path = os.path.abspath(
            os.path.join(os.sep, os.path.abspath(os.path.dirname(__file__)),
            "../config.ini"))
        with open(config_path, encoding="utf8") as config_file:
            config.read_file(config_file)
        sys.path.insert(0, os.path.dirname(config['environment']['RPCAUTH']))
        self.rpcauth = importlib.import_module('rpcauth')

    def test_generate_salt(self):
        for i in range(16, 32 + 1):
            self.assertEqual(len(self.rpcauth.generate_salt(i)), i * 2)

    def test_generate_password(self):
        """Test that generated passwords only consist of urlsafe characters."""
        r = re.compile(r"[0-9a-zA-Z_-]*")
        password = self.rpcauth.generate_password()
        self.assertTrue(r.fullmatch(password))

    def test_check_password_hmac(self):
        salt = self.rpcauth.generate_salt(16)
        password = self.rpcauth.generate_password()
        password_hmac = self.rpcauth.password_to_hmac(salt, password)

        m = hmac.new(salt.encode('utf-8'), password.encode('utf-8'), 'SHA256')
        expected_password_hmac = m.hexdigest()

        self.assertEqual(expected_password_hmac, password_hmac)

if __name__ == '__main__':
    unittest.main()
