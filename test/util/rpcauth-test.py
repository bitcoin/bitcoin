#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test share/rpcauth/rpcauth.py
"""
import base64
import configparser
import hmac
import importlib
import os
import sys
import unittest

posix2windows = lambda filename: filename

class TestRPCAuth(unittest.TestCase):
    def setUp(self):
        config = configparser.ConfigParser()
        directory = os.path.abspath(os.path.dirname(__file__))
        config_path = os.path.abspath(os.path.join(os.sep, directory, "../config.ini"))
        with open(config_path, encoding="utf8") as config_file:
            config.read_file(config_file)

        sys.path.insert(0, directory)

        try:
            importlib.invalidate_caches()
            path_helper = importlib.import_module('path_helper')
            posix2windows = path_helper.posix2windows
        except Exception as e:
            print(sys.path)

        importlib.invalidate_caches()
        path_helper = importlib.import_module('path_helper')

        rpcauth_dir = posix2windows(config['environment']['RPCAUTH'])
        rpcauth_dir = os.path.dirname(rpcauth_dir)
        rpcauth_dir = os.path.abspath(rpcauth_dir)

        sys.path.insert(0, rpcauth_dir)
        self.rpcauth = importlib.import_module('rpcauth')

    def test_generate_salt(self):
        for i in range(16, 32 + 1):
            self.assertEqual(len(self.rpcauth.generate_salt(i)), i * 2)

    def test_generate_password(self):
        password = self.rpcauth.generate_password()
        expected_password = base64.urlsafe_b64encode(
            base64.urlsafe_b64decode(password)).decode('utf-8')
        self.assertEqual(expected_password, password)

    def test_check_password_hmac(self):
        salt = self.rpcauth.generate_salt(16)
        password = self.rpcauth.generate_password()
        password_hmac = self.rpcauth.password_to_hmac(salt, password)

        m = hmac.new(bytearray(salt, 'utf-8'),
            bytearray(password, 'utf-8'), 'SHA256')
        expected_password_hmac = m.hexdigest()

        self.assertEqual(expected_password_hmac, password_hmac)

if __name__ == '__main__':
    unittest.main()
