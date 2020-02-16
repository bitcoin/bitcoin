import json, os, sys, datetime
from blockchain_parser.blockchain import Blockchain

bitcoinPath = '/media/sim/BITCOIN/blocks'
#bitcoinPath = '/home/sim/.bitcoin/blocks' #'/media/sim/BITCOIN/blocks'
startHeight = 605534
endHeight = 617600

if not os.path.exists(bitcoinPath):
	print(bitcoinPath + ' does not exist.')
	sys.exit()

blockchain = Blockchain(os.path.expanduser(bitcoinPath))
output = open(os.path.expanduser(f'~/Desktop/Bitcoin full data V5.csv'), 'w+')
counter = 0
epoch = datetime.datetime.utcfromtimestamp(0)


def dumpBlock(block):
	blockSolver = ''
	blockSolverReward = 0

	#transactionString = 'Number,Amount (Satoshi),Address,ID,Index,Hash,Input Size,Output Size,Uses Segwit,Uses BIP69,Uses Replace_by_Fee,Version,Size,Num. Inputs,Num. Outputs,Lock Time,Witnesses,is_multisig,is_p2sh,is_pubkey,is_pubkeyhash,is_return,is_unknown,type\n'
	transactionSum = 0
	transactionAverage = 0
	transactionMinimum = float('inf')
	transactionMaximum = 0
	totalNumTransactions = 0
	num_segwit = 0
	num_coinbase = 0
	num_bip69 = 0
	num_replace_by_fee = 0
	avg_locktime = 0
	min_locktime = float('inf')
	max_locktime = 0
	avg_ninputs = 0
	avg_noutputs = 0

	for tx in block.transactions:
		txBalance = 0
		totalNumTransactions += max(len(tx.inputs), len(tx.outputs))

		for output in tx.outputs:
			txBalance += output.value

		transactionSum += txBalance
		if(txBalance < transactionMinimum): transactionMinimum = txBalance
		if(txBalance > transactionMaximum): transactionMaximum = txBalance

		avg_ninputs += len(tx.inputs)
		avg_noutputs += len(tx.outputs)
		avg_locktime += tx.locktime
		if(tx.locktime < min_locktime): min_locktime = tx.locktime
		if(tx.locktime > max_locktime): max_locktime = tx.locktime
		if tx.is_segwit: num_segwit += 1
		if tx.uses_bip69(): num_bip69 += 1
		if tx.uses_replace_by_fee(): num_replace_by_fee += 1
		if tx.is_coinbase():
			txAddresses = ''
			for output in tx.outputs:
				try:
					if(txAddresses != ''):
						txAddresses += ' '
					txAddresses += ' '.join(a.address for a in output.addresses)
				except: pass
				txBalance += output.value

			num_coinbase += 1
			if(blockSolverReward != 0):
				# Not good, multiple coinbases
				blockSolver += '\n'
			blockSolverReward += tx.outputs[0].value
			blockSolver += txAddresses.rstrip()

		''' # Transaction processing using
		transactionString += str(transactionNum) + ','
		transactionString += str(tx.outputs[0].value) + ','
		transactionString += str(tx.outputs[0].addresses) + ','
		transactionString += str(tx.txid) + ','
		transactionString += str(tx.inputs[0].transaction_index) + ','
		transactionString += str(tx.inputs[0].transaction_hash) + ','
		transactionString += str(tx.inputs[0].size) + ','
		transactionString += str(tx.outputs[0].size) + ','
		transactionString += str(tx.is_segwit) + ','
		transactionString += str(tx.uses_bip69()) + ','
		transactionString += str(tx.uses_replace_by_fee()) + ','
		transactionString += str(tx.version) + ','
		transactionString += str(tx.size) + ','
		transactionString += str(tx.n_inputs) + ','
		transactionString += str(tx.n_outputs) + ','
		transactionString += str(tx.locktime) + ','
		transactionString += str(tx.inputs[0].witnesses) + ','
		transactionString += str(tx.outputs[0].is_multisig()) + ','
		transactionString += str(tx.outputs[0].is_p2sh()) + ','
		transactionString += str(tx.outputs[0].is_pubkey()) + ','
		transactionString += str(tx.outputs[0].is_pubkeyhash()) + ','
		transactionString += str(tx.outputs[0].is_return()) + ','
		transactionString += str(tx.outputs[0].is_unknown()) + ','
		transactionString += str(tx.outputs[0].type) + ','
		transactionString += '\n'
		'!''transactionString += 'Transaction ' + str(transactionNum) + ': '
		for v2 in dir(tx):
			if(v2.startswith('_')): continue
			if v2 == 'hex': continue
			if v2 == 'from_hex': continue
			txv = getattr(tx, v2)
			if isinstance(txv, (bool, int, float, bytes, str)):
				print('\t' + v2 + ' = ' + str(txv))
				continue
			if type(txv) == list:
				print('\t' + v2 + ':')
				for txv2 in txv:
					for v3 in dir(txv2):
						if(v3.startswith('_')): continue
						if v3 == 'hex': continue
						if v3 == 'from_hex': continue
						if isinstance(getattr(txv2, v3), (bool, int, float, str)):
							print('\t\t' + v3 + ' = ' + str(getattr(txv2, v3)))
							continue
						print('\t\t' + v3 + " : " + str(type(getattr(txv2, v3))) + " = " + str(getattr(txv2, v3)))
				continue
				#print('!\t' + v2 + ' = ' + ''.join(txv))
			print('!\t\t' + v2 + " : " + str(type(txv)) + " = " + str(getattr(tx, v2)))
		'''
	avg_ninputs = avg_ninputs / len(block.transactions)
	avg_noutputs = avg_noutputs / len(block.transactions)
	avg_locktime = avg_locktime / len(block.transactions)
	transactionAverage = transactionSum / totalNumTransactions
	output = ''
	output += str(block.height) + ','
	output += str(block.header.timestamp) + ','
	output += str((block.header.timestamp - epoch).total_seconds()) + ','
	output += str(block.header.difficulty) + ','
	output += str(block.size / 1000) + ','
	output += str(block.n_transactions) + ','
	output += str(totalNumTransactions) + ','
	output += str(num_segwit) + ','
	output += str(num_bip69) + ','
	output += str(num_replace_by_fee) + ','
	output += str(blockSolverReward / 100000000) + ','
	output += str(transactionSum / 100000000) + ','
	output += str(transactionMinimum / 100000000) + ','
	output += str(transactionMaximum / 100000000) + ','
	output += str(transactionAverage / 100000000) + ','
	output += str(min_locktime) + ','
	output += str(max_locktime) + ','
	output += str(avg_locktime) + ','
	output += str(avg_ninputs) + ','
	output += str(avg_noutputs) + ','
	output += str(block.header.version) + ','
	output += str(block.header.nonce) + ','
	output += str(block.header.bits) + ','
	output += str(blockSolver) + ','
	output += str(num_coinbase) + ','
	output += str(block.hash) + ','
	output += str(block.header.previous_block_hash) + ','
	output += str(block.header.merkle_root)
	#output += '"' + transactionString + '"'
	return output
