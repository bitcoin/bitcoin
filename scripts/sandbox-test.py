import StringIO
import json
import unittest

import sandbox


class TestNodeconfig(unittest.TestCase):

    def test_parse_nodeconfig(self):
        config = (
            "alias1 127.0.0.1:12345 %privatekey% %txhash% 42\n"
            "alias2 127.0.0.1:54321 %privatekey% %txhash% 24\n"
        )

        expected = {
            'alias1': {'ip': '127.0.0.1:12345', 'key': '%privatekey%', 'tx': ('%txhash%', 42)},
            'alias2': {'ip': '127.0.0.1:54321', 'key': '%privatekey%', 'tx': ('%txhash%', 24)}
        }
        actual = sandbox.parse_nodeconfig(StringIO.StringIO(config))
        self.assertEqual(expected, actual)

    def test_parse_nodeconfig_additional_spaces(self):
        config = (
            "alias1 127.0.0.1:12345 %privatekey% %txhash% 42\n"
            "alias2   127.0.0.1:54321    %privatekey%        %txhash%   24\n"
        )

        expected = {
            'alias1': {'ip': '127.0.0.1:12345', 'key': '%privatekey%', 'tx': ('%txhash%', 42)},
            'alias2': {'ip': '127.0.0.1:54321', 'key': '%privatekey%', 'tx': ('%txhash%', 24)}
        }
        actual = sandbox.parse_nodeconfig(StringIO.StringIO(config))
        self.assertEqual(expected, actual)

    def test_parse_nodeconfig_with_comments(self):
        config = (
            "alias1 127.0.0.1:12345 %privatekey% %txhash% 42\n"
            "   # alias2 127.0.0.1:54321 %privatekey% %txhash% 24\n"
        )

        expected = {
            'alias1': {'ip': '127.0.0.1:12345', 'key': '%privatekey%', 'tx': ('%txhash%', 42)}
        }
        actual = sandbox.parse_nodeconfig(StringIO.StringIO(config))
        self.assertEqual(expected, actual)

    def test_parse_nodeconfig_same_alias(self):
        config = (
            "alias1 127.0.0.1:12345 %privatekey% %txhash% 42\n"
            "alias1 127.0.0.1:54321 %privatekey% %txhash% 24\n"
        )

        expected = {
            'alias1': {'ip': '127.0.0.1:54321', 'key': '%privatekey%', 'tx': ('%txhash%', 24)}
        }
        actual = sandbox.parse_nodeconfig(StringIO.StringIO(config))
        self.assertEqual(expected, actual)

    def test_write_nodeconfig(self):
        config = {
            'alias1': {'ip': '127.0.0.1:12345', 'key': '%privatekey%', 'tx': ('%txhash%', 42)},
            'alias2': {'ip': '127.0.0.1:54321', 'key': '%privatekey%', 'tx': ('%txhash%', 24)}
        }

        expected = (
            "alias2 127.0.0.1:54321 %privatekey% %txhash% 24\n"
            "alias1 127.0.0.1:12345 %privatekey% %txhash% 42\n"
        )

        actual = StringIO.StringIO()
        sandbox.write_nodeconfig(actual, config)

        self.assertEqual(expected, actual.getvalue())


class TestJson(unittest.TestCase):

    def test_acceptance(self):
        actual = '{\"devnet\":{\"name\": \"\"},\"nodes\": {\"seed\": {},\"miner\": {}}}'
        expected = {
            'devnet': {'name': ''},
            'nodes': {'seed': {}, 'miner': {}}
        }
        self.assertEqual(sandbox.parse_json(StringIO.StringIO(actual)), expected)

    def test_two_nodes(self):
        json = '{\"devnet\":{\"name\": \"\"},\"nodes\": {\"seed\": {},\"miner\": {}}}'
        network = sandbox.parse_json(StringIO.StringIO(json))

        self.assertEqual(network['nodes'], {'seed': {}, 'miner': {}})


