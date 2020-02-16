import os
import json
import datetime

def bitcoin(cmd):
	return os.popen("\"C:\\Program Files\\Bitcoin\\daemon\\bitcoin-cli\" -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 " + cmd).read()

"""file1 = open("lapSynchronizedMemPoolLog.csv", "r")
file2 = open("SynchronizedMemPoolLog.csv", "r")

fileALines = file1.readlines()
fileBLines = file2.readlines()
"""
fileALines = []
fileBLines = []
numALines = 0
numBLines = 0
with open("SynchronizedMemPoolLog.csv", "r") as f2:
	for line in f2:
		fileBLines.append(line)
		numBLines += 1
with open("lapSynchronizedMemPoolLog.csv", "r") as f1:
	for line in f1:
		if(numALines > numBLines):
			break
		fileALines.append(line)
		numALines += 1

minLineLength = min(len(fileALines), len(fileBLines))

output = "A Date;B Date;Seconds apart;|;Num peers A;Num peers B;Total ping time A;Total ping time B;Average ping time A;Average ping time B;Missing pongs 1;Missing pongs 2;Total ban score A;Total ban score B;Shared Peers;|;Mempool A size;Mempool B size;Size difference;|;Only in A size;Only in B size;In both size;In either size;Percent difference;In either fee total;In either size total;In either feerate total;In either;In either JSON"
output += "\n"

outputFile = open("GeneratedAsyncComparison.csv", "w+")

for i in range(0, minLineLength):
	fileAList = fileALines[i].split(',')
	fileBList = fileBLines[i].split(',')
	date1 = fileAList.pop(0)
	date2 = fileBList.pop(0)
	numPeersA = fileAList.pop(0)
	numPeersB = fileBList.pop(0)
	totalPingTime1 = fileAList.pop(0)
	totalPingTime2 = fileBList.pop(0)
	averagePingTime1 = fileAList.pop(0)
	averagePingTime2 = fileBList.pop(0)
	noResponsePings1 = fileAList.pop(0)
	noResponsePings2 = fileBList.pop(0)
	totalBanScore1 = fileAList.pop(0)
	totalBanScore2 = fileBList.pop(0)
	addresses1 = fileAList.pop(0).split(' ')
	addresses2 = fileBList.pop(0).split(' ')
	addressesInBoth = set(addresses1).intersection(addresses2)


	d2 = datetime.datetime.strptime(date1, '%Y-%m-%d %H:%M:%S.%f')
	d1 = datetime.datetime.strptime(date2, '%Y-%m-%d %H:%M:%S.%f')
	secondsDifference = (d2 - d1).total_seconds()

	onlyInA = len(set(fileAList).difference(fileBList))
	inBoth = len(set(fileAList).intersection(fileBList))
	onlyInB = len(set(fileBList).difference(fileAList))
	inEither = set(fileAList).symmetric_difference(fileBList)
	inEitherJSON = []
	inEitherFeeTotal = 0
	inEitherSizeTotal = 0

	print("Line " + str(i + 1) + ' / ' + str(minLineLength) + "\t\tSz: " + str(len(inEither)))
	"""	-txindex=1
	for txid in inEither:
		txJSON = bitcoin('decoderawtransaction "' + bitcoin('getrawtransaction "' + txid))
		#inEitherJSON.append(txJSON)
		#try:
		inputSum = 0
		outputSum = 0
		if(len(txJSON) == 0):
			continue
		tx = json.loads(txJSON)

		for txIn in tx['vin']:
			tx2JSON = bitcoin('decoderawtransaction "' + bitcoin('getrawtransaction "' + txIn['txid']))
			if(len(tx2JSON) == 0):
				continue
			tx2 = json.loads(tx2JSON)
			for tx2Out in tx2['vout']:
				inputSum += tx2Out['value']

		for txOut in tx['vout']:
			outputSum += txOut['value']

		print(txid)
		print(inputSum)
		print(outputSum)
		print(tx)
		inEitherFeeTotal += outputSum - inputSum
		inEitherSizeTotal += tx['size']
		#except:
		#	print('error: json: ' + txJSON)
	#"""
	output += date1 + ';'
	output += date2 + ';'
	output += str(secondsDifference) + ';'
	output += '|;'
	output += str(numPeersA) + ';'
	output += str(numPeersB) + ';'
	output += str(totalPingTime1) + ';'
	output += str(totalPingTime2) + ';'
	output += str(averagePingTime1) + ';'
	output += str(averagePingTime2) + ';'
	output += str(noResponsePings1) + ';'
	output += str(noResponsePings2) + ';'
	output += str(totalBanScore1) + ';'
	output += str(totalBanScore2) + ';'
	output += str(", ".join(addressesInBoth)) + ';'
	output += '|;'
	output += str(len(fileAList)) + ';'
	output += str(len(fileBList)) + ';'
	output += str(len(fileAList) - len(fileBList)) + ';'
	output += '|;'
	output += str(onlyInA) + ';'
	output += str(onlyInB) + ';'
	output += str(inBoth) + ';'
	output += str(len(inEither)) + ';'
	output += str(len(inEither) / (len(inEither) + inBoth)) + ';'
	output += str(inEitherFeeTotal) + ';'
	output += str(inEitherSizeTotal) + ';'
	output += str(0) + ';'
	#output += '|;'
	#output += '"' + ' '.join(onlyInA) + '";'
	#output += '"' + ' '.join(onlyInB) + '";'
	#output += '"' + ' '.join(inBoth) + '";'
	output += '"' + ' '.join(inEither) + '";'
	output += '"' + ' '.join(inEitherJSON) + '"'
	output += '\n'
	outputFile.write(output)
	output = ""

outputFile.close()