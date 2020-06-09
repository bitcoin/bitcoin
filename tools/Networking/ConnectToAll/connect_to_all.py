import json
import os
import random
import time

# Send commands to the Bitcoin Core Console
def bitcoin(cmd):
	return os.popen('./../../../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

file = open('Bitnodes.json', 'r', encoding = 'utf8')
data = json.load(file)
addresses = []

for item in data:
	if item == 'nodes':
		nodes = data[item]
		print(f'Nodes ({len(nodes)}):')
		for address in nodes:
			addresses.append(address)
	else:
		print(f'{item.capitalize()}: {data[item]}')

file.close()
random.shuffle(addresses)

for i, address in enumerate(addresses):
	print(f'{i}: Connecting to {address}')
	bitcoin(f'addnode {address} onetry')
	#time.sleep(0.01)
