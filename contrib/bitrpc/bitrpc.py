from jsonrpc import ServiceProxy
import sys
import string

# ===== BEGIN USER SETTINGS =====
# if you do not set these you will be prompted for a password for every command
rpcuser = ""
rpcpass = ""
# ====== END USER SETTINGS ======


if rpcpass == "":
	access = ServiceProxy("http://127.0.0.1:9332")
else:
	access = ServiceProxy("http://"+rpcuser+":"+rpcpass+"@127.0.0.1:9332")
cmd = sys.argv[1].lower()

if cmd == "backupwallet":
	try:
		path = raw_input("Enter destination path/filename: ")
		print access.backupwallet(path)
	except:
		print "\n---An error occurred---\n"

elif cmd == "getaccount":
	try:
		addr = raw_input("Enter a Litecoin address: ")
		print access.getaccount(addr)
	except:
		print "\n---An error occurred---\n"

elif cmd == "getaccountaddress":
	try:
		acct = raw_input("Enter an account name: ")
		print access.getaccountaddress(acct)
	except:
		print "\n---An error occurred---\n"

elif cmd == "getaddressesbyaccount":
	try:
		acct = raw_input("Enter an account name: ")
		print access.getaddressesbyaccount(acct)
	except:
		print "\n---An error occurred---\n"

elif cmd == "getbalance":
	try:
		acct = raw_input("Enter an account (optional): ")
		mc = raw_input("Minimum confirmations (optional): ")
		try:
			print access.getbalance(acct, mc)
		except:
			print access.getbalance()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getblockbycount":
	try:
		height = raw_input("Height: ")
		print access.getblockbycount(height)
	except:
		print "\n---An error occurred---\n"

elif cmd == "getblockcount":
	try:
		print access.getblockcount()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getblocknumber":
	try:
		print access.getblocknumber()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getconnectioncount":
	try:
		print access.getconnectioncount()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getdifficulty":
	try:
		print access.getdifficulty()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getgenerate":
	try:
		print access.getgenerate()
	except:
		print "\n---An error occurred---\n"

elif cmd == "gethashespersec":
	try:
		print access.gethashespersec()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getinfo":
	try:
		print access.getinfo()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getnewaddress":
	try:
		acct = raw_input("Enter an account name: ")
		try:
			print access.getnewaddress(acct)
		except:
			print access.getnewaddress()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getreceivedbyaccount":
	try:
		acct = raw_input("Enter an account (optional): ")
		mc = raw_input("Minimum confirmations (optional): ")
		try:
			print access.getreceivedbyaccount(acct, mc)
		except:
			print access.getreceivedbyaccount()
	except:
		print "\n---An error occurred---\n"

elif cmd == "getreceivedbyaddress":
	try:
		addr = raw_input("Enter a Litecoin address (optional): ")
		mc = raw_input("Minimum confirmations (optional): ")
		try:
			print access.getreceivedbyaddress(addr, mc)
		except:
			print access.getreceivedbyaddress()
	except:
		print "\n---An error occurred---\n"

elif cmd == "gettransaction":
	try:
		txid = raw_input("Enter a transaction ID: ")
		print access.gettransaction(txid)
	except:
		print "\n---An error occurred---\n"

elif cmd == "getwork":
	try:
		data = raw_input("Data (optional): ")
		try:
			print access.gettransaction(data)
		except:
			print access.gettransaction()
	except:
		print "\n---An error occurred---\n"

elif cmd == "help":
	try:
		cmd = raw_input("Command (optional): ")
		try:
			print access.help(cmd)
		except:
			print access.help()
	except:
		print "\n---An error occurred---\n"

elif cmd == "listaccounts":
	try:
		mc = raw_input("Minimum confirmations (optional): ")
		try:
			print access.listaccounts(mc)
		except:
			print access.listaccounts()
	except:
		print "\n---An error occurred---\n"

elif cmd == "listreceivedbyaccount":
	try:
		mc = raw_input("Minimum confirmations (optional): ")
		incemp = raw_input("Include empty? (true/false, optional): ")
		try:
			print access.listreceivedbyaccount(mc, incemp)
		except:
			print access.listreceivedbyaccount()
	except:
		print "\n---An error occurred---\n"

