from bitcoinrpc import connect_to_local
from getpass import getpass

conn = connect_to_local()
old_pass = getpass("Enter old wallet passphrase: ")
new_pass = getpass("Enter new wallet passphrase: ")

while not conn.walletpassphrasechange(old_pass, new_pass, dont_raise=True):
    print 'Error: The wallet passphrase entered was incorrect. Try again.\n'
    old_pass = getpass("Enter old wallet passphrase: ")
    new_pass = getpass("Enter new wallet passphrase: ")

print 'Password changed successfully'
