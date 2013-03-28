#!/usr/bin/env python

import zmq

def main(argv):

    context = zmq.Context()
    socket = context.socket(zmq.SUB)

    if len(argv) == 1:
        socket.setsockopt(zmq.SUBSCRIBE, "")
    else:
        for arg in argv[1:]:
            print "Subscribing to: ", arg
            socket.setsockopt(zmq.SUBSCRIBE, arg)

    print "Connecting to host: ", argv[0]
    socket.connect(argv[0])

    while True:
        msg = socket.recv()
        print "%s" % (msg[8:])

if __name__ == '__main__':
    import sys

    if len(sys.argv) < 2:
        print "usage: bitcoin-zmq.py <address to connect to> [,<subscribe filter>...]"
        print ""
        print "Filters to subscribe to:"
        print ""
        print "New block.......: ec747b90"
        print "New transaction.: cf954abb"
        print "New ipaddress...: a610612a"
        print ""
        print "Example:"
        print "--------"
        print "(Assuming you are running 'bitcoind -zmqpubbind=\"tcp://lo:61771\"')"
        print ""
        print "  ./bitcoin-zmq.py tcp://localhost:61771"
        print "Will not filter anything so you get all the messages"
        print ""
        print "  ./bitcoin-zmq.py tcp://localhost:61771 ec747b90"
        print "Will get you only the new block messages"

        raise SystemExit

    main(sys.argv[1:])
