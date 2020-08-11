from threading import Timer
import datetime
import glob
import json
import os
import re
import subprocess
import sys
import time
import traceback


startupDelay = 0			# Number of seconds to wait after each node start up
numSecondsPerSample = 1
rowsPerFile = 6000		# Number of rows for cach numconnections file


# These numbers of peers are repeated
connectionSequence = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512];

os.system('clear')

print(f'Number of seconds per file: {rowsPerFile}')
modifyNumSecondsPerFile = input('Would you like to modify this? ').lower() in ['y', 'yes']
if modifyNumSecondsPerFile:
	rowsPerFile = float(input('Enter the number of seconds per file: '))

print()
print('Number of connections order: ' + str(connectionSequence))
modifyConnectionSequence = input('Would you like to modify this? ').lower() in ['y', 'yes']
if modifyConnectionSequence:
	connectionSequenceStr = input('Enter a new connection sequence (separated by spaces): ').split()
	connectionSequence = [int(x) for x in connectionSequenceStr]
	print('Number of connections order: ' + str(connectionSequence))
print()

datadir = os.path.expanduser('~/.bitcoin') # Virtual machine shared folder
if os.path.exists('/media/sf_Bitcoin/blocks'):
	datadir = ' -datadir=/media/sf_Bitcoin' # Virtual machine shared folder
elif os.path.exists('/media/sf_BitcoinVictim/blocks'):
	datadir = ' -datadir=/media/sf_BitcoinVictim'
elif os.path.exists('/media/sf_BitcoinAttacker/blocks'):
	datadir = ' -datadir=/media/sf_BitcoinAttacker'
elif os.path.exists('/media/sim/BITCOIN/blocks'):
	datadir = ' -datadir=/media/sim/BITCOIN'
numSkippedSamples = 0
hasReachedConnections = False

# Fetch the external IP from myexternalip.com
#def getmyIP():
#	return os.popen('curl \'http://myexternalip.com/raw\'').read() + ':8333'

# Send a command to the linux terminal
def terminal(cmd):
	return os.popen(cmd).read()

