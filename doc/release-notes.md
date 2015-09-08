(note: this is a temporary file, to be added-to by anybody, and moved to
release-notes at release time)

Notable changes
===============

SSL support for RPC dropped
----------------------------

SSL support for RPC, previously enabled by the option `rpcssl` has been dropped
from both the client and the server. This was done in preparation for removing
the dependency on OpenSSL for the daemon completely.

Trying to use `rpcssl` will result in an error:

    Error: SSL mode for RPC (-rpcssl) is no longer supported.

If you are one of the few people that relies on this feature, a flexible
migration path is to use `stunnel`. This is an utility that can tunnel
arbitrary TCP connections inside SSL. On e.g. Ubuntu it can be installed with:

    sudo apt-get install stunnel4

Then, to tunnel a SSL connection on 28332 to a RPC server bound on localhost on port 18332 do:

    stunnel -d 28332 -r 127.0.0.1:18332 -p stunnel.pem -P ''

It can also be set up system-wide in inetd style.

Another way to re-attain SSL would be to setup a httpd reverse proxy. This solution
would allow the use of different authentication, loadbalancing, on-the-fly compression and
caching. A sample config for apache2 could look like:

    Listen 443

    NameVirtualHost *:443
    <VirtualHost *:443>

    SSLEngine On
    SSLCertificateFile /etc/apache2/ssl/server.crt
    SSLCertificateKeyFile /etc/apache2/ssl/server.key

    <Location /bitcoinrpc>
        ProxyPass http://127.0.0.1:8332/
        ProxyPassReverse http://127.0.0.1:8332/
        # optional enable digest auth
        # AuthType Digest
        # ...
        
        # optional bypass bitcoind rpc basic auth
        # RequestHeader set Authorization "Basic <hash>"
        # get the <hash> from the shell with: base64 <<< bitcoinrpc:<password>
    </Location>

    # Or, balance the load:
    # ProxyPass / balancer://balancer_cluster_name

    </VirtualHost>

Random-cookie RPC authentication
---------------------------------

When no `-rpcpassword` is specified, the daemon now uses a special 'cookie'
file for authentication. This file is generated with random content when the
daemon starts, and deleted when it exits. Its contents are used as
authentication token. Read access to this file controls who can access through
RPC. By default it is stored in the data directory but its location can be
overridden with the option `-rpccookiefile`.

This is similar to Tor's CookieAuthentication: see
https://www.torproject.org/docs/tor-manual.html.en

This allows running bitcoind without having to do any manual configuration.

Low-level RPC API changes
--------------------------

- Monetary amounts can be provided as strings. This means that for example the
  argument to sendtoaddress can be "0.0001" instead of 0.0001. This can be an
  advantage if a JSON library insists on using a lossy floating point type for
  numbers, which would be dangerous for monetary amounts.

Option parsing behavior
-----------------------

Command line options are now parsed strictly in the order in which they are
specified. It used to be the case that `-X -noX` ends up, unintuitively, with X
set, as `-X` had precedence over `-noX`. This is no longer the case. Like for
other software, the last specified value for an option will hold.

0.12.0 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### RPC and REST

### Configuration and command-line options

### Block and transaction handling

### P2P protocol and network code

### Validation

### Build system

### Wallet

### GUI

### Tests

### Miscellaneous

- Removed bitrpc.py from contrib

