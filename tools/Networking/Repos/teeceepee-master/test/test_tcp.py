#!/usr/bin/env ipython

FAKE_IP = "10.0.4.4"
MAC_ADDR = "60:67:20:eb:7b:bc"
from scapy.all import srp, Ether, ARP
from unittest.case import SkipTest
from logging_tcp_socket import LoggingTCPSocket
import time
import socket
from teeceepee.tcp_listener import TCPListener

# The tests can't run as not-root
RUN = True
try:
    for _ in range(4):
        srp(Ether(dst="ff:ff:ff:ff:ff:ff")/ARP(psrc=FAKE_IP, hwsrc=MAC_ADDR), verbose=0)
    listener = TCPListener(FAKE_IP)
except socket.error:
    # Are you sure you're running as root?
    RUN = False


google_ip = "173.194.43.39"

def test_connect_google():
    if not RUN: raise SkipTest
    conn = LoggingTCPSocket(listener)

    conn.connect(google_ip, 80)
    time.sleep(2)
    conn.close()
    time.sleep(2)
    assert conn.state == "CLOSED"
    assert len(conn.received_packets) == 2
    assert conn.states == ["CLOSED", "SYN-SENT", "ESTABLISHED", "FIN-WAIT-1", "CLOSED"]

def test_get_google_homepage():
    if not RUN: raise SkipTest
    payload = "GET / HTTP/1.0\r\nHost: %s\r\n\r\n" % google_ip
    conn = LoggingTCPSocket(listener)

    conn.connect(google_ip, 80)
    conn.send(payload)
    time.sleep(3)
    data = conn.recv()
    conn.close()
    time.sleep(3)

    assert "google" in data
    assert conn.state == "CLOSED"
    assert len(conn.received_packets) >= 4
    packet_flags = [p.sprintf("%TCP.flags%") for p in conn.received_packets]
    assert packet_flags[0] == "SA"
    assert "F" in packet_flags[-2]
    assert packet_flags[-1] == "A"
    assert "PA" in packet_flags

    assert conn.states == ["CLOSED", "SYN-SENT", "ESTABLISHED", "LAST-ACK", "CLOSED"]
