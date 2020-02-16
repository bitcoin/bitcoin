import os
import csv
import time

# Generates the probability of each message type occuring

def main():
	os.system('clear')
	timeStart = time.perf_counter()
	file = open(os.path.expanduser(f'~/Desktop/GetMsgInfoDatasetProbabilities.csv'), 'w+')
	file.write(getHeader())
	for fileName in os.listdir('GetMsgInfoDatasets'):
		if fileName.endswith(".csv"):
			file.write(parseFile(fileName, f'GetMsgInfoDatasets/{fileName}'))
	timeEnd = time.perf_counter()
	print('Successfully generated "GetMsgInfoDatasetProbabilities.csv" on the Desktop.')
	print(f'\nThat took exactly {timeEnd - timeStart} seconds.')

def parseFile(fileName, path):
	print(f'Parsing "{fileName}"...')
	histogram = {
		'VERSION': 0,
		'VERACK': 0,
		'ADDR': 0,
		'INV': 0,
		'GETDATA': 0,
		'MERKLEBLOCK': 0,
		'GETBLOCKS': 0,
		'GETHEADERS': 0,
		'TX': 0,
		'HEADERS': 0,
		'BLOCK': 0,
		'GETADDR': 0,
		'MEMPOOL': 0,
		'PING': 0,
		'PONG': 0,
		'NOTFOUND': 0,
		'FILTERLOAD': 0,
		'FILTERADD': 0,
		'FILTERCLEAR': 0,
		'SENDHEADERS': 0,
		'FEEFILTER': 0,
		'SENDCMPCT': 0,
		'CMPCTBLOCK': 0,
		'GETBLOCKTXN': 0,
		'BLOCKTXN': 0,
		'REJECT': 0,
	}
	bytesHistogram = {
		'VERSION': 0,
		'VERACK': 0,
		'ADDR': 0,
		'INV': 0,
		'GETDATA': 0,
		'MERKLEBLOCK': 0,
		'GETBLOCKS': 0,
		'GETHEADERS': 0,
		'TX': 0,
		'HEADERS': 0,
		'BLOCK': 0,
		'GETADDR': 0,
		'MEMPOOL': 0,
		'PING': 0,
		'PONG': 0,
		'NOTFOUND': 0,
		'FILTERLOAD': 0,
		'FILTERADD': 0,
		'FILTERCLEAR': 0,
		'SENDHEADERS': 0,
		'FEEFILTER': 0,
		'SENDCMPCT': 0,
		'CMPCTBLOCK': 0,
		'GETBLOCKTXN': 0,
		'BLOCKTXN': 0,
		'REJECT': 0,
	}
	clocksHistogram = {
		'VERSION': 0,
		'VERACK': 0,
		'ADDR': 0,
		'INV': 0,
		'GETDATA': 0,
		'MERKLEBLOCK': 0,
		'GETBLOCKS': 0,
		'GETHEADERS': 0,
		'TX': 0,
		'HEADERS': 0,
		'BLOCK': 0,
		'GETADDR': 0,
		'MEMPOOL': 0,
		'PING': 0,
		'PONG': 0,
		'NOTFOUND': 0,
		'FILTERLOAD': 0,
		'FILTERADD': 0,
		'FILTERCLEAR': 0,
		'SENDHEADERS': 0,
		'FEEFILTER': 0,
		'SENDCMPCT': 0,
		'CMPCTBLOCK': 0,
		'GETBLOCKTXN': 0,
		'BLOCKTXN': 0,
		'REJECT': 0,
	}
	with open(path) as file:
		reader = csv.reader(file, delimiter=',')
		lineCount = 0
		for row in reader:
			if lineCount != 0: # Skip the header
				histogram['VERSION'] += max(0, int(row[4]))
				histogram['VERACK'] += max(0, int(row[14]))
				histogram['ADDR'] += max(0, int(row[24]))
				histogram['INV'] += max(0, int(row[34]))
				histogram['GETDATA'] += max(0, int(row[44]))
				histogram['MERKLEBLOCK'] += max(0, int(row[54]))
				histogram['GETBLOCKS'] += max(0, int(row[64]))
				histogram['GETHEADERS'] += max(0, int(row[74]))
				histogram['TX'] += max(0, int(row[84]))
				histogram['HEADERS'] += max(0, int(row[94]))
				histogram['BLOCK'] += max(0, int(row[104]))
				histogram['GETADDR'] += max(0, int(row[114]))
				histogram['MEMPOOL'] += max(0, int(row[124]))
				histogram['PING'] += max(0, int(row[134]))
				histogram['PONG'] += max(0, int(row[144]))
				histogram['NOTFOUND'] += max(0, int(row[154]))
				histogram['FILTERLOAD'] += max(0, int(row[164]))
				histogram['FILTERADD'] += max(0, int(row[174]))
				histogram['FILTERCLEAR'] += max(0, int(row[184]))
				histogram['SENDHEADERS'] += max(0, int(row[194]))
				histogram['FEEFILTER'] += max(0, int(row[204]))
				histogram['SENDCMPCT'] += max(0, int(row[214]))
				histogram['CMPCTBLOCK'] += max(0, int(row[224]))
				histogram['GETBLOCKTXN'] += max(0, int(row[234]))
				histogram['BLOCKTXN'] += max(0, int(row[244]))
				histogram['REJECT'] += max(0, int(row[254]))

				clocksHistogram['VERSION'] += max(0, int(row[5]))
				clocksHistogram['VERACK'] += max(0, int(row[15]))
				clocksHistogram['ADDR'] += max(0, int(row[25]))
				clocksHistogram['INV'] += max(0, int(row[35]))
				clocksHistogram['GETDATA'] += max(0, int(row[45]))
				clocksHistogram['MERKLEBLOCK'] += max(0, int(row[55]))
				clocksHistogram['GETBLOCKS'] += max(0, int(row[65]))
				clocksHistogram['GETHEADERS'] += max(0, int(row[75]))
				clocksHistogram['TX'] += max(0, int(row[85]))
				clocksHistogram['HEADERS'] += max(0, int(row[95]))
				clocksHistogram['BLOCK'] += max(0, int(row[105]))
				clocksHistogram['GETADDR'] += max(0, int(row[115]))
				clocksHistogram['MEMPOOL'] += max(0, int(row[125]))
				clocksHistogram['PING'] += max(0, int(row[135]))
				clocksHistogram['PONG'] += max(0, int(row[145]))
				clocksHistogram['NOTFOUND'] += max(0, int(row[155]))
				clocksHistogram['FILTERLOAD'] += max(0, int(row[165]))
				clocksHistogram['FILTERADD'] += max(0, int(row[175]))
				clocksHistogram['FILTERCLEAR'] += max(0, int(row[185]))
				clocksHistogram['SENDHEADERS'] += max(0, int(row[195]))
				clocksHistogram['FEEFILTER'] += max(0, int(row[205]))
				clocksHistogram['SENDCMPCT'] += max(0, int(row[215]))
				clocksHistogram['CMPCTBLOCK'] += max(0, int(row[225]))
				clocksHistogram['GETBLOCKTXN'] += max(0, int(row[235]))
				clocksHistogram['BLOCKTXN'] += max(0, int(row[245]))
				clocksHistogram['REJECT'] += max(0, int(row[255]))

				bytesHistogram['VERSION'] += max(0, int(row[9]))
				bytesHistogram['VERACK'] += max(0, int(row[19]))
				bytesHistogram['ADDR'] += max(0, int(row[29]))
				bytesHistogram['INV'] += max(0, int(row[39]))
				bytesHistogram['GETDATA'] += max(0, int(row[49]))
				bytesHistogram['MERKLEBLOCK'] += max(0, int(row[59]))
				bytesHistogram['GETBLOCKS'] += max(0, int(row[69]))
				bytesHistogram['GETHEADERS'] += max(0, int(row[79]))
				bytesHistogram['TX'] += max(0, int(row[89]))
				bytesHistogram['HEADERS'] += max(0, int(row[99]))
				bytesHistogram['BLOCK'] += max(0, int(row[109]))
				bytesHistogram['GETADDR'] += max(0, int(row[119]))
				bytesHistogram['MEMPOOL'] += max(0, int(row[129]))
				bytesHistogram['PING'] += max(0, int(row[139]))
				bytesHistogram['PONG'] += max(0, int(row[149]))
				bytesHistogram['NOTFOUND'] += max(0, int(row[159]))
				bytesHistogram['FILTERLOAD'] += max(0, int(row[169]))
				bytesHistogram['FILTERADD'] += max(0, int(row[179]))
				bytesHistogram['FILTERCLEAR'] += max(0, int(row[189]))
				bytesHistogram['SENDHEADERS'] += max(0, int(row[199]))
				bytesHistogram['FEEFILTER'] += max(0, int(row[209]))
				bytesHistogram['SENDCMPCT'] += max(0, int(row[219]))
				bytesHistogram['CMPCTBLOCK'] += max(0, int(row[229]))
				bytesHistogram['GETBLOCKTXN'] += max(0, int(row[239]))
				bytesHistogram['BLOCKTXN'] += max(0, int(row[249]))
				bytesHistogram['REJECT'] += max(0, int(row[259]))

				
			lineCount += 1
	totalMessages = 0
	totalBytes = 0
	totalClocks = 0
	for messageType in histogram:
		totalMessages += histogram[messageType]
		totalBytes += bytesHistogram[messageType]
		totalClocks += clocksHistogram[messageType]

	line = ''
	line += fileName + ','
	line += str(lineCount) + ','
	line += str(totalMessages) + ','
	line += str(totalBytes) + ','
	line += str(totalClocks) + ','
	for messageType in histogram:
		line += str(histogram[messageType] / totalMessages) + ','
		line += str(bytesHistogram[messageType] / totalBytes) + ','
		line += str(clocksHistogram[messageType] / totalClocks) + ','
	line += '\n'
	return line

