from scapy.all import TCP, IP, Padding
import random
import time

class BadPacketError(Exception):
    pass

def get_payload(packet):
    while not isinstance(packet, TCP):
        packet = packet.payload
    return packet.payload

class TCPSocket(object):
    def __init__(self, listener, verbose=0):
        self.state = "CLOSED"
        self.verbose = verbose
        self.src_ip = listener.ip_address
        self.recv_buffer = ""
        self.listener = listener
        self.seq = self._generate_seq()
        self.last_ack_sent = 0


    @staticmethod
    def _has_load(packet):
        payload = get_payload(packet)
        if isinstance(payload, Padding):
            return False
        return bool(payload)

    def _set_dest(self, host, port):
        self.dest_port = port
        self.dest_ip = host
        self.ip_header = IP(dst=self.dest_ip, src=self.src_ip)

    def connect(self, host, port):
        self._set_dest(host, port)
        self.src_port = self.listener.get_port()
        self.listener.open(self.src_ip, self.src_port, self)
        self._send_syn()

    def bind(self, host, port):
        # Right now, listen for only one connection.
        self.src_ip = host
        self.src_port = port
        self.listener.open(self.src_ip, self.src_port, self)
        self.state = "LISTEN"

    @staticmethod
    def _generate_seq():
        return random.randint(0, 100000)

    def _send(self, flags="", load=None):
        """Every packet we send should go through here."""
        packet = TCP(dport=self.dest_port,
                     sport=self.src_port,
                     seq=self.seq,
                     ack=self.last_ack_sent,
                     flags=flags)
        # Add the IP header
        full_packet = self.ip_header / packet
        # Add the payload
        if load:
            full_packet = full_packet / load
        # Send the packet over the wire
        self.listener.send(full_packet)
        # Update the sequence number with the number of bytes sent
        if load is not None:
            self.seq += len(load)

    def _send_syn(self):
        self.state = "SYN-SENT"
        self._send(flags="S")

    def _send_ack(self, flags="", load=None):
        """We actually don't need to do much here!"""
        self._send(flags=flags + "A", load=load)

    def close(self):
        if self.state == "CLOSED":
            return
        self.state = "FIN-WAIT-1"
        self._send_ack(flags="F")

    @staticmethod
    def next_seq(packet):
        # really not right.
        tcp_flags = packet.sprintf("%TCP.flags%")
        if TCPSocket._has_load(packet):
            return packet.seq + len(packet.load)
        elif 'S' in tcp_flags or 'F' in tcp_flags:
            return packet.seq + 1
        else:
            return packet.seq

    def _close(self):
        self.state = "CLOSED"
        self.listener.close(self.src_ip, self.src_port)

    def handle(self, packet):
        if self.last_ack_sent and self.last_ack_sent != packet.seq:
            # We're not in a place to receive this packet. Drop it.
            return

        self.last_ack_sent = max(self.next_seq(packet), self.last_ack_sent)

        recv_flags = packet.sprintf("%TCP.flags%")

        # Handle all the cases for self.state explicitly
        if self._has_load(packet):
            self.recv_buffer += packet.load
            self._send_ack()
        elif "R" in recv_flags:
            self._close()
        elif "S" in recv_flags:
            if self.state == "LISTEN":
                self.state = "SYN-RECEIVED"
                self._set_dest(packet.payload.src, packet.sport)
                self._send_ack(flags="S")
            elif self.state == "SYN-SENT":
                self.seq += 1
                self.state = "ESTABLISHED"
                self._send_ack()
        elif "F" in recv_flags:
            if self.state == "ESTABLISHED":
                self.seq += 1
                self.state = "LAST-ACK"
                self._send_ack(flags="F")
            elif self.state == "FIN-WAIT-1":
                self.seq += 1
                self._send_ack()
                self._close()
        elif "A" in recv_flags:
            if self.state == "SYN-RECEIVED":
                self.state = "ESTABLISHED"
            elif self.state == "LAST-ACK":
                self._close()
        else:
            raise BadPacketError("Oh no!")

    def send(self, payload):
        # Block
        while self.state != "ESTABLISHED":
            time.sleep(0.001)
        # Do the actual send
        self._send_ack(load=payload, flags="P")


    def recv(self, size, timeout=None):
        start_time = time.time()
        # Block until the connection is closed
        while len(self.recv_buffer) < size:
            time.sleep(0.001)
            if self.state in ["CLOSED", "LAST-ACK"]:
                break
            if timeout < (time.time() - start_time):
                break
        recv = self.recv_buffer[:size]
        self.recv_buffer = self.recv_buffer[size:]
        return recv
