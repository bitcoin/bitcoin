#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys
import os
import base64
from binascii import hexlify
import hmac

def generate_salt(size):
    """Create size byte hex salt"""
    return hexlify(os.urandom(size)).decode()

def generate_password():
    """Create 32 byte b64 password"""
    return base64.urlsafe_b64encode(os.urandom(32)).decode('utf-8')

def password_to_hmac(salt, password):
    m = hmac.new(bytearray(salt, 'utf-8'), bytearray(password, 'utf-8'), 'SHA256')
    return m.hexdigest()

def main():
    if len(sys.argv) < 2:
        sys.stderr.write('Please include username (and an optional password, will generate one if not provided) as an argument.\n')
        sys.exit(0)

    username = sys.argv[1]

    # Create 16 byte hex salt
    salt = generate_salt(16)
    if len(sys.argv) > 2:
        password = sys.argv[2]
    else:
        password = generate_password()
    password_hmac = password_to_hmac(salt, password)

    print('String to be appended to bitcoin.conf:')
    print('rpcauth={0}:{1}${2}'.format(username, salt, password_hmac))
    print('Your password:\n{0}'.format(password))

if __name__ == '__main__':
    main()
