""" Mock listener for testing. """

from teeceepee.tcp_listener import TCPListener


class MockListener(TCPListener):
    """
    Doesn't actually send packets.
    """
    def __init__(self, ip_address="127.0.0.1"):
        super(self.__class__, self).__init__(ip_address)
        self.received_packets = []

    def send(self, packet):
        self.received_packets.append(packet)

    def start_daemon(self):
        pass