class TestConfig(unittest.TestCase):

    def test_write_port(self):
        f = StringIO.StringIO()

        config = sandbox.Config(f)
        config.set_option('port', 42001)
        config.dump()

        self.assertEqual(f.getvalue(), 'port=42001\n')

    def test_write_port_twice(self):
        f = StringIO.StringIO('port=42001')

        config = sandbox.Config(f)
        config.set_option('port', 42002)
        config.dump()

        self.assertEqual(f.getvalue(), 'port=42002\n')

    def test_write_add_two_nodes(self):
        f = StringIO.StringIO('addnode=127.0.0.1')

        config = sandbox.Config(f)
        config.set_option('addnode', '127.0.0.2')
        config.dump()

        self.assertEqual(set(f.getvalue().splitlines()), {'addnode=127.0.0.1', 'addnode=127.0.0.2'})

    def test_write_add_node_twice(self):
        f = StringIO.StringIO('addnode=127.0.0.1')

        config = sandbox.Config(f)
        config.set_option('addnode', '127.0.0.1')
        config.dump()

        self.assertEqual(set(f.getvalue().splitlines()), {'addnode=127.0.0.1'})

    def test_write_mining_stuff(self):
        f = StringIO.StringIO()

        config = sandbox.Config(f)
        config.set_option('gen', '')
        config.set_option('genproclimit', 13)
        config.dump()

        self.assertEqual(set(f.getvalue().splitlines()), {'gen=', 'genproclimit=13'})


test_json = '''{
  "devnet":{
    "name": "o_O"
  },

  "nodes": {
    "seed": {
      "seed": true,
      "addnodes": ["miner"]
    },
    "miner": {
      "generate": true
    }
  }
}'''

masternode_json = '''{
  "devnet":{
    "name": "rose"
  },

  "nodes": {
    "seed": {
      "seed": true,
      "addnodes": ["miner"]
    },
    "miner": {
      "generate": true
    },
    "mn1": {}, "mn2": {},
    "wallet": {
      "masternodes":["mn1", "mn2"]
    }
  }
}'''


tx_json = '''{
    "txid" : "2085bdef731ade8d0b0ce79ae0397a75aacf57b5028bd34724de3ae3faf48830",
    "version" : 1,
    "locktime" : 0,
    "vin" : [
        {
            "txid" : "eb71407741a48cff14eeef2d3b9c72d68ad76edd1129e1d4ce84364c5a811d82",
            "vout" : 0,
            "scriptSig" : {
                "asm" : "3045022100b1bd89430406d6c1396bcfd63afed2e62df9d9d5c6175017caefef4ba0e44b9702200eb7ad7c79f1bd41e9607316bbb165f161aca920f0941a336359df30c1dfc66401 021051d021fe53b1a18c236dfcee7c2c341c4fab029aa80b889dd811c175c2e64f",
                "hex" : "483045022100b1bd89430406d6c1396bcfd63afed2e62df9d9d5c6175017caefef4ba0e44b9702200eb7ad7c79f1bd41e9607316bbb165f161aca920f0941a336359df30c1dfc6640121021051d021fe53b1a18c236dfcee7c2c341c4fab029aa80b889dd811c175c2e64f"
            },
            "sequence" : 4294967295
        }
    ],
    "vout" : [
        {
            "value" : 0.96000000,
            "n" : 0,
            "scriptPubKey" : {
                "asm" : "OP_DUP OP_HASH160 881c1d72c1893e6f19b6fc78917cbdcc160b45c3 OP_EQUALVERIFY OP_CHECKSIG",
                "hex" : "76a914881c1d72c1893e6f19b6fc78917cbdcc160b45c388ac",
                "reqSigs" : 1,
                "type" : "pubkeyhash",
                "addresses" : [
                    "msvdscK7JaMZPZCqhEjuP9VjHLb7Dsm8TA"
                ]
            }
        },
        {
            "value" : 4.04000000,
            "n" : 1,
            "scriptPubKey" : {
                "asm" : "OP_DUP OP_HASH160 917df68ca216a7a0b15a1fba2703c266e98dc1b2 OP_EQUALVERIFY OP_CHECKSIG",
                "hex" : "76a914917df68ca216a7a0b15a1fba2703c266e98dc1b288ac",
                "reqSigs" : 1,
                "type" : "pubkeyhash",
                "addresses" : [
                    "mtnFAwMMCqyRh8ohPmv1EDbGZa98uHDvT9"
                ]
            }
        }
    ]
}'''


