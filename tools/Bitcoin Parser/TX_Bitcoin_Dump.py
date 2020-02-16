import json, os, sys, datetime, re
from blockchain_parser.blockchain import Blockchain

bitcoinPath = '/media/sim/BITCOIN/blocks'
#bitcoinPath = '/home/sim/.bitcoin/blocks' #'/media/sim/BITCOIN/blocks'
startHeight = 300000
endHeight = 587693

if not os.path.exists(bitcoinPath):
	print(bitcoinPath + ' does not exist.')
	sys.exit()

blockchain = Blockchain(os.path.expanduser(bitcoinPath))
output = open('Bitcoin Transactions.csv', 'w+')
counter = 0
epoch = datetime.datetime.utcfromtimestamp(0)

def extractAscii(value):
	ASCII_occurances = ''
	operations = value.split(' ')
	for i in range(len(operations)):
		try:
			op_ascii = bytearray.fromhex(operations[i]).decode()
			if re.match('^[\x0A\x09\x0A\x20-\x7E]+$', op_ascii) == None:
				raise Exception()
			# We have found something that can convert to printable ascii
			if not ASCII_occurances:
				ASCII_occurances += "\n" # More than one ASCII messages
			ASCII_occurances += op_ascii
		except:
			pass
	return ASCII_occurances

def scriptValid(inOrOut, inOut):
	#return inOut.script.in_unknown
	ASCII_occurances = extractAscii(inOut.script.value)
	if ASCII_occurances:
		print(ASCII_occurances)
		return False # There's a message!

	if(inOrOut == "Input"):
		try:
			a = inOut.script.is_pubkey()
			return True # ! Skipping a lot of possibly valuable inputs
		except:
			return False
	else:
		if(inOut.value == 0):
			return True # Just skip the outputs that don't send any money
		if re.match('^\s?[0-9a-f]+(?: OP_CHECKSIG)?$', inOut.script.value) != None:
			return True # Skip the scripts that are only a single value, it may have glitched
		try:
			return inOut.script.is_pubkeyhash() or inOut.script.is_p2sh() or inOut.script.is_multisig()
		except:
			return False

def dumpScript(block, tx, inOrOut, inOut):
	line = ''
	line += str(block.height) + ','
	line += str(block.header.timestamp) + ','
	line += str((block.header.timestamp - epoch).total_seconds()) + ','
	if(inOrOut == "Output"):
		line += str(inOut.value / 100000000) + ','
	else:
		line += ','
	ASCII_occurances = re.sub(r'\\', '\\\\', re.sub('"', '""', extractAscii(inOut.script.value))) # Prevent it from escaping .csv format
	line += '"' + ASCII_occurances + '",'
	line += inOrOut + ','
	line += str(inOut.size) + ','
	line += str(tx.txid) + ','
	line += '"' + inOut.script.value + '",'
	line += str(tx.version) + ','
	line += str(tx.is_segwit) + ','
	line += str(tx.locktime) + ','
	line += str(tx.uses_replace_by_fee()) + ','
	line += str(tx.uses_bip69()) + ','
	line += str(tx.n_inputs) + ','
	line += str(tx.n_outputs)
	return line

def dumpBlock(block):
	lines = ''

	for tx in block.transactions:
		if(tx.is_coinbase()):
			continue

		for _input in tx.inputs:
			if(not scriptValid("Input", _input)):
				lines += dumpScript(block, tx, "Input", _input) + '\n'

		for _output in tx.outputs:
			if(not scriptValid("Output", _output)):
				lines += dumpScript(block, tx, "Output", _output) + '\n'


		continue
	return lines

line = 'Block Height,Time,Time (s),Value (BTC),Detected ASCII,Input/Output,In/Out Size,Tx ID,Script,Tx Version,Is Segwit,Locktime,Uses Replace_by_fee,Uses BIP69,Num. Inputs,Num. Outputs'
output.write(line + '\n')

for block in blockchain.get_ordered_blocks(os.path.expanduser(bitcoinPath + '/index'), start=startHeight, end=endHeight, cache='index-cache.pickle'):
	counter += 1
	if(counter % 100 == 0): #100
		print('Block ' + str(counter) + '\t' + str(100 * counter / (endHeight - startHeight)) + '% Complete')
		output.flush()
	line = dumpBlock(block)
	if(line):
		output.write(line)

output.close()
print('Ok.')
