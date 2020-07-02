import matplotlib.pyplot as plt
import csv

titles = {
	'AvgBlockDelay': 'Average block delay in seconds',
	'NumAllMsgs': 'Total number of messages',
	'NumUniqueTX': 'Number of unique transactions',
	'NumAllTX': 'Number of all transaction messages',
	'TXRatioUnique/All': 'Ratio of unique transactions to all transactions',
	'NumUniqueBlocks': 'Number of unique blocks',
	'NumAllBlocks': 'Number of all block messages',
	'BlockRatioUnique/All': 'Ratio of unique blocks to all blocks',
	'AllMsgs/Sec': 'Number of messages per second',
	'UniqueTX/Sec': 'Number of unique transactions per second',
	'AllTX/Sec': 'Number of all transactions per second',
	'AvgCPU': 'Average CPU percentage',
	'AvgMemory': 'Average memory percentage',
	'AvgMemoryMB': 'Average size of memory in megabytes'
}

if __name__ == '__main__':
	readerFile = open('Graphable Rate Info.csv', 'r')
	reader = csv.reader(x.replace('\0', '') for x in readerFile)
	header = next(reader)

	plotNumber = '-1'

	x_axis = header[3:]
	# Remove the last item if it's empty
	if x_axis[-1] == '': x_axis = x_axis[:-1]
	
	for row in reader:
		# Remove the last item if it's empty
		if row[-1] == '': row = row[:-1]

		print(row[2])
		y_axis = [float(x) for x in row[3:]]


		# Check if it's continuing the last plot (multiple lines), or starting new plot
		if plotNumber == row[1] or plotNumber == '-1':
			if row[2] in titles: plt.title(titles[row[2]])
			else: plt.title(row[2])
			plt.plot(x_axis, y_axis, color='black', linewidth=2)
		else:

			plt.show()
			plt.clf()
			plt.plot(x_axis, y_axis, color='black', linewidth=2)
			plt.xlabel('Number of peer connections')
			if row[2] in titles: plt.title(titles[row[2]])
			else: plt.title(row[2])
			plt.grid(b=True, which='both', axis='y')

		plotNumber = row[1]

	plt.show()