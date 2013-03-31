from bitcoinrpc import connect_to_local
from getpass import getpass

conn = connect_to_local()
pwd = getpass("Enter wallet passphrase: ")

while not conn.walletpassphrase(pwd, 60, dont_raise=True):
    print "Wrong password. Try again."
    pwd = getpass("Enter wallet passphrase: ")

print "Wallet unlocked."
