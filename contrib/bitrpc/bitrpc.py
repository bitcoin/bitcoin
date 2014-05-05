import sys
import string
from jsonrpc import ServiceProxy


class BitcoinRPC:
    def __init__(self, username=None, password=None):
        if rpcuser and rpcpass:
            url = "http://%s:%s@127.0.0.1:8332" % (username, password)
        else:
            url = "http://127.0.0.1:8332"
        self.access = ServiceProxy(url)

    def backupwallet(self):
        path = raw_input("Enter destination path/filename: ")
        return self.access.backupwallet(path)

    def getaccount(self):
        address = raw_input("Enter a Bitcoin addressess: ")
        return self.access.getaccount(address)

    def getaccountaddressess(self):
        account = raw_input("Enter an account name: ")
        return self.access.getaccountaddressess(account)

    def getaddressessesbyaccount(self):
        account = raw_input("Enter an account name: ")
        return self.access.getaddressessesbyaccount(account)

    def getbalance(self):
        account = raw_input("Enter an account (optional): ")
        minimum_confirmations = raw_input("Minimum confirmations (optional): ")
        try:
            return self.access.getbalance(account, minimum_confirmations)
        except:
            return self.access.getbalance()

    def getblockbycount(self):
        height = raw_input("Height: ")
        return self.access.getblockbycount(height)

    def getblockcount(self):
        return self.access.getblockcount()

    def getblocknumber(self):
        return self.access.getblocknumber()

    def getconnectioncount(self):
        return self.access.getconnectioncount()

    def getdifficulty(self):
        return self.access.getdifficulty()

    def getgenerate(self):
        return self.access.getgenerate()

    def gethashespersec(self):
        return self.access.gethashespersec()

    def getinfo(self):
        return self.access.getinfo()

    def getnewaddressess(self):
        account = raw_input("Enter an account name: ")
        try:
            return self.access.getnewaddressess(account)
        except:
            return self.access.getnewaddressess()

    def getreceivedbyaccount(self):
        account = raw_input("Enter an account (optional): ")
        minimum_confirmations = raw_input("Minimum confirmations (optional): ")
        try:
            return self.access.getreceivedbyaccount(
                account, minimum_confirmations)
        except:
            return self.access.getreceivedbyaccount()

    def getreceivedbyaddressess(self):
        address = raw_input("Enter a Bitcoin addressess (optional): ")
        minimum_confirmations = raw_input(
            "Minimum confirmations (optional): ")
        try:
            return self.access.getreceivedbyaddressess(
                address, minimum_confirmations)
        except:
            return self.access.getreceivedbyaddressess()

    def gettransaction(self):
        txid = raw_input("Enter a transaction ID: ")
        return self.access.gettransaction(txid)

    def getwork(self):
        data = raw_input("Data (optional): ")
        try:
            return self.access.gettransaction(data)
        except:
            return self.access.gettransaction()

    def help(self):
        cmd = raw_input("Command (optional): ")
        try:
            return self.access.help(cmd)
        except:
            return self.access.help()

    def listaccounts(self):
        minimum_confirmations = raw_input("Minimum confirmations (optional): ")
        try:
            return self.access.listaccounts(minimum_confirmations)
        except:
            return self.access.listaccounts()

    def listreceivedbyaccount(self):
        minimum_confirmations = raw_input("Minimum confirmations (optional): ")
        include_empty = raw_input("Include empty? (true/false, optional): ")
        try:
            return self.access.listreceivedbyaccount(
                minimum_confirmations, include_empty)
        except:
            return self.access.listreceivedbyaccount()

    def listreceivedbyaddressess(self):
        minimum_confirmations = raw_input("Minimum confirmations (optional): ")
        include_empty = raw_input("Include empty? (true/false, optional): ")
        try:
            return self.access.listreceivedbyaddressess(
                minimum_confirmations, include_empty)
        except:
            return self.access.listreceivedbyaddressess()

    def listtransactions(self):
        account = raw_input("Account (optional): ")
        count = raw_input("Number of transactions (optional): ")
        frm = raw_input("Skip (optional): ")
        try:
            return self.access.listtransactions(account, count, frm)
        except:
            return self.access.listtransactions()

    def move(self):
        frm = raw_input("From: ")
        to = raw_input("To: ")
        amount = raw_input("Amount: ")
        minimum_confirmations = raw_input("Minimum confirmations (optional): ")
        comment = raw_input("Comment (optional): ")
        try:
            return self.access.move(
                frm, to, amount, minimum_confirmations, comment)
        except:
            return self.access.move(frm, to, amount)

    def sendfrom(self):
        frm = raw_input("From: ")
        to = raw_input("To: ")
        amount = raw_input("Amount: ")
        minimum_confirmations = raw_input("Minimum confirmations (optional): ")
        comment = raw_input("Comment (optional): ")
        comment_to = raw_input("Comment-to (optional): ")
        try:
            return self.access.sendfrom(
                frm, to, amount, minimum_confirmations, comment, comment_to)
        except:
            return self.access.sendfrom(frm, to, amount)

    def sendmany(self):
        frm = raw_input("From: ")
        to = raw_input(
            "To (format addressess1:amount1,addressess2:amount2,...): ")
        minimum_confirmations = raw_input("Minimum confirmations (optional): ")
        comment = raw_input("Comment (optional): ")
        try:
            return self.access.sendmany(
                rm, to, minimum_confirmations, comment)
        except:
            return self.access.sendmany(frm, to)

    def sendtoaddressess(self):
        to = raw_input(
            "To (format addressess1:amount1,addressess2:amount2,...): ")
        amount = raw_input("Amount: ")
        comment = raw_input("Comment (optional): ")
        comment_to = raw_input("Comment-to (optional): ")
        try:
            return self.access.sendtoaddressess(
                to, amount, comment, comment_to)
        except:
            return self.access.sendtoaddressess(to, amount)

    def setaccount(self):
        address = raw_input("Address: ")
        account = raw_input("Account: ")
        return self.access.setaccount(address, account)

    def setgenerate(self):
        gen = raw_input("Generate? (true/false): ")
        cpus = raw_input("Max processors/cores (-1 for unlimited, optional): ")
        try:
            return self.access.setgenerate(gen, cpus)
        except:
            return self.access.setgenerate(gen)

    def settxfee(self):
        amount = raw_input("Amount: ")
        return self.access.settxfee(amount)

    def stop(self):
        return self.access.stop()

    def validateaddressess(self):
        address = raw_input("Address: ")
        return self.access.validateaddressess(address)

    def walletpassphrase(self):
        password = raw_input("Enter wallet passphrase: ")
        self.access.walletpassphrase(password, 60)
        return "\n---Wallet unlocked---\n"

    def walletpassphrasechange(self):
        password = raw_input("Enter old wallet passphrase: ")
        new_password = raw_input("Enter new wallet passphrase: ")
        self.access.walletpassphrasechange(password, new_password)
        return "\n---Passphrase changed---\n"

cmd = sys.argv[1].lower()
rpc_client = BitcoinRPC()
try:
    print getattr(rpc_client, cmd)
except AttributeError:
    print "Command not found or not supported"
except:
    print "\n---An error occurred---\n"