'''
	for v1 in dir(block):
		if(v1.startswith('_')): continue
		if v1 == 'hex': continue
		if v1 == 'from_hex': continue
		v1type = type(getattr(block, v1))
		if(v1type == str or v1type == int):
			print(v1 + " = " + str(getattr(block, v1)))
			continue
		if(v1 == 'header'):
			header = block.header
			for v2 in dir(header):
				if(v2.startswith('_')): continue
				if v2 == 'hex': continue
				if v2 == 'from_hex': continue
				print(v2 + " = " + str(getattr(header, v2)))
			continue
		if(v1 == 'transactions'):
			transactionNum = 0
			for tx in block.transactions:
				transactionNum += 1
				print('Transaction ' + str(transactionNum) + ':')
				for v2 in dir(tx):
					if(v2.startswith('_')): continue
					if v2 == 'hex': continue
					if v2 == 'from_hex': continue
					txv = getattr(tx, v2)
					if isinstance(txv, (bool, int, float, bytes, str)):
						print('\t' + v2 + ' = ' + str(txv))
						continue
					if type(txv) == list:
						print('\t' + v2 + ':')
						for txv2 in txv:
							for v3 in dir(txv2):
								if(v3.startswith('_')): continue
								if v3 == 'hex': continue
								if v3 == 'from_hex': continue
								if isinstance(getattr(txv2, v3), (bool, int, float, str)):
									print('\t\t' + v3 + ' = ' + str(getattr(txv2, v3)))
									continue
								print('\t\t' + v3 + " : " + str(type(getattr(txv2, v3))) + " = " + str(getattr(txv2, v3)))
						continue
						#print('!\t' + v2 + ' = ' + ''.join(txv))
					print('!\t\t' + v2 + " : " + str(type(txv)) + " = " + str(getattr(tx, v2)))
			continue

		print('!!!! ' + v1 + " : " + str(type(getattr(block, v1))))

		#for v2 in dir(v1):
		#	print("\t" + v2)

		if(v.startswith("_")): continue
		if v == 'hex': continue
		val = getattr(block, v)
		if level >= 2 or isinstance(val, (int, float, bytes, str, list, dict, set)):
			print(level*' ', val)
		else:
			print("deep into " + v + "=" + str(val))
			dumpBlock(val, level=level+1)'''
	

line = 'Height,Timestamp,Timestamp (s),Difficulty,Size (KB),N. Txs on Block,N. Txs Total,N. Segwit,N. BIP69,N. Replace by Fee,Solver Reward,Tx Total,Tx Min,Tx Max,Tx Avg,Locktime Min,Locktime Max,Locktime Avg,N. Inputs Avg,N. Outputs Avg,Version,Nonce,Bits,Solver,N. Coinbase,Hash,Prev. Hash,Merkle Root'
output.write(line + '\n')
for block in blockchain.get_ordered_blocks(os.path.expanduser(bitcoinPath + '/index'), start=startHeight, end=endHeight, cache='index-cache.pickle'):
	counter += 1
	if(counter % 100 == 0): #100
		print('Block ' + str(counter) + '\t' + str(100 * counter / (endHeight - startHeight)) + '% Complete')
		output.flush()
	line = dumpBlock(block)
	'''transactionString = ''
	for v in dir(block):
		if v.startswith('_'): continue
		if v == 'from_hex': continue
		if v == 'hex': continue
		if v == 'transactions':
			#txCount = 0
			#for tx in block.transactions:
			#	txCount += 1
			#	transactionString += 'Transactions' + txCount + ''
			#	transactionString += v + ':' + str(getattr(block, v)) + '\t'
 			#	#print(tx)
			#	#for tx2 in dir(tx):
			#	#	print(tx2 + ' = ' + str(getattr(tx, tx2)))
			#
			continue

		line += v + ':' + str(getattr(block, v)) + '\t'
			#print(v)
			#print(getattr(block, v))
	if(counter % 10000 == 0):
		print('Block ' + str(counter))
	#if(counter % 100000 == 0):
	#	output.close()
	#	output = open('Bitcoin' + str(counter) + '.txt', 'w+')
	'''
	output.write(line + '\n')

output.close()
print('Ok.')
