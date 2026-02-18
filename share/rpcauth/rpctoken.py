#!/usr/bin/env python3
# Copyright (c) 2015-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from argparse import ArgumentParser
from secrets import token_hex, token_urlsafe
import hmac
import json

def generate_salt(size):
    """Create size byte hex salt"""
    return token_hex(size)

def generate_token():
    """Create 43 byte URL-safe token (base64 encoded 32 bytes)"""
    return token_urlsafe(32)

def token_to_hmac(salt, token):
    """Compute HMAC-SHA256 of token with salt"""
    m = hmac.new(salt.encode('utf-8'), token.encode('utf-8'), 'SHA256')
    return m.hexdigest()

def main():
    parser = ArgumentParser(description='Create API token credentials for a JSON-RPC user')
    parser.add_argument('username', help='the username for authentication')
    parser.add_argument('token', help='optional: provide an existing token, or omit to generate a random token', nargs='?')
    parser.add_argument("-j", "--json", help="output to json instead of plain-text", action='store_true')
    args = parser.parse_args()

    if not args.token:
        args.token = generate_token()

    # Create 16 byte hex salt
    salt = generate_salt(16)
    token_hmac = token_to_hmac(salt, args.token)

    if args.json:
        credentials_dict = {'username': args.username, 'token': args.token, 'rpctoken': f'{args.username}:{salt}${token_hmac}'}
        print(json.dumps(credentials_dict))
    else:
        print('String to be appended to bitcoin.conf:')
        print(f'rpctoken={args.username}:{salt}${token_hmac}')
        print(f'Your token:\n{args.token}')

if __name__ == '__main__':
    main()
