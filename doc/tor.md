TOR SUPPORT IN DASH CORE
=======================

It is possible to run Dash Core as a Tor hidden service, and connect to such services.

The following directions assume you have a Tor proxy running on port 9050. Many
distributions default to having a SOCKS proxy listening on port 9050, but others
may not. In particular, the Tor Browser Bundle defaults to listening on port 9150.
See [Tor Project FAQ:TBBSocksPort](https://www.torproject.org/docs/faq.html.en#TBBSocksPort)
for how to properly configure Tor.


1. Run Dash Core behind a Tor proxy
----------------------------------

The first step is running Dash Core behind a Tor proxy. This will already make all
outgoing connections be anonymized, but more is possible.

	-proxy=ip:port  Set the proxy server. If SOCKS5 is selected (default), this proxy
	                server will be used to try to reach .onion addresses as well.

	-onion=ip:port  Set the proxy server to use for tor hidden services. You do not
	                need to set this if it's the same as -proxy. You can use -noonion
	                to explicitly disable access to hidden service.

	-listen         When using -proxy, listening is disabled by default. If you want
	                to run a hidden service (see next section), you'll need to enable
	                it explicitly.

	-connect=X      When behind a Tor proxy, you can specify .onion addresses instead
	-addnode=X      of IP addresses or hostnames in these parameters. It requires
	-seednode=X     SOCKS5. In Tor mode, such addresses can also be exchanged with
	                other P2P nodes.

	-onlynet=tor    Only connect to .onion nodes and drop IPv4/6 connections.

An example how to start the client if the Tor proxy is running on local host on
port 9050 and only allows .onion nodes to connect:

	./dashd -onion=127.0.0.1:9050 -onlynet=tor -listen=0 -addnode=ssapp53tmftyjmjb.onion

In a typical situation, this suffices to run behind a Tor proxy:

	./dashd -proxy=127.0.0.1:9050


2. Run a Dash Core hidden server
-------------------------------

If you configure your Tor system accordingly, it is possible to make your node also
reachable from the Tor network. Add these lines to your /etc/tor/torrc (or equivalent
config file):

	HiddenServiceDir /var/lib/tor/dashcore-service/
	HiddenServicePort 9999 127.0.0.1:9999
	HiddenServicePort 19999 127.0.0.1:19999

The directory can be different of course, but (both) port numbers should be equal to
your dashd's P2P listen port (9999 by default).

	-externalip=X   You can tell Dash Core about its publicly reachable address using
	                this option, and this can be a .onion address. Given the above
	                configuration, you can find your onion address in
	                /var/lib/tor/dashcore-service/hostname. Onion addresses are given
	                preference for your node to advertise itself with, for connections
	                coming from unroutable addresses (such as 127.0.0.1, where the
	                Tor proxy typically runs).

	-listen         You'll need to enable listening for incoming connections, as this
	                is off by default behind a proxy.

	-discover       When -externalip is specified, no attempt is made to discover local
	                IPv4 or IPv6 addresses. If you want to run a dual stack, reachable
	                from both Tor and IPv4 (or IPv6), you'll need to either pass your
	                other addresses using -externalip, or explicitly enable -discover.
	                Note that both addresses of a dual-stack system may be easily
	                linkable using traffic analysis.

In a typical situation, where you're only reachable via Tor, this should suffice:

	./dashd -proxy=127.0.0.1:9050 -externalip=ssapp53tmftyjmjb.onion -listen

(obviously, replace the Onion address with your own). It should be noted that you still
listen on all devices and another node could establish a clearnet connection, when knowing
your address. To mitigate this, additionally bind the address of your Tor proxy:

	./dashd ... -bind=127.0.0.1

If you don't care too much about hiding your node, and want to be reachable on IPv4
as well, use `discover` instead:

	./dashd ... -discover

and open port 9999 on your firewall (or use -upnp).

If you only want to use Tor to reach onion addresses, but not use it as a proxy
for normal IPv4/IPv6 communication, use:

	./dashd -onion=127.0.0.1:9050 -externalip=ssapp53tmftyjmjb.onion -discover


3. List of known Dash Core Tor relays
------------------------------------

* [darkcoinie7ghp67.onion](http://darkcoinie7ghp67.onion/)
* [drktalkwaybgxnoq.onion](http://drktalkwaybgxnoq.onion/)
* [drkcoinooditvool.onion](http://drkcoinooditvool.onion/)
* [darkcoxbtzggpmcc.onion](http://darkcoxbtzggpmcc.onion/)
* [ssapp53tmftyjmjb.onion](http://ssapp53tmftyjmjb.onion/)
* [j2dfl3cwxyxpbc7s.onion](http://j2dfl3cwxyxpbc7s.onion/)
* [vf6d2mxpuhh2cbxt.onion](http://vf6d2mxpuhh2cbxt.onion/)
* [rj24sicr6i4vsnkv.onion](http://rj24sicr6i4vsnkv.onion/)
* [wrwx2dy7jyh32o53.onion](http://wrwx2dy7jyh32o53.onion/)
* [f5ekot4ajkbe23gt.onion](http://f5ekot4ajkbe23gt.onion/)
* [dshtord4mqvgzqev.onion](http://dshtord4mqvgzqev.onion/)


4. Automatically listen on Tor
--------------------------------

Starting with Tor version 0.2.7.1 it is possible, through Tor's control socket
API, to create and destroy 'ephemeral' hidden services programmatically.
Dash Core has been updated to make use of this.

This means that if Tor is running (and proper authentication has been configured),
Dash Core automatically creates a hidden service to listen on. This will positively 
affect the number of available .onion nodes.

This new feature is enabled by default if Dash Core is listening, and
a connection to Tor can be made. It can be configured with the `-listenonion`,
`-torcontrol` and `-torpassword` settings. To show verbose debugging
information, pass `-debug=tor`.

Connecting to Tor's control socket API requires one of two authentication methods to be 
configured. For cookie authentication the user running dashd must have write access 
to the `CookieAuthFile` specified in Tor configuration. In some cases this is 
preconfigured and the creation of a hidden service is automatic. If permission problems 
are seen with `-debug=tor` they can be resolved by adding both the user running tor and 
the user running dashd to the same group and setting permissions appropriately. On 
Debian-based systems the user running dashd can be added to the debian-tor group, 
which has the appropriate permissions. An alternative authentication method is the use 
of the `-torpassword` flag and a `hash-password` which can be enabled and specified in 
Tor configuration.

4. Privacy recommendations
---------------------------

- Do not add anything but bitcoin ports to the hidden service created in section 2.
  If you run a web service too, create a new hidden service for that.
  Otherwise it is trivial to link them, which may reduce privacy. Hidden
  services created automatically (as in section 3) always have only one port
  open.
