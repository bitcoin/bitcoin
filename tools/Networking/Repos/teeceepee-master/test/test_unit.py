"""
Unit tests for TCPSocket.

Mocks a listener instead of sending real packets.

"""

from teeceepee.tcp import TCPSocket, get_payload
from scapy.all import IP, TCP, Ether, rdpcap, Padding
from mock_listener import MockListener


def test_syn():
    """When we connect, we should send a SYN."""
    listener = MockListener()
    conn = TCPSocket(listener)
    conn.connect("localhost", 80)

    assert conn.state == "SYN-SENT"
    pkts = listener.received_packets
    assert len(pkts) == 1
    assert pkts[0].sprintf("%TCP.flags%") == "S"


def test_handshake_client():
    """When we handshake, we should send a SYN and an ACK"""
    packet_log = rdpcap("test/inputs/tiniest-session.pcap")
    listener, conn = create_session(packet_log)

    syn, syn_ack, ack = packet_log[:3]
    listener.dispatch(syn_ack)

    # Check that they look okay
    our_syn, our_ack = listener.received_packets
    our_syn.seq = syn.seq # These aren't going to match anyway because reasons
    check_mostly_same(syn, our_syn)
    check_mostly_same(ack, our_ack)


def test_send_push_ack():
    """
    When we send a message, it should look the same as in the PCAP log
    """
    packet_log = rdpcap("test/inputs/localhost-wget.pcap")
    listener, conn = create_session(packet_log)

    _, syn_ack, _, push_ack = packet_log[:4]
    listener.dispatch(syn_ack)
    assert conn.state == "ESTABLISHED"

    # Extract the payload (3 levels down: Ether, IP, TCP)
    payload = get_payload(push_ack)
    conn.send(payload)

    # Check to make sure the PUSH-ACK packet that gets sent looks good
    our_push_ack = listener.received_packets[-1]
    check_mostly_same(our_push_ack, push_ack)


def test_fin_ack_client():
    """
    When we run conn.close(), it should end the connection properly.
    """
    packet_log = rdpcap("test/inputs/tiniest-session.pcap")
    listener, conn = create_session(packet_log)

    _, syn_ack_log, _, our_fin_ack_log, their_fin_ack_log, our_ack_log = packet_log
    listener.dispatch(syn_ack_log)

    conn.close()
    listener.dispatch(their_fin_ack_log)

    assert len(listener.received_packets) == 4

    our_ack = listener.received_packets[-1]
    check_mostly_same(our_ack, our_ack_log)

    assert conn.state == "CLOSED"


def test_recv_one_packet():
    """
    conn.recv() should get the contents of one packet
    """
    packet_log = rdpcap("test/inputs/localhost-wget.pcap")
    listener, conn = create_session(packet_log)

    _, syn_ack, _, push_ack = packet_log[:4]

    listener.dispatch(syn_ack)
    payload = get_payload(push_ack)
    conn.send(payload)

    # Check that the PUSH/ACK sequence is the same
    check_replay(listener, conn, packet_log[4:7])

    # Check that recv() actually works
    conn.state = "CLOSED"
    assert conn.recv(1000) == str(packet_log[5].payload.payload.payload)

def test_recv_many_packets_in_order():
    """
    We should be able to put multiple packets together, if they come in order
    """
    packet_log = rdpcap("test/inputs/wget-36000-nums.pcap")
    listener, conn = create_session(packet_log)
    _, syn_ack, _, push_ack = packet_log[:4]

    listener.dispatch(syn_ack)
    payload = get_payload(push_ack)
    conn.send(payload)

    check_replay(listener, conn, packet_log[4:], check=False)

    # Check that the contents of the packet is right
    conn.state = "CLOSED"
    recv = conn.recv(40000)
    assert recv[-36001:-1]  == "1234567890" * 3600

