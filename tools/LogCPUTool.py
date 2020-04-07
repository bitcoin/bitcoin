import os
import sys
import re
import json
import time
import datetime
import threading

numSamples = 100
mineCommand = 'mine 1 seconds'

def bitcoin(cmd):
	return os.popen('./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

def terminal(cmd):
	return os.popen(cmd).read()

def getPID():
	ps = terminal('ps -a')
	t = re.search(r'([0-9]+).+bitcoind', ps)
	pid = -1
	if t != None:
		pid = int(t.group(1))
	print(f'Bitcoin process ID: {pid}')
	return pid

def getCurrentResources(pid):
	top = terminal(f'top -p {pid} -n 1 -b')
	t = re.search(r'[^ ] +([0-9\.]+) +([0-9\.]+) +[0-9:\.]+ +bitcoind', top)
	if t != None:
		return (t.group(1), t.group(2))
	return ('', '')

def fetchHeader():
	line = 'Timestamp,'
	line += 'Timestamp (Seconds),'
	line += 'Target CPU,'
	line += 'Current CPU,'
	line += 'Current Memory,'
	line += 'Number of hashes,'
	line += 'Internal Time,'
	line += 'External Time,'
	line += 'Internal Hashes Per Second,'
	line += 'External Hashes Per Second,'
	return line + '\n'

# Used to create threads
class CPUToolThreadManager(threading.Thread):
	def __init__(self,  *args, **kwargs):
		super(CPUToolThreadManager, self).__init__(*args, **kwargs)
		self._stop_event = threading.Event()

	def run(self):
		global bitcoindPID, CPUPercent

		print(f'CPU percentage set to {CPUPercent}')
		#print(f'Starting {self.getName()}')
		terminal(f'cputool -p {bitcoindPID} --cpu-limit {CPUPercent}')

	def stop(self):
		print('Stopping...')
		self._stop_event.set()

	def isRunning():
		return self._stop_event.is_set()

	
	



# MAIN
if __name__ == '__main__':
	global fileName, bitcoindPID, CPUPercent
	fileName = 'CPUToolLog.csv'
	
	bitcoindPID = getPID()
	if bitcoindPID == -1:
		print('Bitcoin is not running. Please run this script when bitcoin is running.')
		sys.exit()

	cputool = CPUToolThreadManager(name = 'CPUTool Thread Manager')
	# cputool.daemon = True

	file = open(os.path.expanduser(f'~/Desktop/{fileName}'), 'w+')
	file.write(fetchHeader())

	CPUStepSize = 1
	count = 0

	CPUPercent = 50
	cputool.start()

	for CPUPercent in range(100, -1, -CPUStepSize):
		# Start thread...?

		for j in range(1, numSamples + 1):
			count += 1
			t1 = time.perf_counter()
			mineResponseStr = bitcoin(mineCommand)
			t2 = time.perf_counter()
			cpuUsage, memUsage = getCurrentResources(bitcoindPID)

			mineResponse = json.loads(mineResponseStr)
			#hashesPerSecond = mineResponse['Hashes per second']
			numHashes = mineResponse['Number of hashes']
			internalSeconds = mineResponse['Elapsed time (seconds)']
			externalSeconds = t2 - t1

			now = datetime.datetime.now()
			line = str(now) + ','
			line += str((now - datetime.datetime(1970, 1, 1)).total_seconds()) + ','
			line += str(CPUPercent) + ','
			line += str(cpuUsage) + ','
			line += str(memUsage) + ','
			line += str(numHashes) + ','
			line += str(internalSeconds) + ','
			line += str(externalSeconds) + ','
			line += str(numHashes / internalSeconds) + ','
			line += str(numHashes / externalSeconds) + ','

			line += '\n'
			file.write(line)
			print(f'Sample {count} PID={bitcoindPID} CPU={cpuUsage}')

		#cputool.stop()