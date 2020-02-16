import os
import json
import datetime
from threading import Timer

numSecondsPerSample = 5

#def getmyIP():
#	return os.popen("curl \"http://myexternalip.com/raw\"").read() + ":8333"

def bitcoin(cmd):
	return os.popen("\"C:\\Program Files\\Bitcoin\\daemon\\bitcoin-cli\" -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 " + cmd).read()

def fetchMempoolHeader():
	return ""
	"""header = "Timestamp\t"
	mempool = json.loads(bitcoin("getmempoolinfo"))
	for e in mempool:
		header += e.capitalize() + "\t"
	header.strip()
	return header"""

def fetchMempool():
	line = str(datetime.datetime.now()) + ","
	mempool = json.loads(bitcoin("getrawmempool"))
	peerinfo = json.loads(bitcoin("getpeerinfo"))
	numPeers = len(peerinfo)
	addresses = '' #my_ip # Append my external ip address to the front, so we can detect direct connections
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

	"""for tx in mempool:
		info = json.loads(bitcoin('getmempoolentry "' + tx + '"'))

		if(tx != mempool[0]):
			line += ","
		line = line + '"' + str(info) + '"'
	"""
	if numPingTimes != 0:
		avgPingTime = totalPingTime / numPingTimes
	else:
		avgPingTime = "N/A"
	line += str(numPeers) + ","
	line += str(totalPingTime) + ","
	line += str(avgPingTime) + ","
	line += str(noResponsePings) + ","
	line += str(totalBanScore) + ","
	line += addresses + ","
	for i in range(len(mempool)):
		mempool[i] = mempool[i][:6] # Chance of collision: 1 / 16777216
	line += ",".join(mempool)
	return line
	"""line = str(datetime.datetime.now()) + "\t"
	mempool = json.loads(bitcoin("get"))
	for e in mempool:
		line += str(mempool[e]) + "\t"
	line.strip()
	return line"""

def log(file, targetDateTime, count = 1):
	"""if count % 10 == 0:
		print("Line " + str(count))
		file.flush()"""
	#if(count % (3600 / numSecondsPerSample) == 0): # Get my external ip every hour
	#	my_ip = getmyIP()
	file.write(fetchMempool() + "\n")
	print("Line " + str(count))
	file.flush()
	#if count >= 3600:
	#	file.close()
	#	return
	count += 1
	targetDateTime = targetDateTime + datetime.timedelta(seconds = numSecondsPerSample)
	delay = getDelay(targetDateTime)
	Timer(delay, log, [file, targetDateTime, count]).start()
	# 1000 / 50 / 1000 = 50 times per second
	# Timer(1000 / 4 / 1000, log, [file, count]).start()

def getDelay(time):
	return (time - datetime.datetime.now()).total_seconds()

time = input("Enter what time it should begin (example: \"1:33pm\"): ")
file = open("SynchronizedMemPoolLog.csv", "w+")
#file.write(fetchMempoolHeader() + "\n")
targetDateTime = datetime.datetime.strptime(datetime.datetime.now().strftime('%d-%m-%Y') + " " + time, '%d-%m-%Y %I:%M%p')
#my_ip = getmyIP()
delay = getDelay(targetDateTime)
Timer(delay, log, [file, targetDateTime, 1]).start()

if(delay > 60):
	print("Time left: " + str(delay / 60) + " minutes.")
else:
	print("Time left: " + str(delay) + " seconds.")

#print(fetchMempoolHeader())
#print(fetchMempool())