# Send a command to the Bitcoin console
def bitcoin(cmd):
	return terminal('./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd)

# Given a raw memory string from the linux "top" command, return the number of megabytes
# 1 EiB = 1024 * 1024 * 1024 * 1024 * 1024 * 1024 bytes
# 1 PiB = 1024 * 1024 * 1024 * 1024 * 1024 bytes
# 1 GiB = 1024 * 1024 * 1024 * 1024 bytes
# 1 MiB = 1024 * 1024 * 1024 bytes
# 1 KiB = 1024 * 1024 bytes
# 1 byte = 1000000 megabytes
def top_mem_to_mb(mem):
	if mem.endswith('e'): return float(mem[:-1]) * 1024 * 1024 * 1024 * 1024 * 1024 * 1024 / 1000000 # exbibytes to megabytes
	elif mem.endswith('p'): return float(mem[:-1]) * 1024 * 1024 * 1024 * 1024 * 1024 / 1000000 # gibibytes to megabytes
	elif mem.endswith('t'): return float(mem[:-1]) * 1024 * 1024 * 1024 * 1024 / 1000000 # tebibytes to megabytes
	elif mem.endswith('g'): return float(mem[:-1]) * 1024 * 1024 * 1024 / 1000000 # gibabytes to megabytes
	elif mem.endswith('m'): return float(mem[:-1]) * 1024 * 1024 / 1000000 # mebibytes to megabytes
	else: return float(mem) * 1024 / 1000000 # kibibytes to megabytes

# Given a raw memory string from the linux "top" command, return the number of mebibytes
def top_mem_to_MiB(mem):
	if mem.endswith('e'): return float(mem[:-1]) * 1024 * 1024 * 1024 * 1024 # exbibytes to mebibytes
	elif mem.endswith('p'): return float(mem[:-1]) * 1024 * 1024 * 1024 # gibibytes to mebibytes
	elif mem.endswith('t'): return float(mem[:-1]) * 1024 * 1024 # tebibytes to mebibytes
	elif mem.endswith('g'): return float(mem[:-1]) * 1024 # gibabytes to mebibytes
	elif mem.endswith('m'): return float(mem[:-1]) # mebibytes to mebibytes
	else: return float(mem) / 1024 # kibibytes to mebibytes

# Uses "top" to log the amount of resources that Bitcoin is using up, returns blank stings if the process is not running
def getCPUData():
	full_system = terminal('grep \'cpu \' /proc/stat | awk \'{usage=($2+$4)*100/($2+$4+$5)} END {print usage}\'').strip()
	#full_system = terminal('top -bn1 | grep "Cpu(s)" | sed "s/.*, *\([0-9.]*\)%* id.*/\1/" | awk \'{print 100 - $1}\'').strip()
	raw = terminal('top -b -n 1 |grep bitcoind').strip().split()
	while len(raw) < 12: raw.append('0')
	output = {
		'full_system': full_system,
		'process_ID': raw[0],
		'user': raw[1],
		'priority': raw[2],
		'nice_value': raw[3],
		'virtual_memory': str(top_mem_to_MiB(raw[4])),
		'memory': str(top_mem_to_MiB(raw[5])),
		'shared_memory': str(top_mem_to_MiB(raw[6])),
		'state': raw[7],
		'cpu_percent': raw[8],
		'memory_percent': raw[9],
		'time': raw[10],
		'process_name': raw[11],

	}
	return output


# Reads /proc/net/dev to get networking bytes/packets sent/received
def getNetworkData():
	raw = terminal('awk \'/:/ { print($1, $2, $3, $10, $11) }\' < /proc/net/dev -').strip().split('\n')
	selected_interface = ''
	for interface in raw:
		if not interface.startswith('lo:'):
			selected_interface = interface
			break
	data = selected_interface.strip().split()
	if selected_interface != '' and len(data) == 5:
		if data[0].endswith(':'): data[0] = data[0][:-1]
		# Differentiate between error and no error
		if data[0] == '': data[0] == ' '
		try:
			return {
				'interface': data[0],
				'bytes_sent': int(data[3]),
				'bytes_received': int(data[1]),
				'packets_sent': int(data[4]),
				'packets_received': int(data[2]),
			}
		except:
			pass
	return {
		'interface': '',
		'bytes_sent': '',
		'bytes_received': '',
		'packets_sent': '',
		'packets_received': '',
	}

bandwidth_download = 0
bandwidth_upload = 0
prev_bytes_received = ''
prev_bytes_sent = ''

# Calls getNetworkData to compute the difference in bytes send and received
def getBandwidth():
	global numSecondsPerSample, bandwidth_download, bandwidth_upload, prev_bytes_received, prev_bytes_sent
	network_data = getNetworkData()

	if network_data['interface'] != '':
		if prev_bytes_received == '': prev_bytes_received = network_data['bytes_received']
		r = (network_data['bytes_received'] - prev_bytes_received) / numSecondsPerSample
		bandwidth_download = r
		prev_bytes_received = network_data['bytes_received']

		if prev_bytes_sent == '': prev_bytes_sent = network_data['bytes_sent']
		d = (network_data['bytes_sent'] - prev_bytes_sent) / numSecondsPerSample
		bandwidth_upload = d
		prev_bytes_sent = network_data['bytes_sent']
	return {
		'download_bps': bandwidth_download,
		'upload_bps': bandwidth_upload,
	}

# Send commands to the Victim's Bitcoin Core Console
def bitcoinVictim(cmd):
	victim_ip = '10.0.2.5'
	return os.popen('curl --data-binary \'{"jsonrpc":"1.0","id":"curltext","method":"' + cmd + '","params":[]}\' -H \'content-type:text/plain;\' http://cybersec:kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame@' + victim_ip + ':8332').read()

def startBitcoin():
	subprocess.Popen(['gnome-terminal -t "Custom Bitcoin Core Instance" -- bash .././run.sh'], shell=True)

def bitcoinUp():
	return winexists('Custom Bitcoin Core Instance')

def winexists(target):
    for line in subprocess.check_output(['wmctrl', '-l']).splitlines():
        window_name = line.split(None, 3)[-1].decode()
        if window_name == target:
            return True
    return False

def fetchHeader():
	line = 'Timestamp,'
	line += 'Timestamp (Seconds),'

	line += 'NumPeers,'
	line += "NumInbound,"
	line += "NumPruned,"
	line += "NumTXRelayers,"
	line += "NumAddnode,"
	line += "NumBloom,"
	line += "AvgPingTime,"
	line += "TotalBanScore,"
	line += "Connections,"

	line += 'Bitcoin CPU %,'
	line += 'Memory %,'
	line += 'Memory (MB),'
	line += 'Full System Bandwidth (download/upload),'
	line += 'Full System CPU %,'

	line += 'BlockHeight,'
	line += 'Block/HeaderDelay (Seconds),'
	line += 'Num AcceptedBlocks PerSec,'
	line += 'Avg AcceptedBlocks PerSec,'
	line += 'MempoolSize,'
	line += 'MempoolBytes,'
	line += 'Num AcceptedTx PerSec,'
	line += 'Avg AcceptedTx PerSec,'

	line += 'ClocksPerSec,'
	line += '# VERSION Msgs,'
	line += 'Num VERSION PerSec,'
	line += 'Avg VERSION PerSec,'
	line += 'ClocksAvg VERSION,'
	line += 'ClocksMax VERSION,'
	line += 'BytesAvg VERSION,'
	line += 'BytesMax VERSION,'
	line += '# VERACK Msgs,'
	line += 'Num VERACK PerSec,'
	line += 'Avg VERACK PerSec,'
	line += 'ClocksAvg VERACK,'
	line += 'ClocksMax VERACK,'
	line += 'BytesAvg VERACK,'
	line += 'BytesMax VERACK,'
	line += '# ADDR Msgs,'
	line += 'Num ADDR PerSec,'
	line += 'Avg ADDR PerSec,'
	line += 'ClocksAvg ADDR,'
	line += 'ClocksMax ADDR,'
	line += 'BytesAvg ADDR,'
	line += 'BytesMax ADDR,'
	line += '# INV Msgs,'
	line += 'Num INV PerSec,'
	line += 'Avg INV PerSec,'
	line += 'ClocksAvg INV,'
	line += 'ClocksMax INV,'
	line += 'BytesAvg INV,'
	line += 'BytesMax INV,'
	line += '# GETDATA Msgs,'
	line += 'Num GETDATA PerSec,'
	line += 'Avg GETDATA PerSec,'
	line += 'ClocksAvg GETDATA,'
	line += 'ClocksMax GETDATA,'
	line += 'BytesAvg GETDATA,'
	line += 'BytesMax GETDATA,'
	line += '# MERKLEBLOCK Msgs,'
	line += 'Num MERKLEBLOCK PerSec,'
	line += 'Avg MERKLEBLOCK PerSec,'
	line += 'ClocksAvg MERKLEBLOCK,'
	line += 'ClocksMax MERKLEBLOCK,'
	line += 'BytesAvg MERKLEBLOCK,'
	line += 'BytesMax MERKLEBLOCK,'
	line += '# GETBLOCKS Msgs,'
	line += 'Num GETBLOCKS PerSec,'
	line += 'Avg GETBLOCKS PerSec,'
	line += 'ClocksAvg GETBLOCKS,'
	line += 'ClocksMax GETBLOCKS,'
	line += 'BytesAvg GETBLOCKS,'
	line += 'BytesMax GETBLOCKS,'
	line += '# GETHEADERS Msgs,'
	line += 'Num GETHEADERS PerSec,'
	line += 'Avg GETHEADERS PerSec,'
	line += 'ClocksAvg GETHEADERS,'
	line += 'ClocksMax GETHEADERS,'
	line += 'BytesAvg GETHEADERS,'
	line += 'BytesMax GETHEADERS,'
	line += '# TX Msgs,'
	line += 'Num TX PerSec,'
	line += 'Avg TX PerSec,'
	line += 'ClocksAvg TX,'
	line += 'ClocksMax TX,'
	line += 'BytesAvg TX,'
	line += 'BytesMax TX,'
	line += '# HEADERS Msgs,'
	line += 'Num HEADERS PerSec,'
	line += 'Avg HEADERS PerSec,'
	line += 'ClocksAvg HEADERS,'
	line += 'ClocksMax HEADERS,'
	line += 'BytesAvg HEADERS,'
	line += 'BytesMax HEADERS,'
	line += '# BLOCK Msgs,'
	line += 'Num BLOCK PerSec,'
	line += 'Avg BLOCK PerSec,'
	line += 'ClocksAvg BLOCK,'
	line += 'ClocksMax BLOCK,'
	line += 'BytesAvg BLOCK,'
	line += 'BytesMax BLOCK,'
	line += '# GETADDR Msgs,'
	line += 'Num GETADDR PerSec,'
	line += 'Avg GETADDR PerSec,'
	line += 'ClocksAvg GETADDR,'
	line += 'ClocksMax GETADDR,'
	line += 'BytesAvg GETADDR,'
	line += 'BytesMax GETADDR,'
	line += '# MEMPOOL Msgs,'
	line += 'Num MEMPOOL PerSec,'
	line += 'Avg MEMPOOL PerSec,'
	line += 'ClocksAvg MEMPOOL,'
	line += 'ClocksMax MEMPOOL,'
	line += 'BytesAvg MEMPOOL,'
	line += 'BytesMax MEMPOOL,'
	line += '# PING Msgs,'
	line += 'Num PING PerSec,'
	line += 'Avg PING PerSec,'
	line += 'ClocksAvg PING,'
	line += 'ClocksMax PING,'
	line += 'BytesAvg PING,'
	line += 'BytesMax PING,'
	line += '# PONG Msgs,'
	line += 'Num PONG PerSec,'
	line += 'Avg PONG PerSec,'
	line += 'ClocksAvg PONG,'
	line += 'ClocksMax PONG,'
	line += 'BytesAvg PONG,'
	line += 'BytesMax PONG,'
	line += '# NOTFOUND Msgs,'
	line += 'Num NOTFOUND PerSec,'
	line += 'Avg NOTFOUND PerSec,'
	line += 'ClocksAvg NOTFOUND,'
	line += 'ClocksMax NOTFOUND,'
	line += 'BytesAvg NOTFOUND,'
	line += 'BytesMax NOTFOUND,'
	line += '# FILTERLOAD Msgs,'
	line += 'Num FILTERLOAD PerSec,'
	line += 'Avg FILTERLOAD PerSec,'
	line += 'ClocksAvg FILTERLOAD,'
	line += 'ClocksMax FILTERLOAD,'
	line += 'BytesAvg FILTERLOAD,'
	line += 'BytesMax FILTERLOAD,'
	line += '# FILTERADD Msgs,'
	line += 'Num FILTERADD PerSec,'
	line += 'Avg FILTERADD PerSec,'
	line += 'ClocksAvg FILTERADD,'
	line += 'ClocksMax FILTERADD,'
	line += 'BytesAvg FILTERADD,'
	line += 'BytesMax FILTERADD,'
	line += '# FILTERCLEAR Msgs,'
	line += 'Num FILTERCLEAR PerSec,'
	line += 'Avg FILTERCLEAR PerSec,'
	line += 'ClocksAvg FILTERCLEAR,'
	line += 'ClocksMax FILTERCLEAR,'
	line += 'BytesAvg FILTERCLEAR,'
	line += 'BytesMax FILTERCLEAR,'
	line += '# SENDHEADERS Msgs,'
	line += 'Num SENDHEADERS PerSec,'
	line += 'Avg SENDHEADERS PerSec,'
	line += 'ClocksAvg SENDHEADERS,'
	line += 'ClocksMax SENDHEADERS,'
	line += 'BytesAvg SENDHEADERS,'
	line += 'BytesMax SENDHEADERS,'
	line += '# FEEFILTER Msgs,'
	line += 'Num FEEFILTER PerSec,'
	line += 'Avg FEEFILTER PerSec,'
	line += 'ClocksAvg FEEFILTER,'
	line += 'ClocksMax FEEFILTER,'
	line += 'BytesAvg FEEFILTER,'
	line += 'BytesMax FEEFILTER,'
	line += '# SENDCMPCT Msgs,'
	line += 'Num SENDCMPCT PerSec,'
	line += 'Avg SENDCMPCT PerSec,'
	line += 'ClocksAvg SENDCMPCT,'
	line += 'ClocksMax SENDCMPCT,'
	line += 'BytesAvg SENDCMPCT,'
	line += 'BytesMax SENDCMPCT,'
	line += '# CMPCTBLOCK Msgs,'
	line += 'Num CMPCTBLOCK PerSec,'
	line += 'Avg CMPCTBLOCK PerSec,'
	line += 'ClocksAvg CMPCTBLOCK,'
	line += 'ClocksMax CMPCTBLOCK,'
	line += 'BytesAvg CMPCTBLOCK,'
	line += 'BytesMax CMPCTBLOCK,'
	line += '# GETBLOCKTXN Msgs,'
	line += 'Num GETBLOCKTXN PerSec,'
	line += 'Avg GETBLOCKTXN PerSec,'
	line += 'ClocksAvg GETBLOCKTXN,'
	line += 'ClocksMax GETBLOCKTXN,'
	line += 'BytesAvg GETBLOCKTXN,'
	line += 'BytesMax GETBLOCKTXN,'
	line += '# BLOCKTXN Msgs,'
	line += 'Num BLOCKTXN PerSec,'
	line += 'Avg BLOCKTXN PerSec,'
	line += 'ClocksAvg BLOCKTXN,'
	line += 'ClocksMax BLOCKTXN,'
	line += 'BytesAvg BLOCKTXN,'
	line += 'BytesMax BLOCKTXN,'
	line += '# REJECT Msgs,'
	line += 'Num REJECT PerSec,'
	line += 'Avg REJECT PerSec,'
	line += 'ClocksAvg REJECT,'
	line += 'ClocksMax REJECT,'
	line += 'BytesAvg REJECT,'
	line += 'BytesMax REJECT,'
	line += '# [UNDOCUMENTED] Msgs,'
	line += 'Num [UNDOCUMENTED] PerSec,'
	line += 'Avg [UNDOCUMENTED] PerSec,'
	line += 'ClocksAvg [UNDOCUMENTED],'
	line += 'ClocksMax [UNDOCUMENTED],'
	line += 'BytesAvg [UNDOCUMENTED],'
	line += 'BytesMax [UNDOCUMENTED],'

	line += 'NumSkippedSamples,'

	line += '\n'

	line += 'Time in which the sample was taken (in human readable format),' # Timestamp
	line += 'Time in which the sample was taken (in number of seconds since 1/01/1970),' # Timestamp (Seconds)

	line += 'Number of peer connections,' # NumPeers
	line += 'Sum of inbound connections (initiated by the peer),' # NumInbound
	line += 'Sum of peers that do not have the full blockchain downloaded,' # NumPruned
	line += 'Sum of peers that relay transactions,' # NumTXRelayers
	line += 'Sum of peers that were connected through the "addnode command,' # NumAddnode
	line += 'Sum of peers that have a bloom filter,' # NumBloom
	line += 'Average round trip time for ping to return a pong,' # AvgPingTime
	line += 'Sum of all banscores (0 means genuine and 100 means ban),' # TotalBanScore
	line += 'An array of all peer addresses separated by a space,' # Connections

	line += 'Percentage of CPU usage of bitcoind for each core summed together (i.e. it may exceed 100%),' # CPU %
	line += 'Percentage of memory usage of bitcoind,' # Memory %
	line += 'Megabytes of memory being used,' # Mem
	line += 'System bytes per second of the download and upload rates,' # Full System Bandwidth (download/upload)
	line += 'System CPU usage pergentage,' # Full System CPU %
	#line += 'Contains the output from the "top" instruction for the bitcoind process,' # Resource usage
	#line += 'A list of the current peer connections,' # GetPeerInfo
	line += 'Block height of the node,' # BlockHeight
	line += 'Amount of time it took for the block and header to reach you,' # BlockDelay
	line += 'Number of unconfirmed transactions currently in the mempool,' # MempoolSize
	line += 'Number of bytes that the mempool is currently taking up,' # MempoolBytes
	line += 'Change in block height over the change in time for the most recent block update,' # AcceptedBlocksPerSec
	line += 'Average of all previous AcceptedBlocksPerSec values,' # AcceptedBlocksPerSecAvg
	line += 'Change in mempool size over the change in time for the most recent mempool size update,' # AcceptedTxPerSec
	line += 'Average of all previous AcceptedTxPerSec values,' # AcceptedTxPerSecAvg
	line += 'Number of clock cycles that occur each second within Bitcoin Core,' # ClocksPerSec
	line += 'Total number of VERSION messages received thus far,' # # VERSION Msgs
	line += 'Change in number of messages over the change in time for the most recent VERSION message received,' # Num VERSION PerSec
	line += 'Average number of VERSION messages received per second thus far (the average of all previous Num VERSION PerSec values),' # Avg VERSION PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg VERSION
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax VERSION
	line += 'Average number of bytes that this message took up,' # BytesAvg VERSION
	line += 'Maximum number of bytes that this message took up,' # BytesMax VERSION
	line += 'Total number of VERACK messages received thus far,' # # VERACK Msgs
	line += 'Change in number of messages over the change in time for the most recent VERACK message received,' # Num VERACK PerSec
	line += 'Average number of VERACK messages received per second thus far (the average of all previous Num VERACK PerSec values),' # Avg VERACK PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg VERACK
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax VERACK
	line += 'Average number of bytes that this message took up,' # BytesAvg VERACK
	line += 'Maximum number of bytes that this message took up,' # BytesMax VERACK
	line += 'Total number of ADDR messages received thus far,' # # ADDR Msgs
	line += 'Change in number of messages over the change in time for the most recent ADDR message received,' # Num ADDR PerSec
	line += 'Average number of ADDR messages received per second thus far (the average of all previous Num ADDR PerSec values),' # Avg ADDR PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg ADDR
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax ADDR
	line += 'Average number of bytes that this message took up,' # BytesAvg ADDR
	line += 'Maximum number of bytes that this message took up,' # BytesMax ADDR
	line += 'Total number of INV messages received thus far,' # # INV Msgs
	line += 'Change in number of messages over the change in time for the most recent INV message received,' # Num INV PerSec
	line += 'Average number of INV messages received per second thus far (the average of all previous Num INV PerSec values),' # Avg INV PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg INV
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax INV
	line += 'Average number of bytes that this message took up,' # BytesAvg INV
	line += 'Maximum number of bytes that this message took up,' # BytesMax INV
	line += 'Total number of GETDATA messages received thus far,' # # GETDATA Msgs
	line += 'Change in number of messages over the change in time for the most recent GETDATA message received,' # Num GETDATA PerSec
	line += 'Average number of GETDATA messages received per second thus far (the average of all previous Num GETDATA PerSec values),' # Avg GETDATA PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg GETDATA
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax GETDATA
	line += 'Average number of bytes that this message took up,' # BytesAvg GETDATA
	line += 'Maximum number of bytes that this message took up,' # BytesMax GETDATA
	line += 'Total number of MERKLEBLOCK messages received thus far,' # # MERKLEBLOCK Msgs
	line += 'Change in number of messages over the change in time for the most recent MERKLEBLOCK message received,' # Num MERKLEBLOCK PerSec
	line += 'Average number of MERKLEBLOCK messages received per second thus far (the average of all previous Num MERKLEBLOCK PerSec values),' # Avg MERKLEBLOCK PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg MERKLEBLOCK
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax MERKLEBLOCK
	line += 'Average number of bytes that this message took up,' # BytesAvg MERKLEBLOCK
	line += 'Maximum number of bytes that this message took up,' # BytesMax MERKLEBLOCK
	line += 'Total number of GETBLOCKS messages received thus far,' # # GETBLOCKS Msgs
	line += 'Change in number of messages over the change in time for the most recent GETBLOCKS message received,' # Num GETBLOCKS PerSec
	line += 'Average number of GETBLOCKS messages received per second thus far (the average of all previous Num GETBLOCKS PerSec values),' # Avg GETBLOCKS PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg GETBLOCKS
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax GETBLOCKS
	line += 'Average number of bytes that this message took up,' # BytesAvg GETBLOCKS
	line += 'Maximum number of bytes that this message took up,' # BytesMax GETBLOCKS
	line += 'Total number of GETHEADERS messages received thus far,' # # GETHEADERS Msgs
	line += 'Change in number of messages over the change in time for the most recent GETHEADERS message received,' # Num GETHEADERS PerSec
	line += 'Average number of GETHEADERS messages received per second thus far (the average of all previous Num GETHEADERS PerSec values),' # Avg GETHEADERS PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg GETHEADERS
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax GETHEADERS
	line += 'Average number of bytes that this message took up,' # BytesAvg GETHEADERS
	line += 'Maximum number of bytes that this message took up,' # BytesMax GETHEADERS
	line += 'Total number of TX messages received thus far,' # # TX Msgs
	line += 'Change in number of messages over the change in time for the most recent TX message received,' # Num TX PerSec
	line += 'Average number of TX messages received per second thus far (the average of all previous Num TX PerSec values),' # Avg TX PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg TX
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax TX
	line += 'Average number of bytes that this message took up,' # BytesAvg TX
	line += 'Maximum number of bytes that this message took up,' # BytesMax TX
	line += 'Total number of HEADERS messages received thus far,' # # HEADERS Msgs
	line += 'Change in number of messages over the change in time for the most recent HEADERS message received,' # Num HEADERS PerSec
	line += 'Average number of HEADERS messages received per second thus far (the average of all previous Num HEADERS PerSec values),' # Avg HEADERS PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg HEADERS
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax HEADERS
	line += 'Average number of bytes that this message took up,' # BytesAvg HEADERS
	line += 'Maximum number of bytes that this message took up,' # BytesMax HEADERS
	line += 'Total number of BLOCK messages received thus far,' # # BLOCK Msgs
	line += 'Change in number of messages over the change in time for the most recent BLOCK message received,' # Num BLOCK PerSec
	line += 'Average number of BLOCK messages received per second thus far (the average of all previous Num BLOCK PerSec values),' # Avg BLOCK PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg BLOCK
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax BLOCK
	line += 'Average number of bytes that this message took up,' # BytesAvg BLOCK
	line += 'Maximum number of bytes that this message took up,' # BytesMax BLOCK
	line += 'Total number of GETADDR messages received thus far,' # # GETADDR Msgs
	line += 'Change in number of messages over the change in time for the most recent GETADDR message received,' # Num GETADDR PerSec
	line += 'Average number of GETADDR messages received per second thus far (the average of all previous Num GETADDR PerSec values),' # Avg GETADDR PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg GETADDR
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax GETADDR
	line += 'Average number of bytes that this message took up,' # BytesAvg GETADDR
	line += 'Maximum number of bytes that this message took up,' # BytesMax GETADDR
	line += 'Total number of MEMPOOL messages received thus far,' # # MEMPOOL Msgs
	line += 'Change in number of messages over the change in time for the most recent MEMPOOL message received,' # Num MEMPOOL PerSec
	line += 'Average number of MEMPOOL messages received per second thus far (the average of all previous Num MEMPOOL PerSec values),' # Avg MEMPOOL PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg MEMPOOL
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax MEMPOOL
	line += 'Average number of bytes that this message took up,' # BytesAvg MEMPOOL
	line += 'Maximum number of bytes that this message took up,' # BytesMax MEMPOOL
	line += 'Total number of PING messages received thus far,' # # PING Msgs
	line += 'Change in number of messages over the change in time for the most recent PING message received,' # Num PING PerSec
	line += 'Average number of PING messages received per second thus far (the average of all previous Num PING PerSec values),' # Avg PING PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg PING
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax PING
	line += 'Average number of bytes that this message took up,' # BytesAvg PING
	line += 'Maximum number of bytes that this message took up,' # BytesMax PING
	line += 'Total number of PONG messages received thus far,' # # PONG Msgs
	line += 'Change in number of messages over the change in time for the most recent PONG message received,' # Num PONG PerSec
	line += 'Average number of PONG messages received per second thus far (the average of all previous Num PONG PerSec values),' # Avg PONG PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg PONG
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax PONG
	line += 'Average number of bytes that this message took up,' # BytesAvg PONG
	line += 'Maximum number of bytes that this message took up,' # BytesMax PONG
	line += 'Total number of NOTFOUND messages received thus far,' # # NOTFOUND Msgs
	line += 'Change in number of messages over the change in time for the most recent NOTFOUND message received,' # Num NOTFOUND PerSec
	line += 'Average number of NOTFOUND messages received per second thus far (the average of all previous Num NOTFOUND PerSec values),' # Avg NOTFOUND PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg NOTFOUND
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax NOTFOUND
	line += 'Average number of bytes that this message took up,' # BytesAvg NOTFOUND
	line += 'Maximum number of bytes that this message took up,' # BytesMax NOTFOUND
	line += 'Total number of FILTERLOAD messages received thus far,' # # FILTERLOAD Msgs
	line += 'Change in number of messages over the change in time for the most recent FILTERLOAD message received,' # Num FILTERLOAD PerSec
	line += 'Average number of FILTERLOAD messages received per second thus far (the average of all previous Num FILTERLOAD PerSec values),' # Avg FILTERLOAD PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg FILTERLOAD
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax FILTERLOAD
	line += 'Average number of bytes that this message took up,' # BytesAvg FILTERLOAD
	line += 'Maximum number of bytes that this message took up,' # BytesMax FILTERLOAD
	line += 'Total number of FILTERADD messages received thus far,' # # FILTERADD Msgs
	line += 'Change in number of messages over the change in time for the most recent FILTERADD message received,' # Num FILTERADD PerSec
	line += 'Average number of FILTERADD messages received per second thus far (the average of all previous Num FILTERADD PerSec values),' # Avg FILTERADD PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg FILTERADD
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax FILTERADD
	line += 'Average number of bytes that this message took up,' # BytesAvg FILTERADD
	line += 'Maximum number of bytes that this message took up,' # BytesMax FILTERADD
	line += 'Total number of FILTERCLEAR messages received thus far,' # # FILTERCLEAR Msgs
	line += 'Change in number of messages over the change in time for the most recent FILTERCLEAR message received,' # Num FILTERCLEAR PerSec
	line += 'Average number of FILTERCLEAR messages received per second thus far (the average of all previous Num FILTERCLEAR PerSec values),' # Avg FILTERCLEAR PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg FILTERCLEAR
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax FILTERCLEAR
	line += 'Average number of bytes that this message took up,' # BytesAvg FILTERCLEAR
	line += 'Maximum number of bytes that this message took up,' # BytesMax FILTERCLEAR
	line += 'Total number of SENDHEADERS messages received thus far,' # # SENDHEADERS Msgs
	line += 'Change in number of messages over the change in time for the most recent SENDHEADERS message received,' # Num SENDHEADERS PerSec
	line += 'Average number of SENDHEADERS messages received per second thus far (the average of all previous Num SENDHEADERS PerSec values),' # Avg SENDHEADERS PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg SENDHEADERS
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax SENDHEADERS
	line += 'Average number of bytes that this message took up,' # BytesAvg SENDHEADERS
	line += 'Maximum number of bytes that this message took up,' # BytesMax SENDHEADERS
	line += 'Total number of FEEFILTER messages received thus far,' # # FEEFILTER Msgs
	line += 'Change in number of messages over the change in time for the most recent FEEFILTER message received,' # Num FEEFILTER PerSec
	line += 'Average number of FEEFILTER messages received per second thus far (the average of all previous Num FEEFILTER PerSec values),' # Avg FEEFILTER PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg FEEFILTER
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax FEEFILTER
	line += 'Average number of bytes that this message took up,' # BytesAvg FEEFILTER
	line += 'Maximum number of bytes that this message took up,' # BytesMax FEEFILTER
	line += 'Total number of SENDCMPCT messages received thus far,' # # SENDCMPCT Msgs
	line += 'Change in number of messages over the change in time for the most recent SENDCMPCT message received,' # Num SENDCMPCT PerSec
	line += 'Average number of SENDCMPCT messages received per second thus far (the average of all previous Num SENDCMPCT PerSec values),' # Avg SENDCMPCT PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg SENDCMPCT
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax SENDCMPCT
	line += 'Average number of bytes that this message took up,' # BytesAvg SENDCMPCT
	line += 'Maximum number of bytes that this message took up,' # BytesMax SENDCMPCT
	line += 'Total number of CMPCTBLOCK messages received thus far,' # # CMPCTBLOCK Msgs
	line += 'Change in number of messages over the change in time for the most recent CMPCTBLOCK message received,' # Num CMPCTBLOCK PerSec
	line += 'Average number of CMPCTBLOCK messages received per second thus far (the average of all previous Num CMPCTBLOCK PerSec values),' # Avg CMPCTBLOCK PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg CMPCTBLOCK
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax CMPCTBLOCK
	line += 'Average number of bytes that this message took up,' # BytesAvg CMPCTBLOCK
	line += 'Maximum number of bytes that this message took up,' # BytesMax CMPCTBLOCK
	line += 'Total number of GETBLOCKTXN messages received thus far,' # # GETBLOCKTXN Msgs
	line += 'Change in number of messages over the change in time for the most recent GETBLOCKTXN message received,' # Num GETBLOCKTXN PerSec
	line += 'Average number of GETBLOCKTXN messages received per second thus far (the average of all previous Num GETBLOCKTXN PerSec values),' # Avg GETBLOCKTXN PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg GETBLOCKTXN
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax GETBLOCKTXN
	line += 'Average number of bytes that this message took up,' # BytesAvg GETBLOCKTXN
	line += 'Maximum number of bytes that this message took up,' # BytesMax GETBLOCKTXN
	line += 'Total number of BLOCKTXN messages received thus far,' # # BLOCKTXN Msgs
	line += 'Change in number of messages over the change in time for the most recent BLOCKTXN message received,' # Num BLOCKTXN PerSec
	line += 'Average number of BLOCKTXN messages received per second thus far (the average of all previous Num BLOCKTXN PerSec values),' # Avg BLOCKTXN PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg BLOCKTXN
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax BLOCKTXN
	line += 'Average number of bytes that this message took up,' # BytesAvg BLOCKTXN
	line += 'Maximum number of bytes that this message took up,' # BytesMax BLOCKTXN
	line += 'Total number of REJECT messages received thus far,' # # REJECT Msgs
	line += 'Change in number of messages over the change in time for the most recent REJECT message received,' # Num REJECT PerSec
	line += 'Average number of REJECT messages received per second thus far (the average of all previous Num REJECT PerSec values),' # Avg REJECT PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg REJECT
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax REJECT
	line += 'Average number of bytes that this message took up,' # BytesAvg REJECT
	line += 'Maximum number of bytes that this message took up,' # BytesMax REJECT
	line += 'Total number of undocumented messages received thus far,' # # [UNDOCUMENTED] Msgs
	line += 'Change in number of messages over the change in time for the most recent undocumented message received,' # Num [UNDOCUMENTED] PerSec
	line += 'Average number of undocumented messages received per second thus far (the average of all previous Num [UNDOCUMENTED] PerSec values),' # Avg [UNDOCUMENTED] PerSec
	line += 'Average number of clocks that it took to process this message,' # ClocksAvg [UNDOCUMENTED]
	line += 'Maximum number of clocks that it took to process this message,' # ClocksMax [UNDOCUMENTED]
	line += 'Average number of bytes that this message took up,' # BytesAvg [UNDOCUMENTED]
	line += 'Maximum number of bytes that this message took up,' # BytesMax [UNDOCUMENTED]
	line += 'Number of rows that were skipped before this row (because the number of connections was not equal to the correct value),' # NumSkippedSamples

	return line

prevNumMsgs = {}
prevClocksSum = {}
prevBytesSum = {}

numMsgsPerSecond = {} # Computing the rate
numMsgsPerSecondTime = {}
sumOfMsgsPerSecond = {} # Used to compute the average
countOfMsgsPerSecond = {} # Used to compute the average

# Compute the blocks per second and tx per second
lastBlockcount = 0
lastMempoolSize = 0
numAcceptedBlocksPerSec = 0
numAcceptedBlocksPerSecTime = 0
numAcceptedTxPerSec = 0
numAcceptedTxPerSecTime = 0
acceptedBlocksPerSecSum = 0
acceptedBlocksPerSecCount = 0
acceptedTxPerSecSum = 0
acceptedTxPerSecCount = 0

def parseMessage(message, string, time):
	line = ''
	numMsgs = re.findall(r'([0-9\.]+) msgs', string)[0]
	matches = re.findall(r'\[[0-9\., ]+\]', string)
	if len(matches) != 2:
		return None
	match1 = json.loads(matches[0])
	match2 = json.loads(matches[1])

	numMsgsDiff = int(numMsgs)
	if message in prevNumMsgs:
		numMsgsDiff = int(numMsgs) - int(prevNumMsgs[message])

	clocksSumDiff = int(match1[0])
	clocksAvgDiff = clocksSumDiff
	if message in prevClocksSum:
		clocksSumDiff = int(match1[0]) - int(prevClocksSum[message])
		clocksAvgDiff = 0
		if numMsgsDiff != 0:
			clocksAvgDiff = clocksSumDiff / numMsgsDiff

	bytesSumDiff = int(match2[0])
	bytesAvgDiff = bytesSumDiff
	if message in prevBytesSum:
		bytesSumDiff = int(match2[0]) - int(prevBytesSum[message])
		bytesAvgDiff = 0
		if numMsgsDiff != 0:
			bytesAvgDiff = bytesSumDiff / numMsgsDiff

	if numMsgsDiff != 0: # Calculate the messages per second = numMessages / (timestamp - timestampOfLastMessageOccurance)
		oldTime = 0
		if message in numMsgsPerSecondTime:
			oldTime = numMsgsPerSecondTime[message]

		numMsgsPerSecond[message] = numMsgsDiff / (time - oldTime)
		numMsgsPerSecondTime[message] = time

	msgsPerSecond = 0
	if message in numMsgsPerSecond: # If it exists, read it
		msgsPerSecond = numMsgsPerSecond[message]

	if message not in sumOfMsgsPerSecond: # Initialization
		sumOfMsgsPerSecond[message] = 0
		countOfMsgsPerSecond[message] = 0

	sumOfMsgsPerSecond[message] += msgsPerSecond
	countOfMsgsPerSecond[message] += 1

	line += str(numMsgs) + ','		# Num MSG

	line += str(msgsPerSecond) + ','	# Rate
	line += str(sumOfMsgsPerSecond[message] / countOfMsgsPerSecond[message]) + ','

	line += str(match1[1]) + ','	# Clocks Avg
	line += str(match1[2]) + ','	# Clocks Max
	line += str(match2[1]) + ','	# Bytes Avg
	line += str(match2[2])			# Bytes Max

	prevNumMsgs[message] = numMsgs
	prevClocksSum[message] = match1[0]
	prevBytesSum[message] = match2[0]
	return line


def fetch(now):
	global numPeers, disableLog, numSkippedSamples, lastBlockcount, lastMempoolSize, numAcceptedBlocksPerSec, numAcceptedBlocksPerSecTime, numAcceptedTxPerSec, numAcceptedTxPerSecTime, acceptedBlocksPerSecSum, acceptedBlocksPerSecCount, acceptedTxPerSecSum, acceptedTxPerSecCount, hasReachedConnections
	numPeers = 0
	try:
		# Fetch the bandwidth rate every second regardless of errors, because otherwise after the logging resumes, there will be a sharp jump calculating the difference in bytes per second
		networkBandwidth = getBandwidth()
		numPeers = int(bitcoin('getconnectioncount').strip())

		# Disable all logging messages within bitcoin
		if disableLog:
			bitcoin('log')
			disableLog = False

		# If eclipse attack, verify that the read-to-fake ratio is correct
		if eclipsing:
			success = True
			# Disconnect if there's any extra peers, then verify that the connection count is correct
			realfakestatus = json.loads(bitcoin(f'forcerealfake {eclipse_real_numpeers} {eclipse_fake_numpeers}'))
			if 'At correct peer count' in realfakestatus:
				success = realfakestatus['At correct peer count']
			else:
				success = False
			if not success:
				numSkippedSamples += 1
				print(f'Connections at {numPeers}, waiting for it to reach {maxConnections}, attempt #{numSkippedSamples}')
				# If it has at some point reached the limit, and is not connected for 2 hours, reset the node
				if hasReachedConnections and numSkippedSamples > 0 and numSkippedSamples % 7200 == 0: resetNode()
				return ''
		elif numPeers > connectionSequence[maxConnections]:
			# If not eclipsing, and number of connections is too big, reduce connections
			bitcoin(f'forcerealfake {connectionSequence[maxConnections]} 0')


		if waitForConnectionNum and numPeers < connectionSequence[maxConnections]:
			numSkippedSamples += 1
			print(f'Connections at {numPeers}, waiting for it to reach {connectionSequence[maxConnections]}, attempt #{numSkippedSamples}')
			# If it has at some point reached the limit, and is not connected for 2 hours, reset the node
			if hasReachedConnections and numSkippedSamples > 0 and numSkippedSamples % 7200 == 0: resetNode()
			return ''

		messages = json.loads(bitcoin('getmsginfo'))
		blockcount = int(bitcoin('getblockcount').strip())
		blockdelay = bitcoin('blocktimeoffset').strip()
		headerdelay = bitcoin('headertimeoffset').strip()
		mempoolinfo = json.loads(bitcoin('getmempoolinfo'))
		peerinfo = json.loads(bitcoin('getpeerinfo'))
		resourceUsage = getCPUData()
	except Exception as e:
		print(e)
		raise Exception('Unable to access Bitcoin console...')

	seconds = (now - datetime.datetime(1970, 1, 1)).total_seconds()

	addresses = ''
	totalBanScore = 0
	totalPingTime = 0
	numPingTimes = 0
	numInbound = 0
	numAddnode = 0
	numPruned = 0
	numBloom = 0
	numTXRelayers = 0
	for peer in peerinfo:
		if addresses != '':
			addresses += ' '
		try:
			addresses += peer["addr"]
			totalBanScore += peer["banscore"]
		except: pass
		try:
			totalPingTime += peer["pingtime"]
			numPingTimes += 1
		except: pass
		try:
			if peer['inbound'] != 'false':
				numInbound += 1
		except: pass
		try:
			if 'NETWORK_LIMITED' in peer['servicesnames']:
				numPruned += 1
		except: pass
		try:
			if peer['relaytxes'] != 'false':
				numTXRelayers += 1
		except: pass
		try:
			if peer['addnode'] != 'false':
				numAddnode += 1
		except: pass
		try:
			if 'BLOOM' in peer['servicesnames']:
				numBloom += 1
		except: pass

	if numPingTimes != 0:
		avgPingTime = totalPingTime / numPingTimes
	else:
		avgPingTime = '0'

	line = str(now) + ','
	line += str(seconds) + ','

	line += str(numPeers) + ','
	line += str(numInbound) + ','
	line += str(numPruned) + ','
	line += str(numTXRelayers) + ','
	line += str(numAddnode) + ','
	line += str(numBloom) + ','
	line += str(avgPingTime) + ','
	line += str(totalBanScore) + ','
	line += addresses + ','

	line += resourceUsage['cpu_percent'] + '%,'
	line += resourceUsage['memory_percent'] + '%,'
	line += resourceUsage['memory'] + ','
	line += str(networkBandwidth['download_bps']) + '/' + str(networkBandwidth['upload_bps']) + ','
	line += resourceUsage['full_system'] + '%,'
	#line += '"' + re.sub(r'\s', '', json.dumps(resourceUsage)).replace('"', '""') + '",'
	#line += '"' + re.sub(r'\s', '', getpeerinfo_raw).replace('"', '""') + '",'


	# Compute the blocks per second and tx per second
	blockcountDiff = blockcount - lastBlockcount
	mempoolSizeDiff = mempoolinfo['size'] - lastMempoolSize
	lastBlockcount = blockcount
	lastMempoolSize = mempoolinfo['size']

	#numMsgsDiff / (time - oldTime)
	if blockcountDiff != 0:
		numAcceptedBlocksPerSec = blockcountDiff / (seconds - numAcceptedBlocksPerSecTime)
		numAcceptedBlocksPerSecTime = seconds

	if mempoolSizeDiff != 0:
		numAcceptedTxPerSec = mempoolSizeDiff / (seconds - numAcceptedTxPerSecTime)
		numAcceptedTxPerSecTime = seconds

	acceptedBlocksPerSecSum += numAcceptedBlocksPerSec
	acceptedBlocksPerSecCount += 1
	acceptedTxPerSecSum += numAcceptedTxPerSec
	acceptedTxPerSecCount += 1

	line += str(blockcount) + ','
	line += blockdelay + ' / ' + headerdelay + ','
	line += str(numAcceptedBlocksPerSec) + ','
	line += str(acceptedBlocksPerSecSum / acceptedBlocksPerSecCount) + ','
	line += str(mempoolinfo['size']) + ','
	line += str(mempoolinfo['bytes']) + ','

	line += str(numAcceptedTxPerSec) + ','
	line += str(acceptedTxPerSecSum / acceptedTxPerSecCount) + ','

	line += str(messages['CLOCKS PER SECOND']) + ','
	line += parseMessage('VERSION', messages['VERSION'], seconds) + ','
	line += parseMessage('VERACK', messages['VERACK'], seconds) + ','
	line += parseMessage('ADDR', messages['ADDR'], seconds) + ','
	line += parseMessage('INV', messages['INV'], seconds) + ','
	line += parseMessage('GETDATA', messages['GETDATA'], seconds) + ','
	line += parseMessage('MERKLEBLOCK', messages['MERKLEBLOCK'], seconds) + ','
	line += parseMessage('GETBLOCKS', messages['GETBLOCKS'], seconds) + ','
	line += parseMessage('GETHEADERS', messages['GETHEADERS'], seconds) + ','
	line += parseMessage('TX', messages['TX'], seconds) + ','
	line += parseMessage('HEADERS', messages['HEADERS'], seconds) + ','
	line += parseMessage('BLOCK', messages['BLOCK'], seconds) + ','
	line += parseMessage('GETADDR', messages['GETADDR'], seconds) + ','
	line += parseMessage('MEMPOOL', messages['MEMPOOL'], seconds) + ','
	line += parseMessage('PING', messages['PING'], seconds) + ','
	line += parseMessage('PONG', messages['PONG'], seconds) + ','
	line += parseMessage('NOTFOUND', messages['NOTFOUND'], seconds) + ','
	line += parseMessage('FILTERLOAD', messages['FILTERLOAD'], seconds) + ','
	line += parseMessage('FILTERADD', messages['FILTERADD'], seconds) + ','
	line += parseMessage('FILTERCLEAR', messages['FILTERCLEAR'], seconds) + ','
	line += parseMessage('SENDHEADERS', messages['SENDHEADERS'], seconds) + ','
	line += parseMessage('FEEFILTER', messages['FEEFILTER'], seconds) + ','
	line += parseMessage('SENDCMPCT', messages['SENDCMPCT'], seconds) + ','
	line += parseMessage('CMPCTBLOCK', messages['CMPCTBLOCK'], seconds) + ','
	line += parseMessage('GETBLOCKTXN', messages['GETBLOCKTXN'], seconds) + ','
	line += parseMessage('BLOCKTXN', messages['BLOCKTXN'], seconds) + ','
	line += parseMessage('REJECT', messages['REJECT'], seconds) + ','
	line += parseMessage('[UNDOCUMENTED]', messages['[UNDOCUMENTED]'], seconds) + ','

	line += str(numSkippedSamples) + ','

	numSkippedSamples = 0
	hasReachedConnections = True
	return line

def newFile(file):
	global fileSampleNumber
	fileSampleNumber += 1
	success = False
	while not success:
		try:
			file.close()
		except:
			pass
		try:
			file = open(os.path.expanduser(f'~/Desktop/Logs_AlternatingConnections/{getFileName()}'), 'w+')
			file.write(fetchHeader() + '\n')
			success = True
		except Exception as e:
			print('ERROR: Failed to create new file')
			print(e)
			print('Retrying...')
			time.sleep(3)
	return file

def fixBitcoinConf():
	global datadir, maxConnections, numconnectionsString, useBitcoinConf
	with open(datadir + '/bitcoin.conf', 'r') as configFile:
		configData = configFile.read()
	configData = re.sub(r'\s*maxconnections\s*=\s*[0-9]+', '', configData)
	configData = re.sub(r'\s*numconnections\s*=\s*[0-9]+', '', configData)
	configData = re.sub(r'\s*numblocksonlyconnections\s*=\s*[0-9]+', '', configData)
	if useBitcoinConf: # If the user specified to not use bitcoin.conf, add in the connections limit
		if numconnectionsString != 'maxconnections':
			configData += '\n' + 'maxconnections=' + str(connectionSequence[maxConnections])
		configData += '\n' + numconnectionsString + '=' + str(connectionSequence[maxConnections])
	print(configData)
	with open(datadir + '/bitcoin.conf', 'w') as configFile:
		configFile.write(configData)


def resetNode():
	global disableLog
	success = False
	while not success or bitcoinUp():
		try:
			print('Stopping bitcoin...')
			bitcoin('stop')
			time.sleep(3)
			fixBitcoinConf()
			success = True
		except:
			pass
		time.sleep(3)


	success = False
	while not success:
		try:
			print('Starting Bitcoin...')
			startBitcoin()
			success = True
		except:
			pass
		time.sleep(3)

	disableLog = True

	if startupDelay > 0:
		print(f'Startup delay for {startupDelay} seconds...')
		time.sleep(startupDelay)
		print('Startup delay complete.')

def log(file, targetDateTime, count):
	global maxConnections, fileSampleNumber, numPeers, hasReachedConnections
	try:
		now = datetime.datetime.now()
		line = fetch(now)
		if len(line) != 0:
			print(f'Line: {str(count)}, File: {fileSampleNumber}, Connections: {numPeers}, Off by {(now - targetDateTime).total_seconds()} seconds.')
			file.write(line + '\n')
			file.flush()
			##if count >= 3600:
			##	file.close()
			##	return
			if(count % rowsPerFile == 0):
				if not eclipsing: # If eclipse attack, max connections does not increase
					maxConnections = (maxConnections + 1) % len(connectionSequence)
					hasReachedConnections = False
				try:
					file = newFile(file)
					resetNode()
				except Exception as e:
					print('ERROR: ' + e)

				targetDateTime = datetime.datetime.now()

			count += 1
	except Exception as e:
		print('\nLOGGER PAUSED. Hold on...')
		print(e)
		try:
			if not bitcoinUp():
				resetNode()
		except:
			resetNode()
		#print(traceback.format_exc())


	targetDateTime = targetDateTime + datetime.timedelta(seconds = numSecondsPerSample)
	delay = getDelay(targetDateTime)
	Timer(delay, log, [file, targetDateTime, count]).start()

def getDelay(time):
	return (time - datetime.datetime.now()).total_seconds()

# Returns the file name of the current file, given the global configuration variables
def getFileName():
	global rowsPerFile, numSecondsPerSample, eclipsing, fileSampleNumber, eclipse_real_numpeers, eclipse_fake_numpeers, eclipse_drop_rate, waitForConnectionNum
	minutes = rowsPerFile / numSecondsPerSample / 60
	if eclipsing:
		return f'Sample {fileSampleNumber}, genuine = {eclipse_real_numpeers}, fake = {eclipse_fake_numpeers}, drop = {eclipse_drop_rate}, minutes = {minutes}.csv'
	elif waitForConnectionNum:
		return f'Sample {fileSampleNumber}, numConnections = {connectionSequence[maxConnections]}, minutes = {minutes}.csv'
	else:
		return f'Sample {fileSampleNumber}, maxConnections = {connectionSequence[maxConnections]}, minutes = {minutes}.csv'

def init():
	global fileSampleNumber, maxConnections, numconnectionsString, useBitcoinConf, hasReachedConnections
	global waitForConnectionNum, eclipsing, eclipse_real_numpeers, eclipse_fake_numpeers, eclipse_drop_rate

	fileSampleNumber = 0		# File number to start at
	#while len(glob.glob(os.path.expanduser(f'~/Desktop/Logs_AlternatingConnections/Sample {fileSampleNumber + 1}*'))) > 0:
	#	fileSampleNumber += 1

	filesList = '\n'.join(glob.glob(os.path.expanduser('~/Desktop/Logs_AlternatingConnections/Sample *')))
	filesList = filesList.replace(os.path.expanduser('~/Desktop/Logs_AlternatingConnections/'), '')

	# Find the maximum sample number to continue logging off of
	for file in filesList.split('\n'):
		match = re.match(r'Sample ([0-9]+),', file)
		if match == None: continue
		if int(match.group(1)) > fileSampleNumber:
			fileSampleNumber = int(match.group(1))

	print(filesList)
	print()
	print(f'Starting at file "Sample {fileSampleNumber + 1} ..."')
	print()

	print('Use the new "numconnections" in bitcoin.conf instead of "maxconnections"?')
	print('- Option 1: numconnections (New command; strictly enforces numpeers)')
	print('- Option 2: maxconnections (Lightly enforces an upper bound to numpeers)')
	print()
	useNumconnections = ''
	while useNumconnections.strip() not in ['1', '2']:
		useNumconnections = input('(pick an option number): ').strip()
	numconnectionsString = 'numconnections' if useNumconnections == '1' else 'maxconnections'

	if not useNumconnections:
		print()
		useBitcoinConf = input('Should bitcoin.conf be used for maxconnections? (otherwise the default 10 peers) (y/n) ').lower() in ['y', 'yes']
		if not useBitcoinConf:
			print()
			print('WARNING: bitcoin.conf will not be used to change the number of peers, so the default number of peers will be used')
			numconnectionsString = 'maxconnections'
	else:
		useBitcoinConf = True

	print()

	eclipsing = input('Is this an eclipse attack? (y/n) ').lower() in ['y', 'yes']
	print()
	if eclipsing:
		eclipse_fake_numpeers = int(input(f'How many FAKE connections would you like? '))
		eclipse_real_numpeers = int(input(f'How many REAL connections would you like? '))
		eclipse_drop_rate = float(input(f'What is the packet drop rate (0 to 1)? '))
		maxConnections = eclipse_real_numpeers + eclipse_fake_numpeers
		print(f'Total number of connections: {maxConnections}')
		waitForConnectionNum = True
	else:
		if len(connectionSequence) == 1:
			maxConnections = 0
		else:
			print('How many connections should be initially made?')
			for i, v in enumerate(connectionSequence):
				print(f'- Option {i + 1}: {v} connections')
			print()
			maxConnections = -1
			while maxConnections < 0 or maxConnections >= len(connectionSequence):
				maxConnections = int(input(f'(pick an option number): ')) - 1
		print()
		waitForConnectionNum = input(f'Do you want to log ONLY when the number of connections is at {connectionSequence[maxConnections]}? (y/n) ').lower() in ['y', 'yes']

	print()
	print()
	print('OVERVIEW')
	print()
	print(f'Starting file name: {getFileName()}')
	print(f'Number of seconds per file: {rowsPerFile}')
	print(f'Number of connections sequence: {connectionSequence}')
	print(f'Use bitcoin.conf: {useBitcoinConf}')
	print(f'Use maxconnections or numconnections: {numconnectionsString}')
	if eclipsing:
		print(f'Real connections: {eclipse_real_numpeers}')
		print(f'Fake connections: {eclipse_fake_numpeers}')
		print(f'Packet drop rate: {eclipse_drop_rate}')
	print(f'Connections: {connectionSequence[maxConnections]}')
	print(f'Pause if connections don\'t equal {connectionSequence[maxConnections]}: {waitForConnectionNum}')
	print()
	confirm = input(f'Does all of this look correct? (y/n) ').lower() in ['y', 'yes']
	if not confirm:
		print('Then please restart the script')
		sys.exit()
		return

	os.system('clear')

	path = os.path.expanduser('~/Desktop/Logs_AlternatingConnections')
	if not os.path.exists(path):
		print('Creating "Logs_AlternatingConnections" folder on desktop...')
		os.makedirs(path)

	file = newFile(None)
	resetNode()
	targetDateTime = datetime.datetime.now()
	log(file, targetDateTime, 1)


init()
