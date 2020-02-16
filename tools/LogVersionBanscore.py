import os
import re
import json
import datetime
from threading import Timer

numSecondsPerSample = 0.02

#def getmyIP():
#	return os.popen('curl \'http://myexternalip.com/raw\'').read() + ':8333'

def bitcoin(cmd):
	return os.popen('./../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

def fetchHeader():
	line = 'Timestamp,'
	line += 'Timestamp (Seconds),'
	line += 'Timestamp Diff,'
	line += 'Address,'
	line += 'Banscore,'

	line += '# VERSION,'
	line += '# VERSION Diff,'
	return line


lastTimestamp = 0
prevNumMsgs = {}

def parseMessage(message, string):
	line = ''
	numMsgs = re.findall(r'([-0-9\.]+) msgs', string)[0]
	matches = re.findall(r'\[[-0-9\., ]+\]', string)
	if len(matches) != 2:
		return None
	match1 = json.loads(matches[0])
	match2 = json.loads(matches[1])

	numMsgsDiff = int(numMsgs)
	if message in prevNumMsgs:
		numMsgsDiff = int(numMsgs) - int(prevNumMsgs[message])

	line += str(numMsgs) + ','		# Num
	line += str(numMsgsDiff) + ','	# Num Diff

	prevNumMsgs[message] = numMsgs
	return line

def fetch(now):
	global lastTimestamp
	line = ''
	line += str(now) + ',' # Timestamp

	
	timestamp = (now - datetime.datetime(1970, 1, 1)).total_seconds()
	timestampDiff = timestamp - lastTimestamp
	lastTimestamp = timestamp

	line += str(timestamp) + ',' # Timestamp (Seconds)
	line += str(timestampDiff) + ',' # Timestamp Diff
	connections = json.loads(bitcoin('list'))
	addressLogged = False
	for addr in connections:
		line += addr + ',' # Address
		line += str(connections[addr]) + ',' # Banscore
		addressLogged = True
		break
	if not addressLogged:
		line += ',' # Address
		line += ',' # Banscore

	messages = json.loads(bitcoin('getmsginfo'))
	line += parseMessage('VERSION', messages['VERSION']) + ','
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
	description = input('Enter a description for the file name ("VersionBanscoreLog [DESCRIPTION].csv"): ')
	filename = 'VersionBanscoreLog.csv'
	if description != '':
		filename = f'VersionBanscoreLog {description}.csv'

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
