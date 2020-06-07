# This script merges logging files together

import csv
import os
import re
import sys
import time
from decimal import Decimal

# List the files with a regular expression
def listFiles(regex, directory):
	path = os.path.join(os.curdir, directory)
	return [os.path.join(path, file) for file in os.listdir(path) if os.path.isfile(os.path.join(path, file)) and bool(re.match(regex, file))]

if __name__ == '__main__':
	mergedFile = open(f'Merged.csv', 'w', newline='')
	hasHeader = False

	files = listFiles('', 'Datasets')
	for file in files:
		print(f'Processing {file}')
		readerFile = open(file, 'r')
		# Remove NUL bytes to prevent errors
		reader = csv.reader(x.replace('\0', '') for x in readerFile)

		header = next(reader)
		headerDesc = next(reader)

		if not hasHeader:
			mergedFile.write(','.join(header) + '\n')
			mergedFile.write(','.join(headerDesc) + '\n')
			hasHeader = True
		
		for line in reader:
			mergedFile.write(','.join(line) + '\n')

		readerFile.close()
	
	mergedFile.close()