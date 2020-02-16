import os
import re
import json
import datetime
from threading import Timer

numSecondsPerSample = 1

#def getmyIP():
#	return os.popen("curl \"http://myexternalip.com/raw\"").read() + ":8333"

def bitcoin(cmd):
	return os.popen("./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 " + cmd).read()

def fetchHeader():
	line = "Timestamp,"
	line += "Timestamp (Seconds),"
	line += "NumPeers,"
	line += "InactivePeers,"
	line += "AvgPingTime,"
	line += "TotalBanScore,"
	line += "Connections,"
	line += "ClocksPerSec,"

	line += "# VERSION,"
	line += "# VERSION Diff,"
	line += "VERSION ClocksSum Diff,"
	line += "VERSION ClocksAvg Diff,"
	line += "VERSION ClocksAvg,"
	line += "VERSION ClocksMax,"
	line += "VERSION BytesSum Diff,"
	line += "VERSION BytesAvg Diff,"
	line += "VERSION BytesAvg,"
	line += "VERSION BytesMax,"
	line += "# VERACK,"
	line += "# VERACK Diff,"
	line += "VERACK ClocksSum Diff,"
	line += "VERACK ClocksAvg Diff,"
	line += "VERACK ClocksAvg,"
	line += "VERACK ClocksMax,"
	line += "VERACK BytesSum Diff,"
	line += "VERACK BytesAvg Diff,"
	line += "VERACK BytesAvg,"
	line += "VERACK BytesMax,"
	line += "# ADDR,"
	line += "# ADDR Diff,"
	line += "ADDR ClocksSum Diff,"
	line += "ADDR ClocksAvg Diff,"
	line += "ADDR ClocksAvg,"
	line += "ADDR ClocksMax,"
	line += "ADDR BytesSum Diff,"
	line += "ADDR BytesAvg Diff,"
	line += "ADDR BytesAvg,"
	line += "ADDR BytesMax,"
	line += "# INV,"
	line += "# INV Diff,"
	line += "INV ClocksSum Diff,"
	line += "INV ClocksAvg Diff,"
	line += "INV ClocksAvg,"
	line += "INV ClocksMax,"
	line += "INV BytesSum Diff,"
	line += "INV BytesAvg Diff,"
	line += "INV BytesAvg,"
	line += "INV BytesMax,"
	line += "# GETDATA,"
	line += "# GETDATA Diff,"
	line += "GETDATA ClocksSum Diff,"
	line += "GETDATA ClocksAvg Diff,"
	line += "GETDATA ClocksAvg,"
	line += "GETDATA ClocksMax,"
	line += "GETDATA BytesSum Diff,"
	line += "GETDATA BytesAvg Diff,"
	line += "GETDATA BytesAvg,"
	line += "GETDATA BytesMax,"
	line += "# MERKLEBLOCK,"
	line += "# MERKLEBLOCK Diff,"
	line += "MERKLEBLOCK ClocksSum Diff,"
	line += "MERKLEBLOCK ClocksAvg Diff,"
	line += "MERKLEBLOCK ClocksAvg,"
	line += "MERKLEBLOCK ClocksMax,"
	line += "MERKLEBLOCK BytesSum Diff,"
	line += "MERKLEBLOCK BytesAvg Diff,"
	line += "MERKLEBLOCK BytesAvg,"
	line += "MERKLEBLOCK BytesMax,"
	line += "# GETBLOCKS,"
	line += "# GETBLOCKS Diff,"
	line += "GETBLOCKS ClocksSum Diff,"
	line += "GETBLOCKS ClocksAvg Diff,"
	line += "GETBLOCKS ClocksAvg,"
	line += "GETBLOCKS ClocksMax,"
	line += "GETBLOCKS BytesSum Diff,"
	line += "GETBLOCKS BytesAvg Diff,"
	line += "GETBLOCKS BytesAvg,"
	line += "GETBLOCKS BytesMax,"
	line += "# GETHEADERS,"
	line += "# GETHEADERS Diff,"
	line += "GETHEADERS ClocksSum Diff,"
	line += "GETHEADERS ClocksAvg Diff,"
	line += "GETHEADERS ClocksAvg,"
	line += "GETHEADERS ClocksMax,"
	line += "GETHEADERS BytesSum Diff,"
	line += "GETHEADERS BytesAvg Diff,"
	line += "GETHEADERS BytesAvg,"
	line += "GETHEADERS BytesMax,"
	line += "# TX,"
	line += "# TX Diff,"
	line += "TX ClocksSum Diff,"
	line += "TX ClocksAvg Diff,"
	line += "TX ClocksAvg,"
	line += "TX ClocksMax,"
	line += "TX BytesSum Diff,"
	line += "TX BytesAvg Diff,"
	line += "TX BytesAvg,"
	line += "TX BytesMax,"
	line += "# HEADERS,"
	line += "# HEADERS Diff,"
	line += "HEADERS ClocksSum Diff,"
	line += "HEADERS ClocksAvg Diff,"
	line += "HEADERS ClocksAvg,"
	line += "HEADERS ClocksMax,"
	line += "HEADERS BytesSum Diff,"
	line += "HEADERS BytesAvg Diff,"
	line += "HEADERS BytesAvg,"
	line += "HEADERS BytesMax,"
	line += "# BLOCK,"
	line += "# BLOCK Diff,"
	line += "BLOCK ClocksSum Diff,"
	line += "BLOCK ClocksAvg Diff,"
	line += "BLOCK ClocksAvg,"
	line += "BLOCK ClocksMax,"
	line += "BLOCK BytesSum Diff,"
	line += "BLOCK BytesAvg Diff,"
	line += "BLOCK BytesAvg,"
	line += "BLOCK BytesMax,"
	line += "# GETADDR,"
	line += "# GETADDR Diff,"
	line += "GETADDR ClocksSum Diff,"
	line += "GETADDR ClocksAvg Diff,"
	line += "GETADDR ClocksAvg,"
	line += "GETADDR ClocksMax,"
	line += "GETADDR BytesSum Diff,"
	line += "GETADDR BytesAvg Diff,"
	line += "GETADDR BytesAvg,"
	line += "GETADDR BytesMax,"
	line += "# MEMPOOL,"
	line += "# MEMPOOL Diff,"
	line += "MEMPOOL ClocksSum Diff,"
	line += "MEMPOOL ClocksAvg Diff,"
	line += "MEMPOOL ClocksAvg,"
	line += "MEMPOOL ClocksMax,"
	line += "MEMPOOL BytesSum Diff,"
	line += "MEMPOOL BytesAvg Diff,"
	line += "MEMPOOL BytesAvg,"
	line += "MEMPOOL BytesMax,"
	line += "# PING,"
	line += "# PING Diff,"
	line += "PING ClocksSum Diff,"
	line += "PING ClocksAvg Diff,"
	line += "PING ClocksAvg,"
	line += "PING ClocksMax,"
	line += "PING BytesSum Diff,"
	line += "PING BytesAvg Diff,"
	line += "PING BytesAvg,"
	line += "PING BytesMax,"
	line += "# PONG,"
	line += "# PONG Diff,"
	line += "PONG ClocksSum Diff,"
	line += "PONG ClocksAvg Diff,"
	line += "PONG ClocksAvg,"
	line += "PONG ClocksMax,"
	line += "PONG BytesSum Diff,"
	line += "PONG BytesAvg Diff,"
	line += "PONG BytesAvg,"
	line += "PONG BytesMax,"
	line += "# NOTFOUND,"
	line += "# NOTFOUND Diff,"
	line += "NOTFOUND ClocksSum Diff,"
	line += "NOTFOUND ClocksAvg Diff,"
	line += "NOTFOUND ClocksAvg,"
	line += "NOTFOUND ClocksMax,"
	line += "NOTFOUND BytesSum Diff,"
	line += "NOTFOUND BytesAvg Diff,"
	line += "NOTFOUND BytesAvg,"
	line += "NOTFOUND BytesMax,"
	line += "# FILTERLOAD,"
	line += "# FILTERLOAD Diff,"
	line += "FILTERLOAD ClocksSum Diff,"
	line += "FILTERLOAD ClocksAvg Diff,"
	line += "FILTERLOAD ClocksAvg,"
	line += "FILTERLOAD ClocksMax,"
	line += "FILTERLOAD BytesSum Diff,"
	line += "FILTERLOAD BytesAvg Diff,"
	line += "FILTERLOAD BytesAvg,"
	line += "FILTERLOAD BytesMax,"
	line += "# FILTERADD,"
	line += "# FILTERADD Diff,"
	line += "FILTERADD ClocksSum Diff,"
	line += "FILTERADD ClocksAvg Diff,"
	line += "FILTERADD ClocksAvg,"
	line += "FILTERADD ClocksMax,"
	line += "FILTERADD BytesSum Diff,"
	line += "FILTERADD BytesAvg Diff,"
	line += "FILTERADD BytesAvg,"
	line += "FILTERADD BytesMax,"
	line += "# FILTERCLEAR,"
	line += "# FILTERCLEAR Diff,"
	line += "FILTERCLEAR ClocksSum Diff,"
	line += "FILTERCLEAR ClocksAvg Diff,"
	line += "FILTERCLEAR ClocksAvg,"
	line += "FILTERCLEAR ClocksMax,"
	line += "FILTERCLEAR BytesSum Diff,"
	line += "FILTERCLEAR BytesAvg Diff,"
	line += "FILTERCLEAR BytesAvg,"
	line += "FILTERCLEAR BytesMax,"
	line += "# SENDHEADERS,"
	line += "# SENDHEADERS Diff,"
	line += "SENDHEADERS ClocksSum Diff,"
	line += "SENDHEADERS ClocksAvg Diff,"
	line += "SENDHEADERS ClocksAvg,"
	line += "SENDHEADERS ClocksMax,"
	line += "SENDHEADERS BytesSum Diff,"
	line += "SENDHEADERS BytesAvg Diff,"
	line += "SENDHEADERS BytesAvg,"
	line += "SENDHEADERS BytesMax,"
	line += "# FEEFILTER,"
	line += "# FEEFILTER Diff,"
	line += "FEEFILTER ClocksSum Diff,"
	line += "FEEFILTER ClocksAvg Diff,"
	line += "FEEFILTER ClocksAvg,"
	line += "FEEFILTER ClocksMax,"
	line += "FEEFILTER BytesSum Diff,"
	line += "FEEFILTER BytesAvg Diff,"
	line += "FEEFILTER BytesAvg,"
	line += "FEEFILTER BytesMax,"
	line += "# SENDCMPCT,"
	line += "# SENDCMPCT Diff,"
	line += "SENDCMPCT ClocksSum Diff,"
	line += "SENDCMPCT ClocksAvg Diff,"
	line += "SENDCMPCT ClocksAvg,"
	line += "SENDCMPCT ClocksMax,"
	line += "SENDCMPCT BytesSum Diff,"
	line += "SENDCMPCT BytesAvg Diff,"
	line += "SENDCMPCT BytesAvg,"
	line += "SENDCMPCT BytesMax,"
	line += "# CMPCTBLOCK,"
	line += "# CMPCTBLOCK Diff,"
	line += "CMPCTBLOCK ClocksSum Diff,"
	line += "CMPCTBLOCK ClocksAvg Diff,"
	line += "CMPCTBLOCK ClocksAvg,"
	line += "CMPCTBLOCK ClocksMax,"
	line += "CMPCTBLOCK BytesSum Diff,"
	line += "CMPCTBLOCK BytesAvg Diff,"
	line += "CMPCTBLOCK BytesAvg,"
	line += "CMPCTBLOCK BytesMax,"
	line += "# GETBLOCKTXN,"
	line += "# GETBLOCKTXN Diff,"
	line += "GETBLOCKTXN ClocksSum Diff,"
	line += "GETBLOCKTXN ClocksAvg Diff,"
	line += "GETBLOCKTXN ClocksAvg,"
	line += "GETBLOCKTXN ClocksMax,"
	line += "GETBLOCKTXN BytesSum Diff,"
	line += "GETBLOCKTXN BytesAvg Diff,"
	line += "GETBLOCKTXN BytesAvg,"
	line += "GETBLOCKTXN BytesMax,"
	line += "# BLOCKTXN,"
	line += "# BLOCKTXN Diff,"
	line += "BLOCKTXN ClocksSum Diff,"
	line += "BLOCKTXN ClocksAvg Diff,"
	line += "BLOCKTXN ClocksAvg,"
	line += "BLOCKTXN ClocksMax,"
	line += "BLOCKTXN BytesSum Diff,"
	line += "BLOCKTXN BytesAvg Diff,"
	line += "BLOCKTXN BytesAvg,"
	line += "BLOCKTXN BytesMax,"
	line += "# [UNDOCUMENTED],"
	line += "# [UNDOCUMENTED] Diff,"
	line += "[UNDOCUMENTED] ClocksSum Diff,"
	line += "[UNDOCUMENTED] ClocksAvg Diff,"
	line += "[UNDOCUMENTED] ClocksAvg,"
	line += "[UNDOCUMENTED] ClocksMax,"
	line += "[UNDOCUMENTED] BytesSum Diff,"
	line += "[UNDOCUMENTED] BytesAvg Diff,"
	line += "[UNDOCUMENTED] BytesAvg,"
	line += "[UNDOCUMENTED] BytesMax,"
	return line

