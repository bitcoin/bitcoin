import os
import time
import datetime
from subprocess import *

# Send a command to the linux terminal
def terminal(cmd):
	return os.popen(cmd).read().strip()

# Send a command to the Bitcoin console
def bitcoin(cmd):
	return terminal('./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd)

def header():
	line = 'Timestamp,'
	line += 'Timestamp (Seconds),'
	line += 'NumPeers,'
	return line + '\n'

def log():
	now = datetime.datetime.now()
	now_seconds = (now - datetime.datetime(1970, 1, 1)).total_seconds()
	numpeers = bitcoin('getconnectioncount')
	#if numpeers == '': numpeers = 0
	
	line = str(now) + ','
	line += str(now_seconds) + ','
	line += str(numpeers) + ','
	return line + '\n'

fileName = 'LinuxLoggingConnections.csv'
filePath = os.path.join(os.path.expanduser('~'), 'Desktop',fileName)
file = open(filePath, 'w+')
file.write(header())

rows = 1
while True:
	row = log()
	items = row.split(',')
	print(f'Row {rows}\tTime {items[1]}\tConnections = {items[2]}')
	file.write(row)
	time.sleep(1)
	rows += 1
file.close()
