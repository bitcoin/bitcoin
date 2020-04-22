import os
import re
import json
import time
import datetime
import subprocess
import glob
import traceback
from threading import Timer


startupDelay = 0			# Number of seconds to wait after each node start up
numSecondsPerSample = 1
rowsPerNodeReset = 3000		# Number of rows for cach numconnections file

os.system('clear')

datadir = '' # Virtual machine shared folder
if os.path.exists('/media/sf_Bitcoin/blocks'):
	datadir = ' -datadir=/media/sf_Bitcoin' # Virtual machine shared folder
elif os.path.exists('/media/sf_BitcoinVictim/blocks'):
	datadir = ' -datadir=/media/sf_BitcoinVictim'
elif os.path.exists('/media/sf_BitcoinAttacker/blocks'):
	datadir = ' -datadir=/media/sf_BitcoinAttacker'
elif os.path.exists('/media/sim/BITCOIN/blocks'):
	datadir = ' -datadir=/media/sim/BITCOIN'


#def getmyIP():
#	return os.popen('curl \'http://myexternalip.com/raw\'').read() + ':8333'

def bitcoin(cmd):
	return os.popen('./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

def startBitcoin():
	subprocess.Popen(['gnome-terminal -t "Custom Bitcoin Core Instance" -- .././run.sh'], shell=True)

def bitcoinUp():
	return winexists('Custom Bitcoin Core Instance')

def winexists(target):
    for line in subprocess.check_output(['wmctrl', '-l']).splitlines():
        window_name = line.split(None, 3)[-1].decode()
        if window_name == target:
            return True
    return False

def fetchHeader():
	line = 'Timestamp,'
	line += 'Timestamp (Seconds),'
	line += 'NumPeers,'
	line += 'InactivePeers,'
	line += 'AvgPingTime,'
	line += 'TotalBanScore,'
	line += 'Connections,'

	line += 'BlockHeight,'
	line += 'MempoolSize,'
	line += 'MempoolBytes,'
	line += 'AcceptedBlocksPerSec,'
	line += 'AcceptedBlocksPerSecAvg,'
	line += 'AcceptedTxPerSec,'
	line += 'AcceptedTxPerSecAvg,'

	line += 'ClocksPerSec,'
	line += '# VERSION Msgs,'
	line += 'Num VERSION PerSec,'
	line += 'Avg VERSION PerSec,'
	line += 'ClocksAvg VERSION,'
	line += 'ClocksMax VERSION,'
	line += 'BytesAvg VERSION,'
	line += 'BytesMax VERSION,'
	line += '# VERACK Msgs,'
	line += 'Num VERACK PerSec,'
	line += 'Avg VERACK PerSec,'
	line += 'ClocksAvg VERACK,'
	line += 'ClocksMax VERACK,'
	line += 'BytesAvg VERACK,'
	line += 'BytesMax VERACK,'
	line += '# ADDR Msgs,'
	line += 'Num ADDR PerSec,'
	line += 'Avg ADDR PerSec,'
	line += 'ClocksAvg ADDR,'
	line += 'ClocksMax ADDR,'
	line += 'BytesAvg ADDR,'
	line += 'BytesMax ADDR,'
	line += '# INV Msgs,'
	line += 'Num INV PerSec,'
	line += 'Avg INV PerSec,'
	line += 'ClocksAvg INV,'
	line += 'ClocksMax INV,'
	line += 'BytesAvg INV,'
	line += 'BytesMax INV,'
	line += '# GETDATA Msgs,'
	line += 'Num GETDATA PerSec,'
	line += 'Avg GETDATA PerSec,'
	line += 'ClocksAvg GETDATA,'
	line += 'ClocksMax GETDATA,'
	line += 'BytesAvg GETDATA,'
	line += 'BytesMax GETDATA,'
	line += '# MERKLEBLOCK Msgs,'
	line += 'Num MERKLEBLOCK PerSec,'
	line += 'Avg MERKLEBLOCK PerSec,'
	line += 'ClocksAvg MERKLEBLOCK,'
	line += 'ClocksMax MERKLEBLOCK,'
	line += 'BytesAvg MERKLEBLOCK,'
	line += 'BytesMax MERKLEBLOCK,'
	line += '# GETBLOCKS Msgs,'
	line += 'Num GETBLOCKS PerSec,'
	line += 'Avg GETBLOCKS PerSec,'
	line += 'ClocksAvg GETBLOCKS,'
	line += 'ClocksMax GETBLOCKS,'
	line += 'BytesAvg GETBLOCKS,'
	line += 'BytesMax GETBLOCKS,'
	line += '# GETHEADERS Msgs,'
	line += 'Num GETHEADERS PerSec,'
	line += 'Avg GETHEADERS PerSec,'
	line += 'ClocksAvg GETHEADERS,'
	line += 'ClocksMax GETHEADERS,'
	line += 'BytesAvg GETHEADERS,'
	line += 'BytesMax GETHEADERS,'
	line += '# TX Msgs,'
	line += 'Num TX PerSec,'
	line += 'Avg TX PerSec,'
	line += 'ClocksAvg TX,'
	line += 'ClocksMax TX,'
	line += 'BytesAvg TX,'
	line += 'BytesMax TX,'
	line += '# HEADERS Msgs,'
	line += 'Num HEADERS PerSec,'
	line += 'Avg HEADERS PerSec,'
	line += 'ClocksAvg HEADERS,'
	line += 'ClocksMax HEADERS,'
	line += 'BytesAvg HEADERS,'
	line += 'BytesMax HEADERS,'
	line += '# BLOCK Msgs,'
	line += 'Num BLOCK PerSec,'
	line += 'Avg BLOCK PerSec,'
	line += 'ClocksAvg BLOCK,'
	line += 'ClocksMax BLOCK,'
	line += 'BytesAvg BLOCK,'
	line += 'BytesMax BLOCK,'
	line += '# GETADDR Msgs,'
	line += 'Num GETADDR PerSec,'
	line += 'Avg GETADDR PerSec,'
	line += 'ClocksAvg GETADDR,'
	line += 'ClocksMax GETADDR,'
	line += 'BytesAvg GETADDR,'
	line += 'BytesMax GETADDR,'
	line += '# MEMPOOL Msgs,'
	line += 'Num MEMPOOL PerSec,'
	line += 'Avg MEMPOOL PerSec,'
	line += 'ClocksAvg MEMPOOL,'
	line += 'ClocksMax MEMPOOL,'
	line += 'BytesAvg MEMPOOL,'
	line += 'BytesMax MEMPOOL,'
	line += '# PING Msgs,'
	line += 'Num PING PerSec,'
	line += 'Avg PING PerSec,'
	line += 'ClocksAvg PING,'
	line += 'ClocksMax PING,'
	line += 'BytesAvg PING,'
	line += 'BytesMax PING,'
	line += '# PONG Msgs,'
	line += 'Num PONG PerSec,'
	line += 'Avg PONG PerSec,'
	line += 'ClocksAvg PONG,'
	line += 'ClocksMax PONG,'
	line += 'BytesAvg PONG,'
	line += 'BytesMax PONG,'
	line += '# NOTFOUND Msgs,'
	line += 'Num NOTFOUND PerSec,'
	line += 'Avg NOTFOUND PerSec,'
	line += 'ClocksAvg NOTFOUND,'
	line += 'ClocksMax NOTFOUND,'
	line += 'BytesAvg NOTFOUND,'
	line += 'BytesMax NOTFOUND,'
	line += '# FILTERLOAD Msgs,'
	line += 'Num FILTERLOAD PerSec,'
	line += 'Avg FILTERLOAD PerSec,'
	line += 'ClocksAvg FILTERLOAD,'
	line += 'ClocksMax FILTERLOAD,'
	line += 'BytesAvg FILTERLOAD,'
	line += 'BytesMax FILTERLOAD,'
	line += '# FILTERADD Msgs,'
	line += 'Num FILTERADD PerSec,'
	line += 'Avg FILTERADD PerSec,'
	line += 'ClocksAvg FILTERADD,'
	line += 'ClocksMax FILTERADD,'
	line += 'BytesAvg FILTERADD,'
	line += 'BytesMax FILTERADD,'
	line += '# FILTERCLEAR Msgs,'
	line += 'Num FILTERCLEAR PerSec,'
	line += 'Avg FILTERCLEAR PerSec,'
	line += 'ClocksAvg FILTERCLEAR,'
	line += 'ClocksMax FILTERCLEAR,'
	line += 'BytesAvg FILTERCLEAR,'
	line += 'BytesMax FILTERCLEAR,'
	line += '# SENDHEADERS Msgs,'
	line += 'Num SENDHEADERS PerSec,'
	line += 'Avg SENDHEADERS PerSec,'
	line += 'ClocksAvg SENDHEADERS,'
	line += 'ClocksMax SENDHEADERS,'
	line += 'BytesAvg SENDHEADERS,'
	line += 'BytesMax SENDHEADERS,'
	line += '# FEEFILTER Msgs,'
	line += 'Num FEEFILTER PerSec,'
	line += 'Avg FEEFILTER PerSec,'
	line += 'ClocksAvg FEEFILTER,'
	line += 'ClocksMax FEEFILTER,'
	line += 'BytesAvg FEEFILTER,'
	line += 'BytesMax FEEFILTER,'
	line += '# SENDCMPCT Msgs,'
	line += 'Num SENDCMPCT PerSec,'
	line += 'Avg SENDCMPCT PerSec,'
	line += 'ClocksAvg SENDCMPCT,'
	line += 'ClocksMax SENDCMPCT,'
	line += 'BytesAvg SENDCMPCT,'
	line += 'BytesMax SENDCMPCT,'
	line += '# CMPCTBLOCK Msgs,'
	line += 'Num CMPCTBLOCK PerSec,'
	line += 'Avg CMPCTBLOCK PerSec,'
	line += 'ClocksAvg CMPCTBLOCK,'
	line += 'ClocksMax CMPCTBLOCK,'
	line += 'BytesAvg CMPCTBLOCK,'
	line += 'BytesMax CMPCTBLOCK,'
	line += '# GETBLOCKTXN Msgs,'
	line += 'Num GETBLOCKTXN PerSec,'
	line += 'Avg GETBLOCKTXN PerSec,'
	line += 'ClocksAvg GETBLOCKTXN,'
	line += 'ClocksMax GETBLOCKTXN,'
	line += 'BytesAvg GETBLOCKTXN,'
	line += 'BytesMax GETBLOCKTXN,'
	line += '# BLOCKTXN Msgs,'
	line += 'Num BLOCKTXN PerSec,'
	line += 'Avg BLOCKTXN PerSec,'
	line += 'ClocksAvg BLOCKTXN,'
	line += 'ClocksMax BLOCKTXN,'
	line += 'BytesAvg BLOCKTXN,'
	line += 'BytesMax BLOCKTXN,'
	line += '# REJECT Msgs,'
	line += 'Num REJECT PerSec,'
	line += 'Avg REJECT PerSec,'
	line += 'ClocksAvg REJECT,'
	line += 'ClocksMax REJECT,'
	line += 'BytesAvg REJECT,'
	line += 'BytesMax REJECT,'
	line += '# [UNDOCUMENTED] Msgs,'
	line += 'Num [UNDOCUMENTED] PerSec,'
	line += 'Avg [UNDOCUMENTED] PerSec,'
	line += 'ClocksAvg [UNDOCUMENTED],'
	line += 'ClocksMax [UNDOCUMENTED],'
	line += 'BytesAvg [UNDOCUMENTED],'
	line += 'BytesMax [UNDOCUMENTED],'
	return line

prevNumMsgs = {}
prevClocksSum = {}
prevBytesSum = {}

numMsgsPerSecond = {} # Computing the rate
numMsgsPerSecondTime = {}
sumOfMsgsPerSecond = {} # Used to compute the average
countOfMsgsPerSecond = {} # Used to compute the average

# Compute the blocks per second and tx per second
lastBlockcount = 0
lastMempoolSize = 0
numAcceptedBlocksPerSec = 0
numAcceptedBlocksPerSecTime = 0
numAcceptedTxPerSec = 0
numAcceptedTxPerSecTime = 0
acceptedBlocksPerSecSum = 0
acceptedBlocksPerSecCount = 0
acceptedTxPerSecSum = 0
acceptedTxPerSecCount = 0

def parseMessage(message, string, time):
	line = ''
	numMsgs = re.findall(r'([0-9\.]+) msgs', string)[0]
	matches = re.findall(r'\[[0-9\., ]+\]', string)
	if len(matches) != 2:
		return None
	match1 = json.loads(matches[0])
	match2 = json.loads(matches[1])

	numMsgsDiff = int(numMsgs)
	if message in prevNumMsgs:
		numMsgsDiff = int(numMsgs) - int(prevNumMsgs[message])

	clocksSumDiff = int(match1[0])
	clocksAvgDiff = clocksSumDiff
	if message in prevClocksSum:
		clocksSumDiff = int(match1[0]) - int(prevClocksSum[message])
		clocksAvgDiff = 0
		if numMsgsDiff != 0:
			clocksAvgDiff = clocksSumDiff / numMsgsDiff

	bytesSumDiff = int(match2[0])
	bytesAvgDiff = bytesSumDiff
	if message in prevBytesSum:
		bytesSumDiff = int(match2[0]) - int(prevBytesSum[message])
		bytesAvgDiff = 0
		if numMsgsDiff != 0:
			bytesAvgDiff = bytesSumDiff / numMsgsDiff

	if numMsgsDiff != 0: # Calculate the messages per second = numMessages / (timestamp - timestampOfLastMessageOccurance)
		oldTime = 0
		if message in numMsgsPerSecondTime:
			oldTime = numMsgsPerSecondTime[message]

		numMsgsPerSecond[message] = numMsgsDiff / (time - oldTime)
		numMsgsPerSecondTime[message] = time

	msgsPerSecond = 0
	if message in numMsgsPerSecond: # If it exists, read it
		msgsPerSecond = numMsgsPerSecond[message]

	if message not in sumOfMsgsPerSecond: # Initialization
		sumOfMsgsPerSecond[message] = 0
		countOfMsgsPerSecond[message] = 0

	sumOfMsgsPerSecond[message] += msgsPerSecond
	countOfMsgsPerSecond[message] += 1

	line += str(numMsgs) + ','		# Num MSG

	line += str(msgsPerSecond) + ','	# Rate
	line += str(sumOfMsgsPerSecond[message] / countOfMsgsPerSecond[message]) + ','

	line += str(match1[1]) + ','	# Clocks Avg
	line += str(match1[2]) + ','	# Clocks Max
	line += str(match2[1]) + ','	# Bytes Avg
	line += str(match2[2])			# Bytes Max

	prevNumMsgs[message] = numMsgs
	prevClocksSum[message] = match1[0]
	prevBytesSum[message] = match2[0]
	return line


def fetch(now):
	global lastBlockcount, lastMempoolSize, numAcceptedBlocksPerSec, numAcceptedBlocksPerSecTime, numAcceptedTxPerSec, numAcceptedTxPerSecTime, acceptedBlocksPerSecSum, acceptedBlocksPerSecCount, acceptedTxPerSecSum, acceptedTxPerSecCount
	try:
		messages = json.loads(bitcoin('getmsginfo'))
		peerinfo = json.loads(bitcoin('getpeerinfo'))
		blockcount = int(bitcoin('getblockcount').strip())
		mempoolinfo = json.loads(bitcoin('getmempoolinfo'))
	except Exception as e:
		raise Exception('Unable to access Bitcoin console...')

	seconds = (now - datetime.datetime(1970, 1, 1)).total_seconds()
	numPeers = len(peerinfo)
	addresses = ''
	totalBanScore = 0
	totalPingTime = 0
	numPingTimes = 0
	noResponsePings = 0
	for peer in peerinfo:
		if addresses != '':
			addresses += ' '
		try:
			addresses += peer['addr']
			totalBanScore += peer['banscore']
		except:
			pass
		try:
			totalPingTime += peer['pingtime']
			numPingTimes += 1
		except:
			noResponsePings += 1


	if numPingTimes != 0:
		avgPingTime = totalPingTime / numPingTimes
	else:
		avgPingTime = '0'
	line = str(now) + ','
	line += str(seconds) + ','
	line += str(numPeers) + ','
	line += str(noResponsePings) + ','
	line += str(avgPingTime) + ','
	line += str(totalBanScore) + ','
	line += addresses + ','

	line += str(blockcount) + ','
	line += str(mempoolinfo['size']) + ','
	line += str(mempoolinfo['bytes']) + ','

	# Compute the blocks per second and tx per second
	blockcountDiff = blockcount - lastBlockcount
	mempoolSizeDiff = mempoolinfo['size'] - lastMempoolSize
	lastBlockcount = blockcount
	lastMempoolSize = mempoolinfo['size']

	#numMsgsDiff / (time - oldTime)
	if blockcountDiff != 0:
		numAcceptedBlocksPerSec = blockcountDiff / (seconds - numAcceptedBlocksPerSecTime)
		numAcceptedBlocksPerSecTime = seconds

	if mempoolSizeDiff != 0:
		numAcceptedTxPerSec = mempoolSizeDiff / (seconds - numAcceptedTxPerSecTime)
		numAcceptedTxPerSecTime = seconds

	acceptedBlocksPerSecSum += numAcceptedBlocksPerSec
	acceptedBlocksPerSecCount += 1
	acceptedTxPerSecSum += numAcceptedTxPerSec
	acceptedTxPerSecCount += 1

	line += str(numAcceptedBlocksPerSec) + ','
	line += str(acceptedBlocksPerSecSum / acceptedBlocksPerSecCount) + ','
	line += str(numAcceptedTxPerSec) + ','
	line += str(acceptedTxPerSecSum / acceptedTxPerSecCount) + ','

	line += str(messages['CLOCKS PER SECOND']) + ','
	line += parseMessage('VERSION', messages['VERSION'], seconds) + ','
	line += parseMessage('VERACK', messages['VERACK'], seconds) + ','
	line += parseMessage('ADDR', messages['ADDR'], seconds) + ','
	line += parseMessage('INV', messages['INV'], seconds) + ','
	line += parseMessage('GETDATA', messages['GETDATA'], seconds) + ','
	line += parseMessage('MERKLEBLOCK', messages['MERKLEBLOCK'], seconds) + ','
	line += parseMessage('GETBLOCKS', messages['GETBLOCKS'], seconds) + ','
	line += parseMessage('GETHEADERS', messages['GETHEADERS'], seconds) + ','
	line += parseMessage('TX', messages['TX'], seconds) + ','
	line += parseMessage('HEADERS', messages['HEADERS'], seconds) + ','
	line += parseMessage('BLOCK', messages['BLOCK'], seconds) + ','
	line += parseMessage('GETADDR', messages['GETADDR'], seconds) + ','
	line += parseMessage('MEMPOOL', messages['MEMPOOL'], seconds) + ','
	line += parseMessage('PING', messages['PING'], seconds) + ','
	line += parseMessage('PONG', messages['PONG'], seconds) + ','
	line += parseMessage('NOTFOUND', messages['NOTFOUND'], seconds) + ','
	line += parseMessage('FILTERLOAD', messages['FILTERLOAD'], seconds) + ','
	line += parseMessage('FILTERADD', messages['FILTERADD'], seconds) + ','
	line += parseMessage('FILTERCLEAR', messages['FILTERCLEAR'], seconds) + ','
	line += parseMessage('SENDHEADERS', messages['SENDHEADERS'], seconds) + ','
	line += parseMessage('FEEFILTER', messages['FEEFILTER'], seconds) + ','
	line += parseMessage('SENDCMPCT', messages['SENDCMPCT'], seconds) + ','
	line += parseMessage('CMPCTBLOCK', messages['CMPCTBLOCK'], seconds) + ','
	line += parseMessage('GETBLOCKTXN', messages['GETBLOCKTXN'], seconds) + ','
	line += parseMessage('BLOCKTXN', messages['BLOCKTXN'], seconds) + ','
	line += parseMessage('REJECT', messages['REJECT'], seconds) + ','
	line += parseMessage('[UNDOCUMENTED]', messages['[UNDOCUMENTED]'], seconds) + ','
	if numPeers != maxConnections:
		print(f'Connections at {numPeers}, waiting for it to reach {maxConnections}.')
		return ''
	return line

def resetNode(file, numConnections):
	global fileSampleNumber
	success = False
	while not success or bitcoinUp():
		try:
			print('Stopping bitcoin...')
			bitcoin('stop')
			success = True
		except:
			pass
		time.sleep(3)

	fileSampleNumber += 1
	try:
		file.close()
	except:
		pass
	try:
		file = open(os.path.expanduser(f'~/Desktop/Logs_GetMsgInfo_AlternatingConnections/Sample {fileSampleNumber} numConnections {numConnections}.csv'), 'w+')
		file.write(fetchHeader() + '\n')
	except:
		print('ERROR: Failed to create new file.')
	try:
		with open(datadir + '/bitcoin.conf', 'r') as configFile:
			configData = configFile.read()
		configData = re.sub(r'\s*maxconnections\s*=\s*[0-9]+', '', configData)
		configData += '\n' + 'maxconnections=' + str(numConnections)
		with open(datadir + '/bitcoin.conf', 'w') as configFile:
			configFile.write(configData)
	except:
		print('ERROR: Failed to update bitcoin.conf')

	success = False
	while not success:
		try:
			print('Starting Bitcoin...')
			startBitcoin()
			success = True
		except:
			pass

		time.sleep(3)
	if startupDelay > 0:
		print(f'Startup delay for {startupDelay} seconds...')
		time.sleep(startupDelay)
		print('Startup delay complete.')

	return file

def log(file, targetDateTime, count, maxConnections):
	global maxConnections, fileSampleNumber
	try:
		now = datetime.datetime.now()
		print(f'Line: {str(count)}, File: {fileSampleNumber}, Max Connections: {maxConnections}, Off by {(now - targetDateTime).total_seconds()} seconds.')
		line = fetch(now)
		if len(line) != 0:
			file.write(line + '\n')
			file.flush()
		#if count >= 3600:
		#	file.close()
		#	return
		if(count % rowsPerNodeReset == 0):
			maxConnections = 1 + maxConnections % 10 # Bound the max connections to 10 peers
			try:
				file = resetNode(file, maxConnections)
			except Exception as e:
				print('ERROR: ' + e)

			targetDateTime = datetime.datetime.now()

		count += 1
	except Exception as e:
		print('\nLOGGER PAUSED. Hold on...')
		print(e)
		print(traceback.format_exc())


	targetDateTime = targetDateTime + datetime.timedelta(seconds = numSecondsPerSample)
	delay = getDelay(targetDateTime)
	Timer(delay, log, [file, targetDateTime, count, maxConnections]).start()

def getDelay(time):
	return (time - datetime.datetime.now()).total_seconds()

def init():
	global fileSampleNumber
	global maxConnections
	fileSampleNumber = 0		# File number to start at
	while len(glob.glob(os.path.expanduser(f'~/Desktop/Logs_GetMsgInfo_AlternatingConnections/Sample {fileSampleNumber + 1} *'))) > 0:
		fileSampleNumber += 1

	filesList = '\n'.join(glob.glob(os.path.expanduser('~/Desktop/Logs_GetMsgInfo_AlternatingConnections/Sample *')))
	filesList = filesList.replace(os.path.expanduser('~/Desktop/Logs_GetMsgInfo_AlternatingConnections/'), '')
	print(filesList)
	print()
	maxConnections = int(input(f'Starting at file "Sample {fileSampleNumber + 1} numConnections X.csv"\nHow many connections should be made? (From 1 to 10): '))
	print()
	path = os.path.expanduser('~/Desktop/Logs_GetMsgInfo_AlternatingConnections')
	if not os.path.exists(path):
	    os.makedirs(path)

	file = resetNode(None, maxConnections)#open(os.path.expanduser(f'~/Desktop/Logs_GetMsgInfo_AlternatingConnections/sample0.csv'), 'w+')
	targetDateTime = datetime.datetime.now()
	log(file, targetDateTime, 1, maxConnections)


init()
