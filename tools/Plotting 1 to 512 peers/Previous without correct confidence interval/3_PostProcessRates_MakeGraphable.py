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

def logHeader():
	line = 'LineNum,'
	line += 'PlotNum,'
	line += 'PlotName,'
	for i in connectionSequence:
		line += str(i) + ','
	outputFile.write(line + '\n')

# Given all the columns, and a list of column indexes for each data point, log a line/row
def logLine(lineNum, plotNum, plotName, indexes, columns, columnIndex):
	print(f'Logging line {lineNum}')
	line = str(lineNum) + ','
	line += str(plotNum) + ','
	line += str(plotName) + ','
	for index in indexes:
		line += str(columns[columnIndex][index]) + ','
	outputFile.write(line + '\n')

# Main
if __name__ == '__main__':
	readerFile = open('Raw Rate Info.csv', 'r')
	reader = csv.reader(x.replace('\0', '') for x in readerFile)
	header = next(reader)

	# Generate a list of sorted rows
	rows = []
	for row in reader:
		rows.append(row)
	rows.sort(key = lambda x: float(x[1]))
	readerFile.close()

	# Generate a list of columns
	columns = []
	for title in header:
		columns.append([])
	for row in rows:
		i = 0
		for cell in row:
			columns[i].append(cell)
			i += 1

	outputFile = open(f'Graphable Rate Info.csv', 'w', newline='')
	logHeader()

	# Go through the number of connections (sorted by sample number)
	# For every full match to connectionSequence, log a line's data on the output
	columnIndex = 0
	lineIndex = 0
	lineIndexes = []
	lines = []
	print('Searching for full line matches:')
	for numConnections in columns[2]:
		if numConnections == connectionSequence[lineIndex]:
			lineIndexes.append(columnIndex)
			lineIndex += 1
		else:
			lineIndexes = []
			lineIndex = 0
		print(numConnections, lineIndexes)
		if lineIndex == len(connectionSequence):
			print('Line match found!')
			lines.append(lineIndexes.copy())
			lineIndexes = []
			lineIndex = 0
		columnIndex += 1

	# For each set of line indexes, we'll want to generate a set of lines for each column
	lineNum = 1
	plotNum = 1
	rowIndex = 4
	for plotName in header[4:]: # Skip the first few header items that we don't want graphed, such as file name
		for lineIndexes in lines:
			logLine(lineNum, plotNum, plotName, lineIndexes, columns, rowIndex)
			lineNum += 1
		plotNum += 1
		rowIndex += 1
	
	outputFile.close()

	print('Done!')