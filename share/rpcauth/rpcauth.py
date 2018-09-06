#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import sys
import os
from random import SystemRandom
import base64
import hmac

def generate_salt():
    # This uses os.urandom() underneath
    cryptogen = SystemRandom()

    # Create 16 byte hex salt
    salt_sequence = [cryptogen.randrange(256) for _ in range(16)]
    return ''.join([format(r, 'x') for r in salt_sequence])

def generate_password(salt):
    """Create 32 byte b64 password"""
    password = base64.urlsafe_b64encode(os.urandom(32)).decode('utf-8')

    m = hmac.new(bytearray(salt, 'utf-8'), bytearray(password, 'utf-8'), 'SHA256')
    password_hmac = m.hexdigest()

    return password, password_hmac

def main():
    if len(sys.argv) < 2:
        sys.stderr.write('Please include username as an argument.\n')
        sys.exit(0)

    username = sys.argv[1]

    salt = generate_salt()
    password, password_hmac = generate_password(salt)

    print('String to be appended to bitcoin.conf:')
    print('rpcauth={0}:{1}${2}'.format(username, salt, password_hmac))
    print('Your password:\n{0}'.format(password))

if __name__ == '__main__':
    main()