def getHeader():
	line = ''
	line += '"File name",'
	line += '"Num rows",'
	line += '"Total num messages",'
	line += '"Total num bytes",'
	line += '"Total num clocks",'

	line += '"P(VERSION)",'
	line += '"P(VERSION bytes)",'
	line += '"P(VERSION clocks)",'

	line += '"P(VERACK)",'
	line += '"P(VERACK bytes)",'
	line += '"P(VERACK clocks)",'

	line += '"P(ADDR)",'
	line += '"P(ADDR bytes)",'
	line += '"P(ADDR clocks)",'

	line += '"P(INV)",'
	line += '"P(INV bytes)",'
	line += '"P(INV clocks)",'

	line += '"P(GETDATA)",'
	line += '"P(GETDATA bytes)",'
	line += '"P(GETDATA clocks)",'

	line += '"P(MERKLEBLOCK)",'
	line += '"P(MERKLEBLOCK bytes)",'
	line += '"P(MERKLEBLOCK clocks)",'

	line += '"P(GETBLOCKS)",'
	line += '"P(GETBLOCKS bytes)",'
	line += '"P(GETBLOCKS clocks)",'

	line += '"P(GETHEADERS)",'
	line += '"P(GETHEADERS bytes)",'
	line += '"P(GETHEADERS clocks)",'

	line += '"P(TX)",'
	line += '"P(TX bytes)",'
	line += '"P(TX clocks)",'

	line += '"P(HEADERS)",'
	line += '"P(HEADERS bytes)",'
	line += '"P(HEADERS clocks)",'

	line += '"P(BLOCK)",'
	line += '"P(BLOCK bytes)",'
	line += '"P(BLOCK clocks)",'

	line += '"P(GETADDR)",'
	line += '"P(GETADDR bytes)",'
	line += '"P(GETADDR clocks)",'

	line += '"P(MEMPOOL)",'
	line += '"P(MEMPOOL bytes)",'
	line += '"P(MEMPOOL clocks)",'

	line += '"P(PING)",'
	line += '"P(PING bytes)",'
	line += '"P(PING clocks)",'

	line += '"P(PONG)",'
	line += '"P(PONG bytes)",'
	line += '"P(PONG clocks)",'

	line += '"P(NOTFOUND)",'
	line += '"P(NOTFOUND bytes)",'
	line += '"P(NOTFOUND clocks)",'

	line += '"P(FILTERLOAD)",'
	line += '"P(FILTERLOAD bytes)",'
	line += '"P(FILTERLOAD clocks)",'

	line += '"P(FILTERADD)",'
	line += '"P(FILTERADD bytes)",'
	line += '"P(FILTERADD clocks)",'

	line += '"P(FILTERCLEAR)",'
	line += '"P(FILTERCLEAR bytes)",'
	line += '"P(FILTERCLEAR clocks)",'

	line += '"P(SENDHEADERS)",'
	line += '"P(SENDHEADERS bytes)",'
	line += '"P(SENDHEADERS clocks)",'

	line += '"P(FEEFILTER)",'
	line += '"P(FEEFILTER bytes)",'
	line += '"P(FEEFILTER clocks)",'

	line += '"P(SENDCMPCT)",'
	line += '"P(SENDCMPCT bytes)",'
	line += '"P(SENDCMPCT clocks)",'

	line += '"P(CMPCTBLOCK)",'
	line += '"P(CMPCTBLOCK bytes)",'
	line += '"P(CMPCTBLOCK clocks)",'

	line += '"P(GETBLOCKTXN)",'
	line += '"P(GETBLOCKTXN bytes)",'
	line += '"P(GETBLOCKTXN clocks)",'

	line += '"P(BLOCKTXN)",'
	line += '"P(BLOCKTXN bytes)",'
	line += '"P(BLOCKTXN clocks)",'

	line += '"P(REJECT)",'
	line += '"P(REJECT bytes)",'
	line += '"P(REJECT clocks)",'

	line += '\n'
	return line

main()