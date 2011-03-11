#! /usr/bin/python
# $Id: testupnpigd.py,v 1.4 2008/10/11 10:27:20 nanard Exp $
# MiniUPnP project
# Author : Thomas Bernard
# This Sample code is public domain.
# website : http://miniupnp.tuxfamily.org/

# import the python miniupnpc module
import miniupnpc
import socket
import BaseHTTPServer

# function definition
def list_redirections():
	i = 0
	while True:
		p = u.getgenericportmapping(i)
		if p==None:
			break
		print i, p
		i = i + 1

#define the handler class for HTTP connections
class handler_class(BaseHTTPServer.BaseHTTPRequestHandler):
	def do_GET(self):
		self.send_response(200)
		self.end_headers()
		self.wfile.write("OK MON GARS")

# create the object
u = miniupnpc.UPnP()
#print 'inital(default) values :'
#print ' discoverdelay', u.discoverdelay
#print ' lanaddr', u.lanaddr
#print ' multicastif', u.multicastif
#print ' minissdpdsocket', u.minissdpdsocket
u.discoverdelay = 200;

try:
	print 'Discovering... delay=%ums' % u.discoverdelay
	ndevices = u.discover()
	print ndevices, 'device(s) detected'

	# select an igd
	u.selectigd()
	# display information about the IGD and the internet connection
	print 'local ip address :', u.lanaddr
	externalipaddress = u.externalipaddress()
	print 'external ip address :', externalipaddress
	print u.statusinfo(), u.connectiontype()

	#instanciate a HTTPd object. The port is assigned by the system.
	httpd = BaseHTTPServer.HTTPServer((u.lanaddr, 0), handler_class)
	eport = httpd.server_port

	# find a free port for the redirection
	r = u.getspecificportmapping(eport, 'TCP')
	while r != None and eport < 65536:
		eport = eport + 1
		r = u.getspecificportmapping(eport, 'TCP')

	print 'trying to redirect %s port %u TCP => %s port %u TCP' % (externalipaddress, eport, u.lanaddr, httpd.server_port)

	b = u.addportmapping(eport, 'TCP', u.lanaddr, httpd.server_port,
	                    'UPnP IGD Tester port %u' % eport, '')
	if b:
		print 'Success. Now waiting for some HTTP request on http://%s:%u' % (externalipaddress ,eport)
		try:
			httpd.handle_request()
			httpd.server_close()
		except KeyboardInterrupt, details:
			print "CTRL-C exception!", details
		b = u.deleteportmapping(eport, 'TCP')
		if b:
			print 'Successfully deleted port mapping'
		else:
			print 'Failed to remove port mapping'
	else:
		print 'Failed'

	httpd.server_close()

except Exception, e:
	print 'Exception :', e
