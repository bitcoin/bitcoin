import os

def terminal(cmd):
	return os.popen(cmd).read()

def main():
	os.system('clear')
	timeout = int(input('How many seconds should a new file be created? '))

	path = os.path.expanduser('~/Desktop/pcap_logs')
	if not os.path.exists(path):
	    os.makedirs(path)

	count = 1
	while True:
		print(f'\nWriting to {path}/pcap_log_{count}')
		terminal(f'sudo timeout {timeout}s tcpdump -w {path}/pcap_log_{count}')
		count += 1

main()