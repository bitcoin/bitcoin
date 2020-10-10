import os
import time
import datetime
from subprocess import *

def bitcoin(cmd):
	pipe = Popen('cd C:\Program Files\Bitcoin\daemon & bitcoin-cli.exe ' + cmd, shell=True, stdout=PIPE, stderr=PIPE)
	lines = ''
	while True:
		line = pipe.stdout.readline()
		if line: lines += line.decode("utf-8")
		else: break
	return lines.strip()

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

fileName = 'WindowsLoggingConnections.csv'
filePath = os.path.join(os.environ['USERPROFILE'], 'Desktop',fileName) 
file = open(filePath, 'w')
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
