import matplotlib.pyplot as plt
import csv

displayWindows = input('Save plots to file (otherwise display windows)? ').lower() in ['n', 'no']

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

# Initialize the plot information
def initPlot(title):
	plt.clf()
	plt.xlabel('Number of peer connections')
	if title in titles: plt.title(titles[title])
	else: plt.title(title)
	plt.grid(b=True, which='both', axis='y')

def finalizePlot(plotNumber, x_axis, y_average_list, y_average_count):
	# Compute the average
	for i in range(0, len(y_average_list)):
		y_average_list[i] /= y_average_count

	plt.plot(x_axis, y_average_list, color='black', linewidth=4)
	if displayWindows:
		plt.show()
	else:
		plt.savefig('Figures//Figure ' + plotNumber + '.png')

if __name__ == '__main__':
	readerFile = open('Graphable Rate Info.csv', 'r')
	reader = csv.reader(x.replace('\0', '') for x in readerFile)
	header = next(reader)

	plotNumber = '-1'

	x_axis = header[3:]
	# Remove empty strings
	while '' in x_axis: x_axis.remove('')
	y_average_list = [0] * len(x_axis)
	y_average_count = 0
	
	row = ''
	for row in reader:
		# Remove empty strings
		while '' in row: row.remove('')
		if len(row) < 4: continue

		print(row[2])
		y_axis = [float(x) for x in row[3:]]



		# Check if it's continuing the last plot (multiple lines), or starting new plot
		if plotNumber == '-1': initPlot(row[2])

		if plotNumber == row[1] or plotNumber == '-1':
			plt.plot(x_axis, y_axis, color='black', linewidth=1)
		else:

			finalizePlot(plotNumber, x_axis, y_average_list, y_average_count)
			initPlot(row[2])
			plt.plot(x_axis, y_axis, color='black', linewidth=1)
			y_average_list = [0] * len(y_average_list)
			y_average_count = 0

		# Add together the values
		for i in range(0, len(y_average_list)):
			y_average_list[i] += float(y_axis[i])
		y_average_count += 1

		plotNumber = row[1]

	if len(row) >= 4:
		finalizePlot(plotNumber, x_axis, y_average_list, y_average_count)

	print()
	print('Done!')