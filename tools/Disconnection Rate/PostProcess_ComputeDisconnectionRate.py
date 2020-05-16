# This script scans for Sample X numConnections Y.csv files wihin the current directory, and parses them, adding # Diff and BytesSum Diff columns to the end

import csv
import os
import re
import sys
import time
from decimal import Decimal


connectionsColumnIndex = 6

numSecondsPerSample = 1


# List the files with a regular expression
def listFiles(regex, directory):
	path = os.path.join(os.curdir, directory)
	return [os.path.join(path, file) for file in os.listdir(path) if os.path.isfile(os.path.join(path, file)) and bool(re.match(regex, file))]


if __name__ == '__main__':
	outputFile = open(f'Post Processed Connection Info.csv', 'w', newline='')
	line = 'File Name,'
	line += 'Logging Time (seconds),'
	line += 'Num Disconnections,'
	line += 'Avg Num Peers,'
	line += 'Min Num Peers,'
	line += 'Max Num Peers,'
	line += 'Avg Connection Time (seconds),'
	line += 'Min Connection Time (seconds),'
	line += 'Max Connection Time (seconds),'
	line += 'Raw Connection Time Dictionary,'
	outputFile.write(line + '\n')

	files = listFiles('', 'Datasets')
	for file in files:
		print(f'Processing {file}')
		readerFile = open(file, 'r')
		# Remove NUL bytes to prevent errors
		reader = csv.reader(x.replace('\0', '') for x in readerFile)

		header = next(reader)
		headerDesc = next(reader)
		
		if header[connectionsColumnIndex] != 'Connections':
			print(f'Error: Could not find the "Connections" column (not column #{connectionsColumnIndex}).')
			sys.exit()

		loggingTime = 0
		avgNumPeers = 0
		minNumPeers = 1000000000
		maxNumPeers = 0
		avgConnectionTime = 0
		minConnectionTime = 1000000000
		maxConnectionTime = 0
		numDisconnections = 0
		dictionary = {}
		numRows = 0
		connections = []
		for row in reader:
			if connectionsColumnIndex >= len(row):
				break
			connections_str = row[connectionsColumnIndex]
			#print(connections_str),type(connections_str)
			connections = connections_str.strip().split(' ')
			
			for peer in connections:
				if peer not in dictionary:
					dictionary[peer] = numSecondsPerSample
				else:
					dictionary[peer] += numSecondsPerSample
			loggingTime += numSecondsPerSample
			avgNumPeers += len(connections)
			if len(connections) < minNumPeers: minNumPeers = len(connections)
			if len(connections) > maxNumPeers: maxNumPeers = len(connections)
			numRows += 1

		if numRows == 0:
			continue # Skip errored files
		avgNumPeers /= numRows

		# The total number of peers minus the ones at the end, since they did not disconnect
		numDisconnections = len(dictionary) - len(connections)
		numPeers = 0
		for peer in dictionary:
			connectionTime = dictionary[peer]
			avgConnectionTime += connectionTime
			if connectionTime < minConnectionTime: minConnectionTime = connectionTime
			if connectionTime > maxConnectionTime: maxConnectionTime = connectionTime
			numPeers += 1

		avgConnectionTime /= numPeers

		line = os.path.basename(file) + ',' # File Name
		line += str(loggingTime) + ',' # Logging Time
		line += str(numDisconnections) + ',' # Num Disconnections
		line += str(avgNumPeers) + ',' # Avg Num Peers
		line += str(minNumPeers) + ',' # Min Num Peers
		line += str(maxNumPeers) + ',' # Max Num Peers
		line += str(avgConnectionTime) + ',' # Avg Connection Time
		line += str(minConnectionTime) + ',' # Min Connection Time
		line += str(maxConnectionTime) + ',' # Max Connection Time
		line += '"' + str(dictionary) + '",' # Raw Connection Time Dictionary
		outputFile.write(line + '\n')

		readerFile.close()
	
	outputFile.close()