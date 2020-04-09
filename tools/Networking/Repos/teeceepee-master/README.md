[![Build Status](https://travis-ci.org/jvns/teeceepee.png)](https://travis-ci.org/jvns/teeceepee)

teeceepee
=========


This is a tiny TCP stack implemented in pure Python, for fun and
learning. It is essentially a finicky and slow replacement for
`socket` where the interface is not quite right. But you can use it to
get webpages!

It's built on top of [scapy](http://www.secdev.org/projects/scapy/),
which takes care of all the packet parsing and construction. This
module handles actually sending and receiving packets.

There is an example of using it to get a webpage in
`examples/curl.py`. The example is quite finicky -- it uses ARP
spoofing to bypass the kernel's TCP stack, which sometimes results in
it just Not Working. Running it a few times sometimes fixes this
problem.

```
sudo python examples/curl.py 10.0.4.4 example.com
```

works for me. If it doesn't work for you, let me know! If you're not
connected to the internet on `wlan0`, you'll need to specify an
interface as well.

I use Linux and Python 2.7 and it works for me. I would expect it to
not work on a BSD, and certainly not on Windows.

### Features

* Can connect to hosts, send packets, and reassemble the replies in
  the correct order
* Will ignore out-of-order packets
* Basically it is appropriate for implementing `curl`, but not for
  sending data

### Missing features

* Sending more than one packet at once
* Resending dropped packets
* Handling more than one incoming connection at once
* `bind()` hasn't been tested in the wild at all, just unit tested. So
  it probably doesn't work.

### Difficulties

* It needs to run as root because it needs to use raw sockets.
* TCP stacks aren't really *supposed* to start and stop. In
  principle this should really run as a daemon, but it doesn't.
* It needs to bypass the kernel, since normally the TCP stack there
  will intercept any incoming packets and reset the connection. So we
  listen on a fake IP address and send gratuitous ARPs to the router.
  This is why `curl.py` needs your network interface and a fake IP
  address.
* It's slow, because Python. If you watch it in Wireshark, it does a
  hilarious thing where it gets backed up sending ACKs and then sends
  a load of ACKs at the end and takes forever to close the connection.
* Sometimes the ARP spoofing or sniffing doesn't quite work. Usually
  if I run it 5 times it will work.
