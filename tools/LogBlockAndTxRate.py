import os
import re
import json
import datetime
from threading import Timer

numSecondsPerSample = 0.1

#def getmyIP():
#	return os.popen('curl \'http://myexternalip.com/raw\'').read() + ':8333'

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

	line += 'Connections,'

	line += 'ClocksPerSec,'
	line += '# TX,'
	line += '# TX Diff,'
	line += 'TX ClocksSum Diff,'
	line += 'TX ClocksAvg Diff,'
	line += 'TX ClocksAvg,'
	line += 'TX ClocksMax,'
	line += 'TX BytesSum Diff,'
	line += 'TX BytesAvg Diff,'
	line += 'TX BytesAvg,'
	line += 'TX BytesMax,'
	line += '# BLOCK,'
	line += '# BLOCK Diff,'
	line += 'BLOCK ClocksSum Diff,'
	line += 'BLOCK ClocksAvg Diff,'
	line += 'BLOCK ClocksAvg,'
	line += 'BLOCK ClocksMax,'
	line += 'BLOCK BytesSum Diff,'
	line += 'BLOCK BytesAvg Diff,'
	line += 'BLOCK BytesAvg,'
	line += 'BLOCK BytesMax,'
	line += '# CMPCTBLOCK,'
	line += '# CMPCTBLOCK Diff,'
	line += 'CMPCTBLOCK ClocksSum Diff,'
	line += 'CMPCTBLOCK ClocksAvg Diff,'
	line += 'CMPCTBLOCK ClocksAvg,'
	line += 'CMPCTBLOCK ClocksMax,'
	line += 'CMPCTBLOCK BytesSum Diff,'
	line += 'CMPCTBLOCK BytesAvg Diff,'
	line += 'CMPCTBLOCK BytesAvg,'
	line += 'CMPCTBLOCK BytesMax,'
	return line

prevNumMsgs = {}
prevClocksSum = {}
prevBytesSum = {}

def parseMessage(message, string):
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

	line += str(numMsgs) + ','		# Num
	line += str(numMsgsDiff) + ','	# Num Diff

	line += str(clocksSumDiff) + ','
	line += str(clocksAvgDiff) + ','

	line += str(match1[1]) + ','	# ClocksAvg
	line += str(match1[2]) + ','	# ClocksMax

	line += str(bytesSumDiff) + ','
	line += str(bytesAvgDiff) + ','

	line += str(match2[1]) + ','	# BytesAvg
	line += str(match2[2])			# BytesMax

	prevNumMsgs[message] = numMsgs
	prevClocksSum[message] = match1[0]
	prevBytesSum[message] = match2[0]
	return line


def fetch():
	now = datetime.datetime.now()
	messages = json.loads(bitcoin('getmsginfo'))
	peerinfo = json.loads(bitcoin('getpeerinfo'))
	blockcount = bitcoin('getblockcount').strip()
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
	line = str(now) + ','
	line += str((now - datetime.datetime(1970, 1, 1)).total_seconds()) + ','
	line += str(numPeers) + ','
	line += str(noResponsePings) + ','
	line += str(avgPingTime) + ','
	line += str(totalBanScore) + ','

	line += str(blockcount) + ','
	line += str(mempoolinfo['size']) + ','
	line += str(mempoolinfo['bytes']) + ','

	#if 'size' in mempoolinfo: line += str(mempoolinfo['size']) + ','
	#else: line += ','
	#if 'bytes' in mempoolinfo: line += str(mempoolinfo['bytes']) + ','
	#else: line += ','

	line += addresses + ','

	line += str(messages['CLOCKS PER SECOND']) + ','
	line += parseMessage('TX', messages['TX']) + ','
	line += parseMessage('BLOCK', messages['BLOCK']) + ','
	line += parseMessage('CMPCTBLOCK', messages['CMPCTBLOCK']) + ','
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
	except:
		print('\nLOGGER PAUSED. Start the Bitcoin node when you are ready.')
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
