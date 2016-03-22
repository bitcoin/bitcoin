###
###  Automatic seeding over HTTP
###  (C) 2016 Alex D.
###
###  http://www.gnu.org/licenses/agpl-3.0.en.html
###

import json
import random
import urllib2

import os
import platform

###  This module is required to function properly: 
###      https://github.com/jgarzik/python-bitcoinrpc

from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException

sources = ['http://dalexhz1.cloudapp.net', 'http://dalexhz2.cloudapp.net', 'http://dalexhz4.cloudapp.net', 'http://dalexhz5.cloudapp.net']

def confFile():
    folder = ''
    if platform.system() == 'Windows':
        folder = os.path.join(os.path.join(os.environ['APPDATA'], 'NovaCoin'))
    else:
        if platform.system() == 'Darwin':
            folder = os.path.expanduser('~/Library/Application Support/NovaCoin/')
        else:
            folder = os.path.expanduser('~/.novacoin')

    return os.path.join(folder, 'novacoin.conf')

conf_path = confFile()
if not os.path.exists(conf_path):
    parser.error('''Novacoin configuration file not found. Manually enter your RPC password.\r\n'''
        '''If you actually haven't created a configuration file, you should create one at %s with the text like this:\r\n'''
        '''\r\n'''
        '''server=1\r\n'''
        '''rpcuser=yourname\r\n'''
        '''rpcpassword=%x\r\n'''
        '''\r\n'''
        '''Keep that password secret! After creating the file, restart Novacoin.''' % (conf_path, random.randrange(2**128)))

conf = open(conf_path, 'rb').read()
contents = {}

for line in conf.splitlines(True):
    if '#' in line:
        line = line[:line.index('#')]
    if '=' not in line:
        continue
    k, v = line.split('=', 1)
    contents[k.strip()] = v.strip()

if 'rpcpassword' not in contents.keys():
    parser.error(
        '''RPC password is not found in the %s file.''' % (conf_path))

rpcuser = 'novacoin'
rpcpassword = contents['rpcpassword']
rpcport = 8344
rpclisten = '127.0.0.1'

if 'rpcport' in contents.keys():
    rpcport = contents['rpcport']

if 'rpcuser' in contents.keys():
    rpcuser = contents['rpcuser']

if 'rpclisten' in contents.keys():
    rpcuser = contents['rpclisten']

url = "http://"+rpcuser+":"+rpcpassword+"@"+rpclisten+":"+rpcport+"/"

access = AuthServiceProxy(url)
data = json.load(urllib2.urlopen(random.choice(sources)))

for node in data:
    print 'Adding', node
    try:
        access.addnode(node, 'add')
    except:
        print 'Already added'
        pass
    