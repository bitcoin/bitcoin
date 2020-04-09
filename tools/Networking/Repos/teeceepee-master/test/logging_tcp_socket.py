from teeceepee.tcp import TCPSocket

class LoggingTCPSocket(TCPSocket):
    def __init__(self, listener, verbose=0):
        self.received_packets = []
        self.states = []
        super(self.__class__, self).__init__(listener, verbose)

    def handle(self, packet):
        self.received_packets.append(packet)
        super(self.__class__, self).handle(packet)

    def __setattr__(self, attr, value):
        if attr == 'state':
            self.states.append(value)
        super(self.__class__, self).__setattr__(attr, value)
