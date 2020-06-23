# This script scans for Sample X numConnections Y.csv files wihin the current directory, and parses them, adding # Diff and BytesSum Diff columns to the end

import csv
import os
import re
import sys
import time
from decimal import Decimal

# new transactions per second
# new transaction bytes per second
# new blocks per second
connectionsColumnIndex = 6

numSecondsPerSample = 1


# List the files with a regular expression
def listFiles(regex, directory):
	path = os.path.join(os.curdir, directory)
	return [os.path.join(path, file) for file in os.listdir(path) if os.path.isfile(os.path.join(path, file)) and bool(re.match(regex, file))]


if __name__ == '__main__':
	outputFile = open(f'Post Processed Rate Info.csv', 'w', newline='')
	line = 'File Name,'
	line += 'Connections,'
	line += 'Logging Time,'

	line += 'TX/sec,'
	line += 'Blocks/sec,'
	line += 'All messages/sec,'

	line += 'TX bytes/sec,'
	#line += 'Block bytes/sec,'
	line += 'All message bytes/sec'

	outputFile.write(line + '\n')
	directory = 'PostProcessed'
	files = listFiles(r'Sample [0-9]+, numConnections = [0-9]+, minutes = [0-9\.]+', directory)

	for file in files:
		print(f'Processing {file}')
		readerFile = open(file, 'r')
		# Remove NUL bytes to prevent errors
		reader = csv.reader(x.replace('\0', '') for x in readerFile)

		header = next(reader)
		headerDesc = next(reader)

		match = re.match(r'(?:\.[/\\])?' + directory + r'[/\\]Sample [0-9]+, numConnections = ([0-9]+), minutes = [0-9\.]+', file)

		connections = match.group(1)

		txPerSec = 0
		blocksPerSec = 0
		allMsgsPerSec = 0

		txBytesPerSec = 0
		blockBytesPerSec = 0
		allMsgBytesPerSec = 0

		numRows = 0
		firstRow = ''
		lastRow = ''
		row = ''
		prevRow = ''
		for row in reader:
			if numRows == 0: firstRow = row

			# Skip the first row because the difference contains everything before the script execution
			if prevRow != '':
				txDiff = max(0, float(row[10]) - float(prevRow[10]))
				txByteDiff = max(0, float(row[11]) - float(prevRow[11]))

				blockDiff = max(0, float(row[7]) - float(prevRow[7]))
				#blockByteDiff = 

				txPerSec += txByteDiff
				txBytesPerSec += txByteDiff
				blocksPerSec += blockDiff
				#blockBytesPerSec += blockByteDiff

				allMsgsPerSec += (float(row[206]) + float(row[208]) + float(row[210]) + float(row[212]) + float(row[214]) + float(row[216]) + float(row[218]) + float(row[220]) + float(row[222]) + float(row[224]) + float(row[226]) + float(row[228]) + float(row[230]) + float(row[232]) + float(row[234]) + float(row[236]) + float(row[238]) + float(row[240]) + float(row[242]) + float(row[244]) + float(row[246]) + float(row[248]) + float(row[250]) + float(row[252]) + float(row[254]) + float(row[256]) + float(row[258]));
				allMsgBytesPerSec += (float(row[207]) + float(row[209]) + float(row[211]) + float(row[213]) + float(row[215]) + float(row[217]) + float(row[219]) + float(row[221]) + float(row[223]) + float(row[225]) + float(row[227]) + float(row[229]) + float(row[231]) + float(row[233]) + float(row[235]) + float(row[237]) + float(row[239]) + float(row[241]) + float(row[243]) + float(row[245]) + float(row[247]) + float(row[249]) + float(row[251]) + float(row[253]) + float(row[255]) + float(row[257]) + float(row[259]));
			
			numRows += 1
			prevRow = row
		lastRow = row

		loggingTime = (float(lastRow[1]) - float(firstRow[1])) / 60

		txPerSec /= numRows
		blocksPerSec /= numRows
		allMsgsPerSec /= numRows

		txBytesPerSec /= numRows
		#blockBytesPerSec /= numRows
		allMsgBytesPerSec /= numRows


		line = '"' + os.path.basename(file) + '",' # File Name
		line += str(connections) + ',' # Connections
		line += str(loggingTime) + ',' # Logging Time

		line += str(txPerSec) + ',' # TX/sec
		line += str(blocksPerSec) + ',' # Blocks/sec
		line += str(allMsgsPerSec) + ',' # All messages/sec

		line += str(txBytesPerSec) + ',' # TX bytes/sec
		#line += str(blockBytesPerSec) + ',' # Block bytes/sec
		line += str(allMsgBytesPerSec) + ',' # All message bytes/sec

		outputFile.write(line + '\n')

		readerFile.close()
	
	outputFile.close()