elif cmd == "listreceivedbyaddress":
	try:
		mc = raw_input("Minimum confirmations (optional): ")
		incemp = raw_input("Include empty? (true/false, optional): ")
		try:
			print access.listreceivedbyaddress(mc, incemp)
		except:
			print access.listreceivedbyaddress()
	except:
		print "\n---An error occurred---\n"

elif cmd == "listtransactions":
	try:
		acct = raw_input("Account (optional): ")
		count = raw_input("Number of transactions (optional): ")
		frm = raw_input("Skip (optional):")
		try:
			print access.listtransactions(acct, count, frm)
		except:
			print access.listtransactions()
	except:
		print "\n---An error occurred---\n"

elif cmd == "move":
	try:
		frm = raw_input("From: ")
		to = raw_input("To: ")
		amt = raw_input("Amount:")
		mc = raw_input("Minimum confirmations (optional): ")
		comment = raw_input("Comment (optional): ")
		try:
			print access.move(frm, to, amt, mc, comment)
		except:
			print access.move(frm, to, amt)
	except:
		print "\n---An error occurred---\n"

elif cmd == "sendfrom":
	try:
		frm = raw_input("From: ")
		to = raw_input("To: ")
		amt = raw_input("Amount:")
		mc = raw_input("Minimum confirmations (optional): ")
		comment = raw_input("Comment (optional): ")
		commentto = raw_input("Comment-to (optional): ")
		try:
			print access.sendfrom(frm, to, amt, mc, comment, commentto)
		except:
			print access.sendfrom(frm, to, amt)
	except:
		print "\n---An error occurred---\n"

elif cmd == "sendmany":
	try:
		frm = raw_input("From: ")
		to = raw_input("To (in format address1:amount1,address2:amount2,...): ")
		mc = raw_input("Minimum confirmations (optional): ")
		comment = raw_input("Comment (optional): ")
		try:
			print access.sendmany(frm,to,mc,comment)
		except:
			print access.sendmany(frm,to)
	except:
		print "\n---An error occurred---\n"

elif cmd == "sendtoaddress":
	try:
		to = raw_input("To (in format address1:amount1,address2:amount2,...): ")
		amt = raw_input("Amount:")
		comment = raw_input("Comment (optional): ")
		commentto = raw_input("Comment-to (optional): ")
		try:
			print access.sendtoaddress(to,amt,comment,commentto)
		except:
			print access.sendtoaddress(to,amt)
	except:
		print "\n---An error occurred---\n"

elif cmd == "setaccount":
	try:
		addr = raw_input("Address: ")
		acct = raw_input("Account:")
		print access.setaccount(addr,acct)
	except:
		print "\n---An error occurred---\n"

elif cmd == "setgenerate":
	try:
		gen= raw_input("Generate? (true/false): ")
		cpus = raw_input("Max processors/cores (-1 for unlimited, optional):")
		try:
			print access.setgenerate(gen, cpus)
		except:
			print access.setgenerate(gen)
	except:
		print "\n---An error occurred---\n"

elif cmd == "settxfee":
	try:
		amt = raw_input("Amount:")
		print access.settxfee(amt)
	except:
		print "\n---An error occurred---\n"

elif cmd == "stop":
	try:
		print access.stop()
	except:
		print "\n---An error occurred---\n"

elif cmd == "validateaddress":
	try:
		addr = raw_input("Address: ")
		print access.validateaddress(addr)
	except:
		print "\n---An error occurred---\n"

elif cmd == "walletpassphrase":
	try:
		pwd = raw_input("Enter wallet passphrase: ")
		access.walletpassphrase(pwd, 60)
		print "\n---Wallet unlocked---\n"
	except:
		print "\n---An error occurred---\n"

elif cmd == "walletpassphrasechange":
	try:
		pwd = raw_input("Enter old wallet passphrase: ")
		pwd2 = raw_input("Enter new wallet passphrase: ")
		access.walletpassphrasechange(pwd, pwd2)
		print
		print "\n---Passphrase changed---\n"
	except:
		print
		print "\n---An error occurred---\n"
		print

else:
	print "Command not found or not supported"
