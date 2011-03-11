#! /usr/bin/python
# MiniUPnP project
# Author : Thomas Bernard
# This Sample code is public domain.
# website : http://miniupnp.tuxfamily.org/

# import the python miniupnpc module
import miniupnpc
import sys

# create the object
u = miniupnpc.UPnP()
print 'inital(default) values :'
print ' discoverdelay', u.discoverdelay
print ' lanaddr', u.lanaddr
print ' multicastif', u.multicastif
print ' minissdpdsocket', u.minissdpdsocket
u.discoverdelay = 200;
#u.minissdpdsocket = '../minissdpd/minissdpd.sock'
# discovery process, it usualy takes several seconds (2 seconds or more)
print 'Discovering... delay=%ums' % u.discoverdelay
print u.discover(), 'device(s) detected'
# select an igd
try:
  u.selectigd()
except Exception, e:
  print 'Exception :', e
  sys.exit(1)
# display information about the IGD and the internet connection
print 'local ip address :', u.lanaddr
print 'external ip address :', u.externalipaddress()
print u.statusinfo(), u.connectiontype()

#print u.addportmapping(64000, 'TCP',
#                       '192.168.1.166', 63000, 'port mapping test', '')
#print u.deleteportmapping(64000, 'TCP')

port = 0
proto = 'UDP'
# list the redirections :
i = 0
while True:
	p = u.getgenericportmapping(i)
	if p==None:
		break
	print i, p
	(port, proto, (ihost,iport), desc, c, d, e) = p
	#print port, desc
	i = i + 1

print u.getspecificportmapping(port, proto)

