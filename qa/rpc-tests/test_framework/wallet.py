
"""
  Copyright 2016 Tom Zander <tomz@freedommail.ch>

  Wallet is a class that holds the public and private keys usable for
  creating blocks using generate() and resusing the funds of the blockreward
  in further actions.
"""


class WalletAddress:
    def __init__(self, created_address):
        self.private_key = created_address['private']
        self.public_key = created_address['address']
        self.public_hex_key = created_address['pubkey']

class Wallet:

    def __init__(self, node):
        self.parent_node = node
        self.entries = []

    def create_coinbase(self):
        if len(self.entries) < 1:
            coinbase = WalletAddress(self.parent_node.createaddress())
            self.parent_node.importprivkey(coinbase.private_key)
            self.entries.append(coinbase)
        return self.entries[0]

    def generate(self, count):
        self.create_coinbase()
        return self.parent_node.generate(count, self.entries[0].public_hex_key)

    def new_address(self):
        address = WalletAddress(self.parent_node.createaddress())
        self.entries.append(address)
        return address.public_key

