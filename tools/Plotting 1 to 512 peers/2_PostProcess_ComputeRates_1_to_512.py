# This script scans for Sample X numConnections Y.csv files wihin the current directory, and parses them, adding # Diff and BytesSum Diff columns to the end

import csv
import os
import re
import sys
import time
from decimal import Decimal

numSecondsPerSample = 1


# List the files with a regular expression
def listFiles(regex, directory):
	path = os.path.join(os.curdir, directory)
	return [os.path.join(path, file) for file in os.listdir(path) if os.path.isfile(os.path.join(path, file)) and bool(re.match(regex, file))]


if __name__ == '__main__':
	outputFile = open(f'Raw Rate Info.csv', 'w', newline='')
	line = 'File Name,'
	line += 'SampleNum,'
	line += 'Connections,'
	line += 'Logging Time,'

	line += 'AvgBlockDelay,'

	line += 'NumAllMsgs,'

	line += 'NumUniqueTX,'
	line += 'NumAllTX,'
	line += 'TXRatioUnique/All,'
	
	line += 'NumUniqueBlocks,'
	line += 'NumAllBlocks,'
	line += 'BlockRatioUnique/All,'

	line += 'AllMsgs/Sec,'

	line += 'UniqueTX/Sec,'
	line += 'AllTX/Sec,'

	line += 'AvgCPU,'
	line += 'AvgMemory,'
	line += 'AvgMemoryMB,'


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

		match = re.match(r'(?:\.[/\\])?' + directory + r'[/\\]Sample ([0-9]+), numConnections = ([0-9]+), minutes = [0-9\.]+', file)

		fileName = os.path.basename(file)
		sampleNum = match.group(1)
		connections = match.group(2)
		loggingTime = 0
		avgBlockDelay = 0
		numAllMsgs = 0
		numUniqueTx = 0
		numAllTx = 0
		txRatioUniqueAll = 0
		numUniqueBlocks = 0
		numAllBlocks = 0
		blockRatioUniqueAll = 0
		allMsgsPerSec = 0
		uniqueTxPerSec = 0
		allTxPerSec = 0
		avgCpu = 0
		avgMemory = 0
		avgMemoryMB = 0


		numRows = 0
		firstRow = ''
		lastRow = ''
		row = ''
		prevRow = ''
		for row in reader:
			if numRows == 0: firstRow = row

			blockDelay = row[17]
			if blockDelay != '': avgBlockDelay += float(blockDelay)

			# Skip the first row because the difference contains everything before the script execution
			if prevRow != '':
				allMsgsDiff = float(row[216]) + float(row[218]) + float(row[220]) + float(row[222]) + float(row[224]) + float(row[226]) + float(row[228]) + float(row[230]) + float(row[232]) + float(row[234]) + float(row[236]) + float(row[238]) + float(row[240]) + float(row[242]) + float(row[244]) + float(row[246]) + float(row[248]) + float(row[250]) + float(row[252]) + float(row[254]) + float(row[256]) + float(row[258]) + float(row[260]) + float(row[262]) + float(row[264]) + float(row[266])
				numAllMsgs += allMsgsDiff
				uniqueTxDiff = max(0, float(row[20]) - float(prevRow[20]))
				numUniqueTx += uniqueTxDiff
				allTXDiff = float(row[232])
				numAllTx += allTXDiff
				uniqueBlockDiff = max(0, float(row[16]) - float(prevRow[16]))
				numUniqueBlocks += uniqueBlockDiff
				allBlocksDiff = float(row[236]) + float(row[260])
				numAllBlocks += allBlocksDiff
				allMsgsPerSec += allMsgsDiff
				uniqueTxPerSec += uniqueTxDiff
				allTxPerSec += allTXDiff

			cpuPercent = row[11]
			if cpuPercent.endswith('%'): cpuPercent = cpuPercent[:-1]
			avgCpu += float(cpuPercent)
			memoryPercent = row[12]
			if memoryPercent.endswith('%'): memoryPercent = memoryPercent[:-1]
			avgMemory += float(memoryPercent)
			avgMemoryMB += float(row[13])

			numRows += 1
			prevRow = row
		lastRow = row

		loggingTime = (float(lastRow[1]) - float(firstRow[1])) / 60

		avgBlockDelay /= numRows
		#numUniqueTx
		#numAllTx
		txRatioUniqueAll = numUniqueTx / numAllTx
		#numUniqueBlocks
		#numAllBlocks
		blockRatioUniqueAll = numUniqueBlocks / numAllBlocks
		allMsgsPerSec /= numRows
		uniqueTxPerSec /= numRows
		allTxPerSec /= numRows
		avgCpu /= numRows
		avgMemory /= numRows
		avgMemoryMB /= numRows

		line = '"' + fileName + '",'
		line += str(sampleNum) + ','
		line += str(connections) + ','
		line += str(loggingTime) + ','
		line += str(avgBlockDelay) + ','
		line += str(numAllMsgs) + ','
		line += str(numUniqueTx) + ','
		line += str(numAllTx) + ','
		line += str(txRatioUniqueAll) + ','
		line += str(numUniqueBlocks) + ','
		line += str(numAllBlocks) + ','
		line += str(blockRatioUniqueAll) + ','
		line += str(allMsgsPerSec) + ','
		line += str(uniqueTxPerSec) + ','
		line += str(allTxPerSec) + ','
		line += str(avgCpu) + ','
		line += str(avgMemory) + ','
		line += str(avgMemoryMB) + ','

		outputFile.write(line + '\n')

		readerFile.close()
	
	outputFile.close()