class TestSandbox(unittest.TestCase):

    def test_mnnodes(self):
        mnnames = [u'ant', u'bear', u'cat', u'dog', u'eagle', u'fox', u'gorilla', u'horse', u'iguana', u'jaguar']
        all_nodes = [u'eagle', u'horse', u'jaguar', u'fox', u'hq', u'miner', u'dog', u'bear', u'cat', u'ant', u'gorilla', u'seed', u'iguana']
        intersection = [node for node in mnnames if node in all_nodes]
        self.assertEqual(intersection, mnnames)


    def test_bindir_is_none(self):
        fp = StringIO.StringIO(test_json)
        sb = sandbox.Sandbox(None, '/home/some/dir', fp)

        def mock(*args):
            mock.args = args[0]
            return '1234.321',''
        sb.check_call = mock

        balance = sb.check_balance('miner')

        self.assertEqual(
            sb.check_call.args,
            ['crown-cli', '-datadir=/home/some/dir/miner', 'getbalance']
        )
        self.assertEqual(balance, 1234.321)

    def test_seed_of_two_nodes(self):
        fp = StringIO.StringIO(test_json)
        sb = sandbox.Sandbox('', '', fp)

        config = sb.config_for_node('seed')
        expected = {
            ('port', 42001),
            ('rpcport', 43001),
            ('devnet', 'o_O'),
            ('rpcpassword', 'bogus'),
            ('addnode', '127.0.0.1:42002')
        }
        self.assertEqual(config, expected)

    def test_miner_of_two_nodes(self):
        fp = StringIO.StringIO(test_json)
        sb = sandbox.Sandbox('', '', fp)

        config = sb.config_for_node('miner')
        expected = {
            ('port', 42002),
            ('rpcport', 43002),
            ('devnet', 'o_O'),
            ('gen', ''),
            ('genproclimit', 1),
            ('rpcpassword', 'bogus'),
            ('addnode', '127.0.0.1:42001')
        }
        self.assertEqual(config, expected)

    def test_config_filename(self):
        fp = StringIO.StringIO(test_json)
        sb = sandbox.Sandbox('', '/home/xyz/test', fp)

        expected = '/home/xyz/test/seed/crown.conf'
        self.assertEqual(sb.config_filename('seed'), expected)

    def test_miners(self):
        fp = StringIO.StringIO(test_json)
        sb = sandbox.Sandbox('', '/home/xyz/test', fp)

        self.assertEqual(sb.miners(), ['miner'])

    def test_start_nodes_one(self):
        fp = StringIO.StringIO(test_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock(*args):
            if not hasattr(mock, 'args'):
                mock.args = [args[0]]
            else:
                mock.args += args[0]
            return '',''
        sb.unchecked_call = mock

        sb.start_nodes(['miner'])

        self.assertEqual(
            sb.unchecked_call.args[0],
            ['/home/binaries/crownd', '-datadir=/home/xyz/test/miner', '-daemon']
        )

    def test_start_nodes_two(self):
        fp = StringIO.StringIO(test_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock(*args):
            if not hasattr(mock, 'args'):
                mock.args = [args[0]]
            else:
                mock.args.append(args[0])
            return ''
        sb.unchecked_call = mock

        sb.start_nodes(['miner', 'seed'])

        self.assertEqual(
            sb.unchecked_call.args[0],
            ['/home/binaries/crownd', '-datadir=/home/xyz/test/miner', '-daemon']
        )
        self.assertEqual(
            sb.unchecked_call.args[1],
            ['/home/binaries/crownd', '-datadir=/home/xyz/test/seed', '-daemon']
        )

    def test_start_nodes_all(self):
        fp = StringIO.StringIO(test_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock(*args):
            if not hasattr(mock, 'args'):
                mock.args = [args[0]]
            else:
                mock.args.append(args[0])
            return ''
        sb.unchecked_call = mock

        sb.start_nodes()

        self.assertEqual(
            set([tuple(l) for l in sb.unchecked_call.args]),
            {('/home/binaries/crownd', '-datadir=/home/xyz/test/miner', '-daemon'),
             ('/home/binaries/crownd', '-datadir=/home/xyz/test/seed', '-daemon')}
        )

    def test_masternode_controllers(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('', '/home/xyz/test', fp)

        self.assertEqual(sb.masternode_controllers(), ['wallet'])

    def test_masternodes_empty(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('', '/home/xyz/test', fp)

        self.assertEqual(sb.masternodes('seed'), [])

    def test_masternodes_two(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('', '/home/xyz/test', fp)

        self.assertEqual(sb.masternodes('wallet'), ["mn1", "mn2"])

    def test_masternodes_no_such_nodes(self):
        json = '{"devnet":{"name":"_"},"nodes":{"wallet": { "masternodes":["mn1", "mn2"]}}}'
        fp = StringIO.StringIO(json)
        sb = sandbox.Sandbox('', '/home/xyz/test', fp)

        self.assertEqual(sb.masternodes('wallet'), [])

    def test_mnconfig_filename(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('', '/home/xyz/test', fp)

        expected = '/home/xyz/test/wallet/devnet-rose/masternode.conf'
        self.assertEqual(sb.mnconfig_filename('wallet'), expected)

    def test_write_mninfo_to_config(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        sb.network['nodes']['mn1']['mnprivkey'] = 'MNPRIVKEY'
        config = sb.config_for_node('mn1')
        expected = {
            ('port', sb.node_port('mn1')),
            ('rpcport', sb.node_rpc_port('mn1')),
            ('devnet', 'rose'),
            ('rpcpassword', 'bogus'),
            ('masternode', 1),
            ('masternodeprivkey', 'MNPRIVKEY'),
            ('externalip', sb.node_ip('mn1')),
            ('addnode', sb.node_ip('seed'))
        }
        self.assertEqual(config, expected)

    def test_create_key(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock(*args):
            mock.args = args[0]
            return '92ivPHv2DgiUzdvqftKrTYGTXtZNoPf9pPFGjAWWWdNC5KYAehE',''
        sb.check_call = mock

        key = sb.create_key('wallet')

        self.assertEqual(
            sb.check_call.args,
            ['/home/binaries/crown-cli', '-datadir=/home/xyz/test/wallet', 'node', 'genkey']
        )
        self.assertEqual(key, '92ivPHv2DgiUzdvqftKrTYGTXtZNoPf9pPFGjAWWWdNC5KYAehE')

    def test_send_all(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock_check_balance(node):
            self.assertEqual(node, 'miner')
            return 1234
        sb.check_balance = mock_check_balance

        def mock_gen_address(node):
            self.assertEqual(node, 'wallet')
            return 'b0gUS'
        sb.gen_address = mock_gen_address

        def mock_send(node, addr, amount):
            self.assertEqual(node, 'miner')
            self.assertEqual(addr, 'b0gUS')
            self.assertEqual(amount, 1234)
            mock_send.ok = True
        sb.send = mock_send
        sb.send.ok = False

        def mock_client_command(*args):
            pass
        sb.client_command = mock_client_command

        sb.send_all('miner', 'wallet')

        self.assertTrue(sb.send.ok)

    def test_get_next_masternode(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        next = sb.next_masternode_to_setup()

        self.assertEqual(next, ('mn1','wallet'))

    def test_get_next_masternode_second(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)
        sb.network['nodes']['wallet']['mnconfig'] = {
            'alias1': {'ip': sb.node_ip('mn1'), 'key': '%privatekey%', 'tx': ('%txhash%', 24)}
        }

        next = sb.next_masternode_to_setup()

        self.assertEqual(next, ('mn2','wallet'))

    def test_get_next_masternode_no_more(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)
        sb.network['nodes']['wallet']['mnconfig'] = {
            'alias1': {'ip': sb.node_ip('mn1'), 'key': '%privatekey%', 'tx': ('%txhash%', 24)},
            'whatever': {'ip': sb.node_ip('mn2'), 'key': '%privatekey%', 'tx': ('%txhash%', 24)}
        }

        next = sb.next_masternode_to_setup()

        self.assertEqual(next, (None, None))

    def test_gen_or_reuse_address(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock(*args):
            if not hasattr(mock, 'gen_address_args'):
                mock.gen_address_args = args[0]
                return 'mtnFAwMMCqyRh8ohPmv1EDbGZa98uHDvT9',''
            else:
                mock.get_address_args = []
                self.fail('Second call to check_call')
        sb.check_call = mock

        addr1 = sb.gen_or_reuse_address('miner')
        addr2 = sb.gen_or_reuse_address('miner')

        self.assertEqual(sb.network['nodes']['miner']['receive_with'], 'mtnFAwMMCqyRh8ohPmv1EDbGZa98uHDvT9')

        self.assertEqual(
            sb.check_call.gen_address_args,
            ['/home/binaries/crown-cli', '-datadir=/home/xyz/test/miner', 'getnewaddress']
        )
        self.assertEqual(addr1, 'mtnFAwMMCqyRh8ohPmv1EDbGZa98uHDvT9')
        self.assertEqual(addr2, 'mtnFAwMMCqyRh8ohPmv1EDbGZa98uHDvT9')

    def test_send_to(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock(*args):
            if not hasattr(mock, 'gen_address_args'):
                mock.gen_address_args = args[0]
                return 'mtnFAwMMCqyRh8ohPmv1EDbGZa98uHDvT9',''
            else:
                mock.send_args = args[0]
                return '2085bdef731ade8d0b0ce79ae0397a75aacf57b5028bd34724de3ae3faf48830',''
        sb.check_call = mock

        addr = sb.gen_address('miner')
        txhash = sb.send('wallet', addr, 4.04)

        self.assertEqual(
            sb.check_call.gen_address_args,
            ['/home/binaries/crown-cli', '-datadir=/home/xyz/test/miner', 'getnewaddress']
        )
        self.assertEqual(
            sb.check_call.send_args,
            ['/home/binaries/crown-cli', '-datadir=/home/xyz/test/wallet', 'sendtoaddress', 'mtnFAwMMCqyRh8ohPmv1EDbGZa98uHDvT9', '4.04']
        )
        self.assertEqual(txhash, '2085bdef731ade8d0b0ce79ae0397a75aacf57b5028bd34724de3ae3faf48830')

    def test_get_tx(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock(*args):
            if not hasattr(mock, 'getrawtx_args'):
                mock.getrawtx_args = args[0]
                return 'BAADC0DE',''
            else:
                mock.decoderawtx_args = args[0]
                return tx_json,''
        sb.check_call = mock

        tx = sb.get_transaction('wallet', '2085bdef731ade8d0b0ce79ae0397a75aacf57b5028bd34724de3ae3faf48830')

        self.assertEqual(
            sb.check_call.getrawtx_args,
            ['/home/binaries/crown-cli', '-datadir=/home/xyz/test/wallet', 'getrawtransaction', '2085bdef731ade8d0b0ce79ae0397a75aacf57b5028bd34724de3ae3faf48830']
        )
        self.assertEqual(
            sb.check_call.decoderawtx_args,
            ['/home/binaries/crown-cli', '-datadir=/home/xyz/test/wallet', 'decoderawtransaction', 'BAADC0DE']
        )
        self.assertEqual(tx['vout'][1]['value'], 4.04)

    def test_tx_find_output(self):
        tx = json.loads(tx_json)
        self.assertEqual(sandbox.tx_find_output(tx, 4.04), 1)

    def test_check_balance(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)

        def mock(*args):
            mock.args = args[0]
            return '1234.321',''
        sb.check_call = mock

        balance = sb.check_balance('wallet')

        self.assertEqual(
            sb.check_call.args,
            ['/home/binaries/crown-cli', '-datadir=/home/xyz/test/wallet', 'getbalance']
        )
        self.assertEqual(balance, 1234.321)

    def test_setup_masternode(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)
        self.assertEqual(sb.next_masternode_to_setup(), ('mn1','wallet'))

        def mock_create_key(*args):
            return '%privatekey%'
        sb.create_key = mock_create_key

        def mock_send(*args):
            return '%txhash%'
        sb.send = mock_send

        def mock_gen_address(*args):
            return 'addr'
        sb.gen_address = mock_gen_address

        def mock_get_transaction(*args):
            return {'txid': '%txhash%', 'vout':[{'value':42,'n':0},{'value':10000,'n':1}]}
        sb.get_transaction = mock_get_transaction

        def mock_stop_node(*args):
            pass
        sb.stop_node = mock_stop_node

        def mock_write_config(*args):
            pass
        sb.write_config = mock_write_config

        def mock_write_mnconfig(*args):
            pass
        sb.write_mnconfig = mock_write_mnconfig

        def mock_start_node(*args):
            pass
        sb.start_node = mock_start_node

        def mock_start_masternode(*args):
            pass
        sb.start_masternode = mock_start_masternode

        mn1_config = dict(sb.network['nodes']['mn1'])
        mn1_config['mnprivkey'] = '%privatekey%'

        sb.setup_masternode('wallet', 'mn1')

        self.assertEqual(sb.network['nodes']['mn1'], mn1_config)

        self.assertEqual(sb.next_masternode_to_setup(), ('mn2','wallet'))
        self.assertEqual(
            sb.network['nodes']['wallet']['mnconfig'],
            {'mn1': {'ip': sb.node_ip('mn1'), 'key': '%privatekey%', 'tx': ('%txhash%', 1)}}
        )

    def test_setup_masternode2(self):
        fp = StringIO.StringIO(masternode_json)
        sb = sandbox.Sandbox('/home/binaries', '/home/xyz/test', fp)
        sb.network['nodes']['wallet']['mnconfig'] = {
            'mn1': {'ip': sb.node_ip('mn1'), 'key': '%privatekey%', 'tx': ('%txhash%', 24)}
        }
        self.assertEqual(sb.next_masternode_to_setup(), ('mn2','wallet'))

        def mock_create_key(*args):
            return '%privatekey%'
        sb.create_key = mock_create_key

        def mock_send(*args):
            return '%txhash%'
        sb.send = mock_send

        def mock_gen_address(*args):
            return 'addr'
        sb.gen_address = mock_gen_address

        def mock_get_transaction(*args):
            return {'txid': '%txhash%', 'vout':[{'value':42,'n':0},{'value':10000,'n':1}]}
        sb.get_transaction = mock_get_transaction

        def mock_stop_node(*args):
            pass
        sb.stop_node = mock_stop_node

        def mock_write_config(*args):
            pass
        sb.write_config = mock_write_config

        def mock_write_mnconfig(*args):
            pass
        sb.write_mnconfig = mock_write_mnconfig

        def mock_start_node(*args):
            pass
        sb.start_node = mock_start_node

        def mock_start_masternode(*args):
            pass
        sb.start_masternode = mock_start_masternode

        sb.setup_masternode('wallet', 'mn2')

        self.assertEqual(
            sb.network['nodes']['wallet']['mnconfig'],
            {
                'mn1': {'ip': sb.node_ip('mn1'), 'key': '%privatekey%', 'tx': ('%txhash%', 24)},
                'mn2': {'ip': sb.node_ip('mn2'), 'key': '%privatekey%', 'tx': ('%txhash%', 1)}
            }
        )
        self.assertEqual(sb.next_masternode_to_setup(), (None, None))

if __name__ == '__main__':
    unittest.main()
