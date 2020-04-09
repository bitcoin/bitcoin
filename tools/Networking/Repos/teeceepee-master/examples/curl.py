# Tell scapy not to log warnings
import logging
logging.getLogger("scapy.runtime").setLevel(logging.ERROR)
import sys
import socket
import fcntl
import struct 
import time
from scapy.all import srp, Ether, ARP
sys.path.insert(0, ".")
from teeceepee.tcp import TCPSocket
from teeceepee.tcp_listener import TCPListener

# This needs to be in your subnet, and not your IP address.
# Try something like 10.0.4.4 or 192.168.8.8

def arp_spoof(fake_ip, mac_address):
    try:
        for _ in xrange(5):
            srp(Ether(dst="ff:ff:ff:ff:ff:ff")/ARP(psrc=fake_ip, hwsrc=mac_address), verbose=0, timeout=0.05)
            time.sleep(0.05)
    except socket.error:
        # Are you sure you're running as root?
        print "ERROR: You need to run this script as root."
        sys.exit()

def parse(url):
    """
    Parses an URL and returns the hostname and the rest
    """
    # We *definitely* don't support https
    if 'https' in url:
        print "We don't support https."
        sys.exit(1)
    if '://' in url:
        url = url.split('://')[1]

    parts = url.split('/')
    hostname = parts[0]
    path = '/' + '/'.join(parts[1:])
    return hostname, path

def get_page(url, fake_ip):
    hostname, path = parse(url)
    request = "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n" % (path, hostname)
    listener = TCPListener(fake_ip)
    conn = TCPSocket(listener)

    conn.connect(hostname, 80)
    conn.send(request)
    data = conn.recv(10000, timeout=1)
    conn.close()
    return data

def get_mac_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', ifname[:15]))
    return ''.join(['%02x:' % ord(char) for char in info[18:24]])[:-1]


def print_usage():
    print """USAGE: sudo python curl.py 192.168.4.4 example.com [interface]

The IP address you specify should be
- on the same subnet as you and
- not used by anybody else on your network
- not your real IP address

The default interface is wlan0

If it doesn't work, sometimes trying a few extra times helps"""
    sys.exit()
    
if __name__ == "__main__":
    # You need to specify your interface here
    if len(sys.argv) < 3:
        print_usage()
    interface = "wlan0"
    if len(sys.argv) == 4:
        interface = sys.argv[4]
    FAKE_IP, page = sys.argv[1:3]
    MAC_ADDR = get_mac_address(interface)
    arp_spoof(FAKE_IP, MAC_ADDR)
    contents = get_page(page, FAKE_IP)
    print contents
