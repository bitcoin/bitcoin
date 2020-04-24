# This script scans for Sample X numConnections Y.csv files wihin the current directory, and parses them, adding # Diff and BytesSum Diff columns to the end

import csv
import os
import re
import sys
import time
from decimal import Decimal

# List the files with a regular expression
def listFiles(regex):
	return [file for file in os.listdir(os.curdir) if os.path.isfile(file) and bool(re.match(regex, file))]


if __name__ == '__main__':
	try:
		os.mkdir('PostProcessed')
	except: pass
	files = listFiles(r'Sample [0-9]+ numConnections [0-9]+.csv')
	for file in files:
		print(f'Processing {file}')
		readerFile = open(file, 'r')
		writerFile = open(f'PostProcessed/{file}', 'w', newline='')

		reader = csv.reader(readerFile)
		writer = csv.writer(writerFile)

		header = next(reader)
		headerDesc = next(reader)
		newHeader = header.copy()
		newHeaderDesc = headerDesc.copy()
		newColumns = []

		for i, cell in enumerate(header):
			match = re.match(r'# ([^ ]+) Msgs', cell)
			if match is not None:
				name = f'{cell} Diff'
				newColumns.append((name, i))
				newHeader.append(name)
				newHeaderDesc.append('The difference in number of messages')

			match = re.match(r'BytesAvg ([^ ]+)', cell)
			if match is not None:
				name = f'BytesSum {match.group(1)} Diff'
				newColumns.append((name, i))
				newHeader.append(name)
				newHeaderDesc.append('The number of bytes for this individual message')

		writer.writerow(newHeader)
		writer.writerow(newHeaderDesc)
		lastRow = [0] * len(newHeader)
		for row in reader:
			for column in newColumns:
				cell, i = column
				if cell.startswith('# '):
					row.append(int(row[i]) - int(lastRow[i]))
				elif cell.startswith('BytesSum '):
					bytesSum = Decimal(row[i]) * Decimal(row[i - 5])
					lastBytesSum = Decimal(lastRow[i]) * Decimal(lastRow[i - 5])
					row.append(round(bytesSum - lastBytesSum))
			writer.writerow(row)
			lastRow = row

		readerFile.close()
		writerFile.close()