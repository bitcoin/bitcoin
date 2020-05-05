import os
import re
import json
import time
import datetime
import subprocess
import glob
import traceback
from threading import Timer


startupDelay = 0			# Number of seconds to wait after each node start up
numSecondsPerSample = 1
rowsPerNodeReset = 3000		# Number of rows for cach numconnections file

waitForConnectionNum = True

os.system('clear')

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

#def getmyIP():
#	return os.popen('curl \'http://myexternalip.com/raw\'').read() + ':8333'

def bitcoin(cmd):
	return os.popen('./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

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
	line += 'InactivePeers,'
	line += 'AvgPingTime,'
	line += 'TotalBanScore,'
	line += 'Connections,'

	line += 'BlockHeight,'
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

	line += 'The time in which the sample was taken (in human readable format),' # Timestamp
	line += 'The time in which the sample was taken (in number of seconds since 1/01/1970),' # Timestamp (Seconds)
	line += 'The current number of peer connections,' # NumPeers
	line += 'Number of peers that have not responded to the ping message,' # InactivePeers
	line += 'The average network latency of the peer,' # AvgPingTime
	line += 'The misbehavior score of each peer summed together,' # TotalBanScore
	line += 'A list of the current peer connections,' # Connections
	line += 'The block height of this particular full node,' # BlockHeight
	line += 'The number of unconfirmed transactions currently in the mempool,' # MempoolSize
	line += 'The number of bytes that the mempool is currently taking up,' # MempoolBytes
	line += 'The change in block height over the change in time for the most recent block update,' # AcceptedBlocksPerSec
	line += 'The average of all previous AcceptedBlocksPerSec values,' # AcceptedBlocksPerSecAvg
	line += 'The change in mempool size over the change in time for the most recent mempool size update,' # AcceptedTxPerSec
	line += 'The average of all previous AcceptedTxPerSec values,' # AcceptedTxPerSecAvg
	line += 'The number of clock cycles that occur each second within Bitcoin Core,' # ClocksPerSec
	line += 'The total number of VERSION messages received thus far,' # # VERSION Msgs
	line += 'The change in number of messages over the change in time for the most recent VERSION message received,' # Num VERSION PerSec
	line += 'The average number of VERSION messages received per second thus far (the average of all previous Num VERSION PerSec values),' # Avg VERSION PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg VERSION
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax VERSION
	line += 'The average number of bytes that this message took up,' # BytesAvg VERSION
	line += 'The maximum number of bytes that this message took up,' # BytesMax VERSION
	line += 'The total number of VERACK messages received thus far,' # # VERACK Msgs
	line += 'The change in number of messages over the change in time for the most recent VERACK message received,' # Num VERACK PerSec
	line += 'The average number of VERACK messages received per second thus far (the average of all previous Num VERACK PerSec values),' # Avg VERACK PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg VERACK
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax VERACK
	line += 'The average number of bytes that this message took up,' # BytesAvg VERACK
	line += 'The maximum number of bytes that this message took up,' # BytesMax VERACK
	line += 'The total number of ADDR messages received thus far,' # # ADDR Msgs
	line += 'The change in number of messages over the change in time for the most recent ADDR message received,' # Num ADDR PerSec
	line += 'The average number of ADDR messages received per second thus far (the average of all previous Num ADDR PerSec values),' # Avg ADDR PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg ADDR
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax ADDR
	line += 'The average number of bytes that this message took up,' # BytesAvg ADDR
	line += 'The maximum number of bytes that this message took up,' # BytesMax ADDR
	line += 'The total number of INV messages received thus far,' # # INV Msgs
	line += 'The change in number of messages over the change in time for the most recent INV message received,' # Num INV PerSec
	line += 'The average number of INV messages received per second thus far (the average of all previous Num INV PerSec values),' # Avg INV PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg INV
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax INV
	line += 'The average number of bytes that this message took up,' # BytesAvg INV
	line += 'The maximum number of bytes that this message took up,' # BytesMax INV
	line += 'The total number of GETDATA messages received thus far,' # # GETDATA Msgs
	line += 'The change in number of messages over the change in time for the most recent GETDATA message received,' # Num GETDATA PerSec
	line += 'The average number of GETDATA messages received per second thus far (the average of all previous Num GETDATA PerSec values),' # Avg GETDATA PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg GETDATA
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax GETDATA
	line += 'The average number of bytes that this message took up,' # BytesAvg GETDATA
	line += 'The maximum number of bytes that this message took up,' # BytesMax GETDATA
	line += 'The total number of MERKLEBLOCK messages received thus far,' # # MERKLEBLOCK Msgs
	line += 'The change in number of messages over the change in time for the most recent MERKLEBLOCK message received,' # Num MERKLEBLOCK PerSec
	line += 'The average number of MERKLEBLOCK messages received per second thus far (the average of all previous Num MERKLEBLOCK PerSec values),' # Avg MERKLEBLOCK PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg MERKLEBLOCK
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax MERKLEBLOCK
	line += 'The average number of bytes that this message took up,' # BytesAvg MERKLEBLOCK
	line += 'The maximum number of bytes that this message took up,' # BytesMax MERKLEBLOCK
	line += 'The total number of GETBLOCKS messages received thus far,' # # GETBLOCKS Msgs
	line += 'The change in number of messages over the change in time for the most recent GETBLOCKS message received,' # Num GETBLOCKS PerSec
	line += 'The average number of GETBLOCKS messages received per second thus far (the average of all previous Num GETBLOCKS PerSec values),' # Avg GETBLOCKS PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg GETBLOCKS
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax GETBLOCKS
	line += 'The average number of bytes that this message took up,' # BytesAvg GETBLOCKS
	line += 'The maximum number of bytes that this message took up,' # BytesMax GETBLOCKS
	line += 'The total number of GETHEADERS messages received thus far,' # # GETHEADERS Msgs
	line += 'The change in number of messages over the change in time for the most recent GETHEADERS message received,' # Num GETHEADERS PerSec
	line += 'The average number of GETHEADERS messages received per second thus far (the average of all previous Num GETHEADERS PerSec values),' # Avg GETHEADERS PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg GETHEADERS
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax GETHEADERS
	line += 'The average number of bytes that this message took up,' # BytesAvg GETHEADERS
	line += 'The maximum number of bytes that this message took up,' # BytesMax GETHEADERS
	line += 'The total number of TX messages received thus far,' # # TX Msgs
	line += 'The change in number of messages over the change in time for the most recent TX message received,' # Num TX PerSec
	line += 'The average number of TX messages received per second thus far (the average of all previous Num TX PerSec values),' # Avg TX PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg TX
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax TX
	line += 'The average number of bytes that this message took up,' # BytesAvg TX
	line += 'The maximum number of bytes that this message took up,' # BytesMax TX
	line += 'The total number of HEADERS messages received thus far,' # # HEADERS Msgs
	line += 'The change in number of messages over the change in time for the most recent HEADERS message received,' # Num HEADERS PerSec
	line += 'The average number of HEADERS messages received per second thus far (the average of all previous Num HEADERS PerSec values),' # Avg HEADERS PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg HEADERS
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax HEADERS
	line += 'The average number of bytes that this message took up,' # BytesAvg HEADERS
	line += 'The maximum number of bytes that this message took up,' # BytesMax HEADERS
	line += 'The total number of BLOCK messages received thus far,' # # BLOCK Msgs
	line += 'The change in number of messages over the change in time for the most recent BLOCK message received,' # Num BLOCK PerSec
	line += 'The average number of BLOCK messages received per second thus far (the average of all previous Num BLOCK PerSec values),' # Avg BLOCK PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg BLOCK
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax BLOCK
	line += 'The average number of bytes that this message took up,' # BytesAvg BLOCK
	line += 'The maximum number of bytes that this message took up,' # BytesMax BLOCK
	line += 'The total number of GETADDR messages received thus far,' # # GETADDR Msgs
	line += 'The change in number of messages over the change in time for the most recent GETADDR message received,' # Num GETADDR PerSec
	line += 'The average number of GETADDR messages received per second thus far (the average of all previous Num GETADDR PerSec values),' # Avg GETADDR PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg GETADDR
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax GETADDR
	line += 'The average number of bytes that this message took up,' # BytesAvg GETADDR
	line += 'The maximum number of bytes that this message took up,' # BytesMax GETADDR
	line += 'The total number of MEMPOOL messages received thus far,' # # MEMPOOL Msgs
	line += 'The change in number of messages over the change in time for the most recent MEMPOOL message received,' # Num MEMPOOL PerSec
	line += 'The average number of MEMPOOL messages received per second thus far (the average of all previous Num MEMPOOL PerSec values),' # Avg MEMPOOL PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg MEMPOOL
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax MEMPOOL
	line += 'The average number of bytes that this message took up,' # BytesAvg MEMPOOL
	line += 'The maximum number of bytes that this message took up,' # BytesMax MEMPOOL
	line += 'The total number of PING messages received thus far,' # # PING Msgs
	line += 'The change in number of messages over the change in time for the most recent PING message received,' # Num PING PerSec
	line += 'The average number of PING messages received per second thus far (the average of all previous Num PING PerSec values),' # Avg PING PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg PING
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax PING
	line += 'The average number of bytes that this message took up,' # BytesAvg PING
	line += 'The maximum number of bytes that this message took up,' # BytesMax PING
	line += 'The total number of PONG messages received thus far,' # # PONG Msgs
	line += 'The change in number of messages over the change in time for the most recent PONG message received,' # Num PONG PerSec
	line += 'The average number of PONG messages received per second thus far (the average of all previous Num PONG PerSec values),' # Avg PONG PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg PONG
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax PONG
	line += 'The average number of bytes that this message took up,' # BytesAvg PONG
	line += 'The maximum number of bytes that this message took up,' # BytesMax PONG
	line += 'The total number of NOTFOUND messages received thus far,' # # NOTFOUND Msgs
	line += 'The change in number of messages over the change in time for the most recent NOTFOUND message received,' # Num NOTFOUND PerSec
	line += 'The average number of NOTFOUND messages received per second thus far (the average of all previous Num NOTFOUND PerSec values),' # Avg NOTFOUND PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg NOTFOUND
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax NOTFOUND
	line += 'The average number of bytes that this message took up,' # BytesAvg NOTFOUND
	line += 'The maximum number of bytes that this message took up,' # BytesMax NOTFOUND
	line += 'The total number of FILTERLOAD messages received thus far,' # # FILTERLOAD Msgs
	line += 'The change in number of messages over the change in time for the most recent FILTERLOAD message received,' # Num FILTERLOAD PerSec
	line += 'The average number of FILTERLOAD messages received per second thus far (the average of all previous Num FILTERLOAD PerSec values),' # Avg FILTERLOAD PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg FILTERLOAD
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax FILTERLOAD
	line += 'The average number of bytes that this message took up,' # BytesAvg FILTERLOAD
	line += 'The maximum number of bytes that this message took up,' # BytesMax FILTERLOAD
	line += 'The total number of FILTERADD messages received thus far,' # # FILTERADD Msgs
	line += 'The change in number of messages over the change in time for the most recent FILTERADD message received,' # Num FILTERADD PerSec
	line += 'The average number of FILTERADD messages received per second thus far (the average of all previous Num FILTERADD PerSec values),' # Avg FILTERADD PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg FILTERADD
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax FILTERADD
	line += 'The average number of bytes that this message took up,' # BytesAvg FILTERADD
	line += 'The maximum number of bytes that this message took up,' # BytesMax FILTERADD
	line += 'The total number of FILTERCLEAR messages received thus far,' # # FILTERCLEAR Msgs
	line += 'The change in number of messages over the change in time for the most recent FILTERCLEAR message received,' # Num FILTERCLEAR PerSec
	line += 'The average number of FILTERCLEAR messages received per second thus far (the average of all previous Num FILTERCLEAR PerSec values),' # Avg FILTERCLEAR PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg FILTERCLEAR
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax FILTERCLEAR
	line += 'The average number of bytes that this message took up,' # BytesAvg FILTERCLEAR
	line += 'The maximum number of bytes that this message took up,' # BytesMax FILTERCLEAR
	line += 'The total number of SENDHEADERS messages received thus far,' # # SENDHEADERS Msgs
	line += 'The change in number of messages over the change in time for the most recent SENDHEADERS message received,' # Num SENDHEADERS PerSec
	line += 'The average number of SENDHEADERS messages received per second thus far (the average of all previous Num SENDHEADERS PerSec values),' # Avg SENDHEADERS PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg SENDHEADERS
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax SENDHEADERS
	line += 'The average number of bytes that this message took up,' # BytesAvg SENDHEADERS
	line += 'The maximum number of bytes that this message took up,' # BytesMax SENDHEADERS
	line += 'The total number of FEEFILTER messages received thus far,' # # FEEFILTER Msgs
	line += 'The change in number of messages over the change in time for the most recent FEEFILTER message received,' # Num FEEFILTER PerSec
	line += 'The average number of FEEFILTER messages received per second thus far (the average of all previous Num FEEFILTER PerSec values),' # Avg FEEFILTER PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg FEEFILTER
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax FEEFILTER
	line += 'The average number of bytes that this message took up,' # BytesAvg FEEFILTER
	line += 'The maximum number of bytes that this message took up,' # BytesMax FEEFILTER
	line += 'The total number of SENDCMPCT messages received thus far,' # # SENDCMPCT Msgs
	line += 'The change in number of messages over the change in time for the most recent SENDCMPCT message received,' # Num SENDCMPCT PerSec
	line += 'The average number of SENDCMPCT messages received per second thus far (the average of all previous Num SENDCMPCT PerSec values),' # Avg SENDCMPCT PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg SENDCMPCT
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax SENDCMPCT
	line += 'The average number of bytes that this message took up,' # BytesAvg SENDCMPCT
	line += 'The maximum number of bytes that this message took up,' # BytesMax SENDCMPCT
	line += 'The total number of CMPCTBLOCK messages received thus far,' # # CMPCTBLOCK Msgs
	line += 'The change in number of messages over the change in time for the most recent CMPCTBLOCK message received,' # Num CMPCTBLOCK PerSec
	line += 'The average number of CMPCTBLOCK messages received per second thus far (the average of all previous Num CMPCTBLOCK PerSec values),' # Avg CMPCTBLOCK PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg CMPCTBLOCK
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax CMPCTBLOCK
	line += 'The average number of bytes that this message took up,' # BytesAvg CMPCTBLOCK
	line += 'The maximum number of bytes that this message took up,' # BytesMax CMPCTBLOCK
	line += 'The total number of GETBLOCKTXN messages received thus far,' # # GETBLOCKTXN Msgs
	line += 'The change in number of messages over the change in time for the most recent GETBLOCKTXN message received,' # Num GETBLOCKTXN PerSec
	line += 'The average number of GETBLOCKTXN messages received per second thus far (the average of all previous Num GETBLOCKTXN PerSec values),' # Avg GETBLOCKTXN PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg GETBLOCKTXN
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax GETBLOCKTXN
	line += 'The average number of bytes that this message took up,' # BytesAvg GETBLOCKTXN
	line += 'The maximum number of bytes that this message took up,' # BytesMax GETBLOCKTXN
	line += 'The total number of BLOCKTXN messages received thus far,' # # BLOCKTXN Msgs
	line += 'The change in number of messages over the change in time for the most recent BLOCKTXN message received,' # Num BLOCKTXN PerSec
	line += 'The average number of BLOCKTXN messages received per second thus far (the average of all previous Num BLOCKTXN PerSec values),' # Avg BLOCKTXN PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg BLOCKTXN
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax BLOCKTXN
	line += 'The average number of bytes that this message took up,' # BytesAvg BLOCKTXN
	line += 'The maximum number of bytes that this message took up,' # BytesMax BLOCKTXN
	line += 'The total number of REJECT messages received thus far,' # # REJECT Msgs
	line += 'The change in number of messages over the change in time for the most recent REJECT message received,' # Num REJECT PerSec
	line += 'The average number of REJECT messages received per second thus far (the average of all previous Num REJECT PerSec values),' # Avg REJECT PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg REJECT
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax REJECT
	line += 'The average number of bytes that this message took up,' # BytesAvg REJECT
	line += 'The maximum number of bytes that this message took up,' # BytesMax REJECT
	line += 'The total number of undocumented messages received thus far,' # # [UNDOCUMENTED] Msgs
	line += 'The change in number of messages over the change in time for the most recent undocumented message received,' # Num [UNDOCUMENTED] PerSec
	line += 'The average number of undocumented messages received per second thus far (the average of all previous Num [UNDOCUMENTED] PerSec values),' # Avg [UNDOCUMENTED] PerSec
	line += 'The average number of clocks that it took to process this message,' # ClocksAvg [UNDOCUMENTED]
	line += 'The maximum number of clocks that it took to process this message,' # ClocksMax [UNDOCUMENTED]
	line += 'The average number of bytes that this message took up,' # BytesAvg [UNDOCUMENTED]
	line += 'The maximum number of bytes that this message took up,' # BytesMax [UNDOCUMENTED]
	line += 'The number of rows that were skipped before this row (because the number of connections was not equal to the correct value),' # NumSkippedSamples

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
	global numSkippedSamples, lastBlockcount, lastMempoolSize, numAcceptedBlocksPerSec, numAcceptedBlocksPerSecTime, numAcceptedTxPerSec, numAcceptedTxPerSecTime, acceptedBlocksPerSecSum, acceptedBlocksPerSecCount, acceptedTxPerSecSum, acceptedTxPerSecCount
	numPeers = 0
	try:
		peerinfo = json.loads(bitcoin('getpeerinfo'))
		numPeers = len(peerinfo)
		if waitForConnectionNum and numPeers != maxConnections:
			numSkippedSamples += 1
			print(f'Connections at {numPeers}, waiting for it to reach {maxConnections}, attempt #{numSkippedSamples}')
			return ''
		messages = json.loads(bitcoin('getmsginfo'))
		blockcount = int(bitcoin('getblockcount').strip())
		mempoolinfo = json.loads(bitcoin('getmempoolinfo'))
	except Exception as e:
		raise Exception('Unable to access Bitcoin console...')

	seconds = (now - datetime.datetime(1970, 1, 1)).total_seconds()

	addresses = ''
	totalBanScore = 0
	totalPingTime = 0
	numPingTimes = 0
	noResponsePings = 0
	for peer in peerinfo:
		if addresses != '':
			addresses += ' '
		try:
			addresses += peer['addr']
			totalBanScore += peer['banscore']
		except:
			pass
		try:
			totalPingTime += peer['pingtime']
			numPingTimes += 1
		except:
			noResponsePings += 1


	if numPingTimes != 0:
		avgPingTime = totalPingTime / numPingTimes
	else:
		avgPingTime = '0'
	line = str(now) + ','
	line += str(seconds) + ','
	line += str(numPeers) + ','
	line += str(noResponsePings) + ','
	line += str(avgPingTime) + ','
	line += str(totalBanScore) + ','
	line += addresses + ','


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
	return line

def resetNode(file, numConnections):
	global fileSampleNumber
	success = False
	while not success or bitcoinUp():
		try:
			print('Stopping bitcoin...')
			bitcoin('stop')
			success = True
		except:
			pass
		time.sleep(3)

	fileSampleNumber += 1
	try:
		file.close()
	except:
		pass
	try:
		file = open(os.path.expanduser(f'~/Desktop/Logs_GetMsgInfo_AlternatingConnections/Sample {fileSampleNumber} numConnections {numConnections}.csv'), 'w+')
		file.write(fetchHeader() + '\n')
	except:
		print('ERROR: Failed to create new file.')
	try:
		with open(datadir + '/bitcoin.conf', 'r') as configFile:
			configData = configFile.read()
		configData = re.sub(r'\s*maxconnections\s*=\s*[0-9]+', '', configData)
		configData += '\n' + 'maxconnections=' + str(numConnections)
		with open(datadir + '/bitcoin.conf', 'w') as configFile:
			configFile.write(configData)
	except:
		print('ERROR: Failed to update bitcoin.conf')

	success = False
	while not success:
		try:
			print('Starting Bitcoin...')
			startBitcoin()
			success = True
		except:
			pass

		time.sleep(3)
	if startupDelay > 0:
		print(f'Startup delay for {startupDelay} seconds...')
		time.sleep(startupDelay)
		print('Startup delay complete.')

	return file

def log(file, targetDateTime, count):
	global maxConnections, fileSampleNumber
	try:
		now = datetime.datetime.now()
		line = fetch(now)
		if len(line) != 0:
			print(f'Line: {str(count)}, File: {fileSampleNumber}, Connections: {maxConnections}, Off by {(now - targetDateTime).total_seconds()} seconds.')
			file.write(line + '\n')
			file.flush()
			#if count >= 3600:
			#	file.close()
			#	return
			if(count % rowsPerNodeReset == 0):
				maxConnections = 1 + maxConnections % 10 # Bound the max connections to 10 peers
				try:
					file = resetNode(file, maxConnections)
				except Exception as e:
					print('ERROR: ' + e)

				targetDateTime = datetime.datetime.now()

			count += 1
	except Exception as e:
		print('\nLOGGER PAUSED. Hold on...')
		print(e)
		#print(traceback.format_exc())


	targetDateTime = targetDateTime + datetime.timedelta(seconds = numSecondsPerSample)
	delay = getDelay(targetDateTime)
	Timer(delay, log, [file, targetDateTime, count]).start()

def getDelay(time):
	return (time - datetime.datetime.now()).total_seconds()

def init():
	global fileSampleNumber
	global maxConnections
	fileSampleNumber = 0		# File number to start at
	while len(glob.glob(os.path.expanduser(f'~/Desktop/Logs_GetMsgInfo_AlternatingConnections/Sample {fileSampleNumber + 1} *'))) > 0:
		fileSampleNumber += 1

	filesList = '\n'.join(glob.glob(os.path.expanduser('~/Desktop/Logs_GetMsgInfo_AlternatingConnections/Sample *')))
	filesList = filesList.replace(os.path.expanduser('~/Desktop/Logs_GetMsgInfo_AlternatingConnections/'), '')
	print(filesList)
	print()
	maxConnections = int(input(f'Starting at file "Sample {fileSampleNumber + 1} numConnections X.csv"\n\nHow many connections should be initially made? (From 1 to 10)\nPreferably start with 1 connection: '))
	print()
	path = os.path.expanduser('~/Desktop/Logs_GetMsgInfo_AlternatingConnections')
	if not os.path.exists(path):
	    os.makedirs(path)

	file = resetNode(None, maxConnections)#open(os.path.expanduser(f'~/Desktop/Logs_GetMsgInfo_AlternatingConnections/sample0.csv'), 'w+')
	targetDateTime = datetime.datetime.now()
	log(file, targetDateTime, 1)


init()