prevNumMsgs = {}
prevClocksSum = {}
prevBytesSum = {}

def parseMessage(message, string):
	line = ''
	numMsgs = re.findall(r"([0-9\.]+) msgs", string)[0]
	matches = re.findall(r"\[[0-9\., ]+\]", string)
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

	"""
	line = "Timestamp,"
	line += "Timestamp (Seconds),"
	line += "NumPeers,"
	line += "InactivePeers,"
	line += "AvgPingTime,"
	line += "TotalBanScore,"
	line += "Connections,"
	line += "ClocksPerSec,"

	line += "# VERSION,"
	line += "# VERSION Diff,"
	line += "VERSION ClocksSum Diff,"
	line += "VERSION ClocksAvg Diff,"
	line += "VERSION ClocksAvg,"
	line += "VERSION ClocksMax,"
	line += "VERSION BytesSum Diff,"
	line += "VERSION BytesAvg Diff,"
	line += "VERSION BytesAvg,"
	line += "VERSION BytesMax,"
	"""

	prevNumMsgs[message] = numMsgs
	prevClocksSum[message] = match1[0]
	prevBytesSum[message] = match2[0]
	return line


def fetch():
	now = datetime.datetime.now()
	messages = json.loads(bitcoin("getmsginfo"))
	peerinfo = json.loads(bitcoin("getpeerinfo"))
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
			addresses += peer["addr"]
			totalBanScore += peer["banscore"]
		except:
			pass
		try:
			totalPingTime += peer["pingtime"]
			numPingTimes += 1
		except:
			noResponsePings += 1


	if numPingTimes != 0:
		avgPingTime = totalPingTime / numPingTimes
	else:
		avgPingTime = "N/A"
	line = str(now) + ","
	line += str((now - datetime.datetime(1970, 1, 1)).total_seconds()) + ","
	line += str(numPeers) + ","
	line += str(noResponsePings) + ","
	line += str(avgPingTime) + ","
	line += str(totalBanScore) + ","
	line += addresses + ","

	line += str(messages["CLOCKS PER SECOND"]) + ","
	line += parseMessage("VERSION", messages["VERSION"]) + ","
	line += parseMessage("VERACK", messages["VERACK"]) + ","
	line += parseMessage("ADDR", messages["ADDR"]) + ","
	line += parseMessage("INV", messages["INV"]) + ","
	line += parseMessage("GETDATA", messages["GETDATA"]) + ","
	line += parseMessage("MERKLEBLOCK", messages["MERKLEBLOCK"]) + ","
	line += parseMessage("GETBLOCKS", messages["GETBLOCKS"]) + ","
	line += parseMessage("GETHEADERS", messages["GETHEADERS"]) + ","
	line += parseMessage("TX", messages["TX"]) + ","
	line += parseMessage("HEADERS", messages["HEADERS"]) + ","
	line += parseMessage("BLOCK", messages["BLOCK"]) + ","
	line += parseMessage("GETADDR", messages["GETADDR"]) + ","
	line += parseMessage("MEMPOOL", messages["MEMPOOL"]) + ","
	line += parseMessage("PING", messages["PING"]) + ","
	line += parseMessage("PONG", messages["PONG"]) + ","
	line += parseMessage("NOTFOUND", messages["NOTFOUND"]) + ","
	line += parseMessage("FILTERLOAD", messages["FILTERLOAD"]) + ","
	line += parseMessage("FILTERADD", messages["FILTERADD"]) + ","
	line += parseMessage("FILTERCLEAR", messages["FILTERCLEAR"]) + ","
	line += parseMessage("SENDHEADERS", messages["SENDHEADERS"]) + ","
	line += parseMessage("FEEFILTER", messages["FEEFILTER"]) + ","
	line += parseMessage("SENDCMPCT", messages["SENDCMPCT"]) + ","
	line += parseMessage("CMPCTBLOCK", messages["CMPCTBLOCK"]) + ","
	line += parseMessage("GETBLOCKTXN", messages["GETBLOCKTXN"]) + ","
	line += parseMessage("BLOCKTXN", messages["BLOCKTXN"]) + ","
	line += parseMessage("[UNDOCUMENTED]", messages["[UNDOCUMENTED]"]) + ","
	return line

def log(file, targetDateTime, count = 1):
	try:
		file.write(fetch() + "\n")
		print("Line " + str(count))
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

time = input("Enter what time it should begin (example: \"2:45pm\", \"\" for right now): ")
file = open(os.path.expanduser("~/Desktop/GetMsgInfoLog.csv"), "w+")

if time == "":
	targetDateTime = datetime.datetime.now()
	delay = 0
else:
	targetDateTime = datetime.datetime.strptime(datetime.datetime.now().strftime('%d-%m-%Y') + " " + time, '%d-%m-%Y %I:%M%p')
	delay = getDelay(targetDateTime)

file.write(fetchHeader() + "\n")
Timer(delay, log, [file, targetDateTime, 1]).start()

if(delay > 60):
	print("Time left: " + str(delay / 60) + " minutes.")
else:
	print("Time left: " + str(delay) + " seconds.")
