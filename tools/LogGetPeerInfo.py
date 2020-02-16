import os
import re
import json
import datetime
from threading import Timer

numSecondsPerSample = 1

#def getmyIP():
#	return os.popen('curl \'http://myexternalip.com/raw\'').read() + ':8333'

def bitcoin(cmd):
	return os.popen('./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

def fetchHeader():
	line = 'Timestamp,'
	line += 'Timestamp (Seconds),'
	line += 'Num Peers,'
	line += 'ID,'
	line += 'ADDR,'
	line += 'ADDRLOCAL,'
	line += 'ADDRBIND,'
	line += 'SERVICES,'
	line += 'RELAYTXES,'
	line += 'LASTSEND,'
	line += 'LASTRECV,'
	line += 'BYTESSENT,'
	line += 'BYTESRECV,'
	line += 'CONNTIME,'
	line += 'TIMEOFFSET,'
	line += 'PINGTIME,'
	line += 'MINPING,'
	line += 'VERSION,'
	line += 'SUBVER,'
	line += 'INBOUND,'
	line += 'ADDNODE,'
	line += 'STARTINGHEIGHT,'
	line += 'BANSCORE,'
	line += 'SYNCED_HEADERS,'
	line += 'SYNCED_BLOCKS,'
	line += 'INFLIGHT,'
	line += 'WHITELISTED,'
	line += 'MINFEEFILTER,'

	line += 'VERSION_BYTESSENT,'
	line += 'VERACK_BYTESSENT,'
	line += 'ADDR_BYTESSENT,'
	line += 'INV_BYTESSENT,'
	line += 'GETDATA_BYTESSENT,'
	line += 'MERKLEBLOCK_BYTESSENT,'
	line += 'GETBLOCKS_BYTESSENT,'
	line += 'GETHEADERS_BYTESSENT,'
	line += 'TX_BYTESSENT,'
	line += 'HEADERS_BYTESSENT,'
	line += 'BLOCK_BYTESSENT,'
	line += 'GETADDR_BYTESSENT,'
	line += 'MEMPOOL_BYTESSENT,'
	line += 'PING_BYTESSENT,'
	line += 'PONG_BYTESSENT,'
	line += 'NOTFOUND_BYTESSENT,'
	line += 'FILTERLOAD_BYTESSENT,'
	line += 'FILTERADD_BYTESSENT,'
	line += 'FILTERCLEAR_BYTESSENT,'
	line += 'SENDHEADERS_BYTESSENT,'
	line += 'FEEFILTER_BYTESSENT,'
	line += 'SENDCMPCT_BYTESSENT,'
	line += 'CMPCTBLOCK_BYTESSENT,'
	line += 'GETBLOCKTXN_BYTESSENT,'
	line += 'BLOCKTXN_BYTESSENT,'
	line += 'REJECT_BYTESSENT,'

	line += 'VERSION_BYTESRECV,'
	line += 'VERACK_BYTESRECV,'
	line += 'ADDR_BYTESRECV,'
	line += 'INV_BYTESRECV,'
	line += 'GETDATA_BYTESRECV,'
	line += 'MERKLEBLOCK_BYTESRECV,'
	line += 'GETBLOCKS_BYTESRECV,'
	line += 'GETHEADERS_BYTESRECV,'
	line += 'TX_BYTESRECV,'
	line += 'HEADERS_BYTESRECV,'
	line += 'BLOCK_BYTESRECV,'
	line += 'GETADDR_BYTESRECV,'
	line += 'MEMPOOL_BYTESRECV,'
	line += 'PING_BYTESRECV,'
	line += 'PONG_BYTESRECV,'
	line += 'NOTFOUND_BYTESRECV,'
	line += 'FILTERLOAD_BYTESRECV,'
	line += 'FILTERADD_BYTESRECV,'
	line += 'FILTERCLEAR_BYTESRECV,'
	line += 'SENDHEADERS_BYTESRECV,'
	line += 'FEEFILTER_BYTESRECV,'
	line += 'SENDCMPCT_BYTESRECV,'
	line += 'CMPCTBLOCK_BYTESRECV,'
	line += 'GETBLOCKTXN_BYTESRECV,'
	line += 'BLOCKTXN_BYTESRECV,'
	line += 'REJECT_BYTESRECV,'
	return line

def fetch(now):
	line = ''
	messages = json.loads(bitcoin('getpeerinfo'))
	for message in messages:
		line += str(now) + ','
		line += str((now - datetime.datetime(1970, 1, 1)).total_seconds()) + ','
		line += str(len(messages)) + ','
		line += str(message["id"] if "id" in message else "") + ','
		line += str(message["addr"] if "addr" in message else "") + ','
		line += str(message["addrlocal"] if "addrlocal" in message else "") + ','
		line += str(message["addrbind"] if "addrbind" in message else "") + ','
		line += str(message["services"] if "services" in message else "") + ','
		line += str(message["relaytxes"] if "relaytxes" in message else "") + ','
		line += str(message["lastsend"] if "lastsend" in message else "") + ','
		line += str(message["lastrecv"] if "lastrecv" in message else "") + ','
		line += str(message["bytessent"] if "bytessent" in message else "") + ','
		line += str(message["bytesrecv"] if "bytesrecv" in message else "") + ','
		line += str(message["conntime"] if "conntime" in message else "") + ','
		line += str(message["timeoffset"] if "timeoffset" in message else "") + ','
		line += str(message["pingtime"] if "pingtime" in message else "") + ','
		line += str(message["minping"] if "minping" in message else "") + ','
		line += str(message["version"] if "version" in message else "") + ','
		line += str(message["subver"] if "subver" in message else "") + ','
		line += str(message["inbound"] if "inbound" in message else "") + ','
		line += str(message["addnode"] if "addnode" in message else "") + ','
		line += str(message["startingheight"] if "startingheight" in message else "") + ','
		line += str(message["banscore"] if "banscore" in message else "") + ','
		line += str(message["synced_headers"] if "synced_headers" in message else "") + ','
		line += str(message["synced_blocks"] if "synced_blocks" in message else "") + ','
		line += str(message["inflight"] if "inflight" in message else "") + ','
		line += str(message["whitelisted"] if "whitelisted" in message else "") + ','
		line += str(message["minfeefilter"] if "minfeefilter" in message else "") + ','

		bytessent_per_msg = message["bytessent_per_msg"] if "bytessent_per_msg" in message else {}
		line += str(bytessent_per_msg["version"] if "version" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["verack"] if "verack" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["addr"] if "addr" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["inv"] if "inv" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["getdata"] if "getdata" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["merkleblock"] if "merkleblock" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["getblocks"] if "getblocks" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["getheaders"] if "getheaders" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["tx"] if "tx" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["headers"] if "headers" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["block"] if "block" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["getaddr"] if "getaddr" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["mempool"] if "mempool" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["ping"] if "ping" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["pong"] if "pong" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["notfound"] if "notfound" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["filterload"] if "filterload" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["filteradd"] if "filteradd" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["filterclear"] if "filterclear" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["sendheaders"] if "sendheaders" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["feefilter"] if "feefilter" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["sendcmpct"] if "sendcmpct" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["cmpctblock"] if "cmpctblock" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["getblocktxn"] if "getblocktxn" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["blocktxn"] if "blocktxn" in bytessent_per_msg else 0) + ','
		line += str(bytessent_per_msg["reject"] if "reject" in bytessent_per_msg else 0) + ','

		bytesrecv_per_msg = message["bytesrecv_per_msg"] if "bytesrecv_per_msg" in message else {}
		line += str(bytesrecv_per_msg["version"] if "version" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["verack"] if "verack" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["addr"] if "addr" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["inv"] if "inv" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["getdata"] if "getdata" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["merkleblock"] if "merkleblock" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["getblocks"] if "getblocks" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["getheaders"] if "getheaders" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["tx"] if "tx" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["headers"] if "headers" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["block"] if "block" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["getaddr"] if "getaddr" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["mempool"] if "mempool" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["ping"] if "ping" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["pong"] if "pong" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["notfound"] if "notfound" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["filterload"] if "filterload" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["filteradd"] if "filteradd" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["filterclear"] if "filterclear" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["sendheaders"] if "sendheaders" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["feefilter"] if "feefilter" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["sendcmpct"] if "sendcmpct" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["cmpctblock"] if "cmpctblock" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["getblocktxn"] if "getblocktxn" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["blocktxn"] if "blocktxn" in bytesrecv_per_msg else 0) + ','
		line += str(bytesrecv_per_msg["reject"] if "reject" in bytesrecv_per_msg else 0) + ','
		line += '\n'
	return line

def log(file, targetDateTime, count = 1):
	try:
		now = datetime.datetime.now()
		print(f'Line {str(count)} off by {(now - targetDateTime).total_seconds()} seconds.')
		file.write(fetch(now))
		file.flush()
		#if count >= 3600:
		#	file.close()
		#	return
		count += 1
	except json.decoder.JSONDecodeError:
		print(f'\nLOGGER PAUSED AT LINE {count}. Start the Bitcoin node when you are ready.')
		pass
	targetDateTime = targetDateTime + datetime.timedelta(seconds = numSecondsPerSample)
	delay = getDelay(targetDateTime)
	Timer(delay, log, [file, targetDateTime, count]).start()

def getDelay(time):
	return (time - datetime.datetime.now()).total_seconds()

def main():
	os.system('clear')
	print('GETMSGINFO LOGGER'.center(80))
	print('-' * 80)
	description = input('Enter a description for the file name ("GetPeerInfo [DESCRIPTION].csv"): ')
	filename = 'GetPeerInfo.csv'
	if description != '':
		filename = f'GetPeerInfo {description}.csv'

	time = input('Enter what time it should begin (example: "2:45pm", "" for right now): ')
	file = open(os.path.expanduser(f'~/Desktop/{filename}'), 'w+')

	if time == '':
		targetDateTime = datetime.datetime.now()
		delay = 0
	else:
		targetDateTime = datetime.datetime.strptime(datetime.datetime.now().strftime('%d-%m-%Y') + ' ' + time, '%d-%m-%Y %I:%M%p')
		delay = getDelay(targetDateTime)

	if(delay > 60):
		print('Time left: ' + str(delay / 60) + ' minutes.\n')
	else:
		print('Time left: ' + str(delay) + ' seconds.\n')

	file.write(fetchHeader() + '\n')
	Timer(delay, log, [file, targetDateTime, 1]).start()


main()
