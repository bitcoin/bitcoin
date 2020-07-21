# This script scans for Sample X numConnections Y.csv files wihin the current directory, and parses them, adding # Diff and BytesSum Diff columns to the end

import csv
import os
import re
import sys
import time
from decimal import Decimal

# These numbers of peers are repeated
connectionSequence = ['1', '2', '4', '8', '16', '32', '64', '128', '256', '512'];

os.system('clear')

print()
print('Number of connections order: ' + str(connectionSequence))
modifyConnectionSequence = input('Would you like to modify this? ').lower() in ['y', 'yes']
if modifyConnectionSequence:
	connectionSequence = input('Enter a new connection sequence (separated by spaces): ').split()
	#connectionSequenceStr = input('Enter a new connection sequence (separated by spaces): ').split()
	#connectionSequence = [int(s) for s in connectionSequenceStr]
print()

def listToCSV(data):
	#line = '","'.join(data)
	#while line.startswith('",'):
	#	line = line[2:]
	#while line.endswith(',"'):
	#	line = line[:-2]
	#return line + '\n'
	line = ''
	for item in data:
		line += '"' + item.replace('"', '""') + '",'
	return line + '\n'

# Main
if __name__ == '__main__':
	if not os.path.exists('Matlab Data'):
		os.makedirs('Matlab Data')
	outputFiles = {}
	for i in connectionSequence:
		outputFiles[str(i)] = open(f'Matlab Data\\Connections_{str(i)}.csv', 'w', newline='')
	
	readerFile = open('Raw Rate Info.csv', 'r')
	reader = csv.reader(x.replace('\0', '') for x in readerFile)
	header = next(reader)

	headerStr = listToCSV(header)
	for i in outputFiles:
		outputFiles[i].write(headerStr)

	for row in reader:
		connections = row[2]
		if connections in outputFiles:
			outputFiles[connections].write(listToCSV(row))
		else:
			print(f'ERROR: LINE="{row.strip()}"')
	
	for i in outputFiles:
		outputFiles[i].close()

	print('Done!')