def test_recv_many_packets_out_of_order():
    """
    We should be able to put multiple packets together, if they come
    out of order and are repeated.
    """
    packet_log = rdpcap("test/inputs/wget-36000-nums.pcap")
    listener, conn = create_session(packet_log)
    _, syn_ack, _, push_ack = packet_log[:4]

    listener.dispatch(syn_ack)
    payload = get_payload(push_ack)
    conn.send(payload)

    p1, p2, p3  = packet_log[5], packet_log[7], packet_log[8]

    # Send the packets out of order and repeated
    listener.dispatch(p2)
    listener.dispatch(p3)
    listener.dispatch(p2)
    listener.dispatch(p1) # Right
    listener.dispatch(p3)
    listener.dispatch(p2) # Right
    listener.dispatch(p3) # Right

    # Check that the contents of the packet is right
    # This is a good test because one of the packets starts with a 6 or
    # something
    conn.state = "CLOSED"
    recv = conn.recv(38000)
    assert recv[-36001:-1]  == "1234567890" * 3600

def test_bind_handshake():
    """
    We can do a handshake when we're the server (someone else initiates the
    SYN).
    """
    packet_log = rdpcap("test/inputs/tiniest-session.pcap")
    syn, syn_ack, ack, client_fin_ack, server_fin_ack, client_ack = packet_log

    listener = MockListener()
    conn = TCPSocket(listener)
    conn.seq = syn_ack.seq
    conn.bind(syn.payload.dst, syn.dport)

    listener.dispatch(syn)
    listener.dispatch(ack)

    assert len(listener.received_packets) == 1
    assert conn.state == "ESTABLISHED"
    check_mostly_same(listener.received_packets[0], syn_ack)


def test_tiny_session_server():
    """
    Test the whole tiny session, from a server POV.
    """
    packet_log = rdpcap("test/inputs/tiniest-session.pcap")
    syn, syn_ack = packet_log[:2]

    listener = MockListener()
    conn = TCPSocket(listener)
    conn.seq = syn_ack.seq

    conn.bind(syn.payload.dst, syn.dport)

    check_replay(listener, conn, packet_log)


def test_closes_on_reset():
    """The connection should close when it receives a RST packet"""
    listener = MockListener()

    packet_log = rdpcap("test/inputs/tiniest-session.pcap")
    listener, conn = create_session(packet_log)
    syn_ack = packet_log[1]

    listener.dispatch(syn_ack)

    reset = syn_ack.copy()
    reset.payload.payload.flags = "R"
    reset.seq += 1
    listener.dispatch(reset)

    assert conn.state == "CLOSED"


def test_sends_reset():
    """The connection should send a RST packet when it's closed."""
    listener = MockListener()

    packet_log = rdpcap("test/inputs/tiniest-session.pcap")
    listener, conn = create_session(packet_log)
    conn._close()

    syn_ack = packet_log[1]
    listener.dispatch(syn_ack)

    last_packet = listener.received_packets[-1]
    assert last_packet.sprintf("%TCP.flags%") == "R"


def create_session(packet_log):
    """
    Helper function to create a client session from a PCAP log. Sets up the
    sequence numbers to match with the sequence numbers in the PCAP.
    """
    listener = MockListener()
    syn = packet_log[0]
    listener.source_port = syn.sport - 1
    conn = TCPSocket(listener)
    conn.connect(syn.payload.dst, syn.dport)
    # Change the sequence number so that we can test it
    conn.seq = syn.seq
    return listener, conn


def check_mostly_same(pkt1, pkt2):
    """
    Checks that the important things are the same (seq, ack, TCP flags, packet
    contents)
    """
    assert pkt1.seq == pkt2.seq
    assert pkt1.ack == pkt2.ack
    assert pkt1.sprintf("%TCP.flags%") == pkt2.sprintf("%TCP.flags%")
    assert get_payload(pkt1) == get_payload(pkt2)


def check_replay(listener, conn, packet_log, check=True):
    """
    Check if replaying the packets gives the same result
    """
    ip, port = conn.src_ip, conn.src_port
    is_from_source = lambda x: x.payload.dst == ip and x.dport == port

    incoming = [x for x in packet_log if is_from_source(x)]
    outgoing = [x for x in packet_log if not is_from_source(x)]

    for pkt in incoming:
        listener.dispatch(pkt)

    if not check:
        return

    our_outgoing = listener.received_packets[-len(outgoing):]

    for ours, actual in zip(our_outgoing, outgoing):
        check_mostly_same(ours, actual)


def test_has_load():
    pkt = Ether() / IP() / TCP() / Padding("\x00\x00")
    assert not TCPSocket._has_load(pkt)
