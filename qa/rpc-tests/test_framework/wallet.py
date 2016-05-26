
"""
  Copyright 2016 Tom Zander <tomz@freedommail.ch>

  Wallet is a class that holds the public and private keys usable for
  creating blocks using generate() and resusing the funds of the blockreward
  in further actions.
"""


class WalletAddress(object):
    def __init__(self, createdAddress):
        self.privateKey = createdAddress['private']
        self.publicKey = createdAddress['address']
        self.publicHexKey = createdAddress['pubkey']

class Wallet(object):

    def __init__(self, node):
        self.parentNode = node
        self.entries = []

    def createCoinbase(self):
        if len(self.entries) < 1:
            coinbase = WalletAddress(self.parentNode.createaddress())
            self.parentNode.importprivkey(coinbase.privateKey)
            self.entries.append(coinbase)
        return self.entries[0]

    def generate(self, count):
        self.createCoinbase()
        return self.parentNode.generate(count, self.entries[len(self.entries)-1].publicHexKey)

    def newAddress(self):
        address = WalletAddress(self.parentNode.createaddress())
        self.entries.append(address)
        return address.publicKey

