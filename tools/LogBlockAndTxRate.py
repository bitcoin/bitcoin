import os
import re
import json
import datetime
from threading import Timer

numSecondsPerSample = 0.1


def bitcoin(cmd):
	return os.popen('./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

def fetchHeader():
	line = 'Timestamp,'
	line += 'Timestamp (Seconds),'
	line += 'NumPeers,'
	line += 'InactivePeers,'
	line += 'AvgPingTime,'
	line += 'TotalBanScore,'

	line += 'BlockHeight,'
	line += 'MempoolSize,'
	line += 'MempoolBytes,'
	line += 'AcceptedBlocksPerSec'
	line += 'AcceptedBlocksPerSecAvg'
	line += 'AcceptedTxPerSec'
	line += 'AcceptedTxPerSecAvg'

	line += 'NumTxMsgs,'
	line += 'TxsPerSec,'
	line += 'TxsPerSecAvg,'
	line += 'NumBlockMsgs,'
	line += 'BlocksPerSec,'
	line += 'BlocksPerSecAvg,'
	line += 'NumCmpctBlockMsgs,'
	line += 'CMPCTBLOCKs Per Sec Avg,'

	line += 'Connections,'
	return line

prevNumMsgs = {}
numMsgsPerSecond = {}
numMsgsPerSecondTime = {}

sumOfMsgsPerSecond = {} # Used to compute the average
countOfMsgsPerSecond = {} # Used to compute the average

def parseMessage(message, string, time):
	line = ''
	numMsgs = re.findall(r'([0-9\.]+) msgs', string)[0]

	numMsgsDiff = int(numMsgs)
	if message in prevNumMsgs:
		numMsgsDiff = int(numMsgs) - int(prevNumMsgs[message])

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

	line += str(numMsgs) + ','		# Num
	line += str(msgsPerSecond) + ','	# Rate
	line += str(sumOfMsgsPerSecond[message] / countOfMsgsPerSecond[message])		# Rate average


	prevNumMsgs[message] = numMsgs
	return line


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

def fetch():
	now = datetime.datetime.now()
	messages = json.loads(bitcoin('getmsginfo'))
	peerinfo = json.loads(bitcoin('getpeerinfo'))
	blockcount = int(bitcoin('getblockcount').strip())
	mempoolinfo = json.loads(bitcoin('getmempoolinfo'))
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
		avgPingTime = 'N/A'

	seconds = (now - datetime.datetime(1970, 1, 1)).total_seconds()

	line = str(now) + ','
	line += str(seconds) + ','
	line += str(numPeers) + ','
	line += str(noResponsePings) + ','
	line += str(avgPingTime) + ','
	line += str(totalBanScore) + ','

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


	line += parseMessage('TX', messages['TX'], seconds) + ','
	line += parseMessage('BLOCK', messages['BLOCK'], seconds) + ','
	line += parseMessage('CMPCTBLOCK', messages['CMPCTBLOCK'], seconds) + ','

	line += addresses + ','
	return line

def log(file, targetDateTime, count = 1):
	try:
		file.write(fetch() + '\n')
		print('Line ' + str(count))
		file.flush()
		#if count >= 3600:
		#	file.close()
		#	return
		count += 1
	except Exception as e:
		print(f'\nLOGGER PAUSED. Start the Bitcoin node when you are ready. ({e})')
		pass
	targetDateTime = targetDateTime + datetime.timedelta(seconds = numSecondsPerSample)
	delay = getDelay(targetDateTime)
	Timer(delay, log, [file, targetDateTime, count]).start()

def getDelay(time):
	return (time - datetime.datetime.now()).total_seconds()

time = input('Enter what time it should begin (example: \'2:45pm\', \'\' for right now): ')
file = open(os.path.expanduser('~/Desktop/GetMsgInfoLog.csv'), 'w+')

if time == '':
	targetDateTime = datetime.datetime.now()
	delay = 0
else:
	targetDateTime = datetime.datetime.strptime(datetime.datetime.now().strftime('%d-%m-%Y') + ' ' + time, '%d-%m-%Y %I:%M%p')
	delay = getDelay(targetDateTime)

file.write(fetchHeader() + '\n')
Timer(delay, log, [file, targetDateTime, 1]).start()

if(delay > 60):
	print('Time left: ' + str(delay / 60) + ' minutes.')
else:
	print('Time left: ' + str(delay) + ' seconds.')
