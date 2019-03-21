#!/usr/bin/env python2

import ConfigParser
import argparse
import json
import os
import shutil
import subprocess
import time

import math


def create_dir(dirname):
    dirname = os.path.expanduser(dirname)
    if not os.path.exists(dirname):
        os.makedirs(dirname)


def delete_dir(dirname):
    dirname = os.path.expanduser(dirname)
    if os.path.exists(dirname):
        shutil.rmtree(dirname)


def parse_json(fp):
    return json.load(fp)


def parse_nodeconfig(fp):
    config = {}
    for line in fp.readlines():
        lex = [l for l in line.split(' ') if len(l) > 0]
        if lex[0][0] == '#':
            continue

        if len(lex) < 5:
            continue

        alias, ip, key = tuple(lex[0:3])
        tx = (lex[3], int(lex[4]))
        config[alias] = {'ip': ip, 'key': key, 'tx': tx}

    return config


def write_nodeconfig(fp, nodeconfig):
    def line(als, n):
        return ' '.join([als, n['ip'], n['key'], n['tx'][0], str(n['tx'][1])]) + '\n'

    fp.writelines(
        [line(alias, node) for alias, node in nodeconfig.iteritems()]
    )


def tx_find_output(tx, amount):
    vouts = tx['vout']
    for i in range(0, len(vouts)):
        vout = vouts[i]
        if vout['value'] == amount:
            return vout['n']


class Config:
    class _FakeSecHead(object):
        def __init__(self, fp):
            self.fp = fp
            self.sechead = '[_dummy]\n'

        def readline(self):
            if self.sechead:
                try:
                    return self.sechead
                finally:
                    self.sechead = None
            else:
                return self.fp.readline()

    def __init__(self, fp):
        self.options = {}
        self.fp = None
        self.open(fp)

    def set_option(self, key, value):
        if key in self.multioptions():
            if key in self.options:
                self.options[key].add(value)
            else:
                self.options[key] = {value}
        else:
            self.options[key] = value

    def get_option(self, key):
        if key not in self.options:
            return None
        return self.options[key]

    def dump(self):
        self.fp.seek(0)
        for key, value in self.options.iteritems():
            if key in self.multioptions():
                for v in value:
                    self.fp.writelines('{0}={1}\n'.format(key, v))
            else:
                self.fp.writelines('{0}={1}\n'.format(key, value))
        self.reset()

    def reset(self):
        self.options = {}
        self.fp = None

    def open(self, fp):
        config = ConfigParser.ConfigParser()
        config.readfp(self._FakeSecHead(fp))
        for key in config.options('_dummy'):
            value = config.get('_dummy', key)
            self.set_option(key, value)
        self.fp = fp

    @staticmethod
    def multioptions():
        return ['addnode']


def set_default(dic, key, default):
    if key not in dic:
        dic[key] = default


class Sandbox:
    def __init__(self, bindir, targetdir, config_file, port_base=42000):
        self.targetdir = os.path.expanduser(targetdir)
        self.bindir = '' if bindir is None else os.path.expanduser(bindir)
        self.network = parse_json(config_file)
        self.port_base = port_base

        port = self.port_base
        for name, node in self.network['nodes'].iteritems():
            set_default(node, 'generate', False)
            set_default(node, 'addnodes', [])
            set_default(node, 'masternodes', [])
            set_default(node, 'mnconfig', {})

            node['masternodes'] = [{"node": mn} for mn in node['masternodes']]

            port += 1
            node['port'] = port

            self.read_mnconfig(name)

    def read_mnconfig(self, node):
        filename = self.mnconfig_filename(node)
        if not os.path.exists(filename):
            return
        config = parse_nodeconfig(open(filename))
        self.network['nodes'][node]['mnconfig'] = config

    def write_mnconfig(self, node):
        filename = self.mnconfig_filename(node)
        write_nodeconfig(open(filename, "w+"), self.network['nodes'][node]['mnconfig'])

    def mnconfig_filename(self, node):
        filename = 'devnet-{0}/masternode.conf'.format(self.network['devnet']['name'])
        return os.path.join(self.data_directory(node), filename)

    def prepare(self):
        delete_dir(self.targetdir)
        create_dir(self.targetdir)

        for name, node in self.network['nodes'].iteritems():
            create_dir(os.path.join(self.targetdir, name))
            self.write_config(name)

    def start_nodes(self, nodes=None, binary='crownd'):
        if nodes is None:
            nodes = []
        nodes = nodes if nodes != [] else self.network['nodes'].iterkeys()
        for name in nodes:
            self.start_node(name, binary)

    def start_node(self, node, binary='crownd'):
        execute = os.path.join(self.bindir, binary)
        return self.unchecked_call(
            [execute, '-datadir=' + self.data_directory(node), '-daemon']
        )

    @staticmethod
    def check_call(arguments):
        p = subprocess.Popen(arguments, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p.communicate()
        retcode = p.returncode
        if retcode == 1:
            cmd = arguments[0]
            raise subprocess.CalledProcessError(retcode, cmd, err)
        return out, err

    @staticmethod
    def unchecked_call(*args):
        return subprocess.call(*args)

    def write_config(self, node):
        config = Config(open(self.config_filename(node), "w+"))
        for key, value in self.config_for_node(node):
            config.set_option(key, value)
        config.dump()

    def config_for_node(self, name):
        config = set()

        node = self.network['nodes'][name]

        if 'mnprivkey' in node:
            config.add(('masternode', 1))
            config.add(('masternodeprivkey', node['mnprivkey']))
            config.add(('externalip',self.node_ip(name)))

        config.add(('port', self.node_port(name)))
        config.add(('rpcport', self.node_rpc_port(name)))
        config.add(('devnet', self.network['devnet']['name']))

        if node['generate']:
            config.add(('gen', ''))
            config.add(('genproclimit', 1))

        for peer in node['addnodes']:
            config.add(('addnode', self.node_ip(peer)))

        for seed_name in self.network['nodes']:
            if seed_name != name:
                config.add(('addnode', self.node_ip(seed_name)))

        config.add(('rpcpassword', 'bogus'))

        return config

    def node_port(self, name):
        return self.network['nodes'][name]['port']

    def node_ip(self, peer):
        return '127.0.0.1:' + str(self.node_port(peer))

    def config_filename(self, node):
        return os.path.join(self.data_directory(node), 'crown.conf')

    def data_directory(self, node):
        if node not in self.network['nodes']:
            raise RuntimeError("No such node: {0}".format(node))

        return os.path.join(self.targetdir, node)

    def node_rpc_port(self, name):
        return self.node_port(name) + 1000

    def masternodes(self, node):
        all_nodes = [x for x in self.network['nodes'].iterkeys()]
        mnlist = self.network['nodes'][node]['masternodes']
        mnnames = [mn['node'] for mn in mnlist]

        return [node for node in mnnames if node in all_nodes]

    def check_balance(self, node, binary='crown-cli'):
        out, err = self.client_command(node, binary, 'getbalance')
        if len(err) != 0:
            return 0

        return float(out)

    def gen_address(self, node, binary='crown-cli'):
        out, err = self.client_command(node, binary, 'getnewaddress')
        assert (len(err) == 0)

        return out

    def client_command(self, node, binary, *args):
        execute = os.path.join(self.bindir, binary)
        try:
            return self.check_call(
                [execute, '-datadir=' + self.data_directory(node)] + list(args)
            )
        except:
            print node, ": ", list(args)
            raise

    def send_all(self, node_from, node_to):
        balance = self.check_balance(node_from)
        if balance > 0:
            self.client_command(node_from, 'crown-cli', 'settxfee', str(0))
            self.send(node_from, self.gen_address(node_to), balance)

    def send(self, node, addr, amount, binary='crown-cli'):
        out, err = self.client_command(node, binary, 'sendtoaddress', addr, str(amount))
        if len(err) != 0:
            print err
        assert (len(err) == 0)
        return out.strip()

    def get_transaction(self, node, txhash, binary='crown-cli'):
        rtx, err = self.client_command(node, binary, 'getrawtransaction', txhash)
        if len(err) != 0:
            print err
        assert (len(err) == 0)
        dtx, err = self.client_command(node, binary, 'decoderawtransaction', rtx.strip())
        if len(err) != 0:
            print err
            print rtx
        assert (len(err) == 0)
        return json.loads(dtx)

    def create_key(self, node, binary='crown-cli'):
        out, err = self.client_command(node, binary, 'node', 'genkey')
        assert (len(err) == 0)
        return out.strip()

    def next_masternode_to_setup(self):
        def is_setup(masternode, name):
            if 'mnconfig' not in self.network['nodes'][name]:
                return False

            for alias, nodecfg in self.network['nodes'][name]['mnconfig'].iteritems():
                if nodecfg['ip'] == self.node_ip(masternode):
                    return True
            return False

        mnodes = [(mn, controller)
                  for controller in self.masternode_controllers()
                  for mn in self.masternodes(controller)
                  if not is_setup(mn, controller)
                  ]
        if mnodes:
            return mnodes[0]
        else:
            return None, None

    def setup_masternode(self, control_node, masternode):
        prvkey = self.create_key(control_node)
        txhash = self.send(control_node, self.gen_address(control_node), 10000)
        colltx = self.get_transaction(control_node, txhash)
        claddr = tx_find_output(colltx, 10000)

        self.network['nodes'][control_node]['mnconfig'][masternode] = {
            'ip': self.node_ip(masternode),
            'key': prvkey,
            'tx': (txhash, claddr)
        }
        self.network['nodes'][masternode]['mnprivkey'] = prvkey

        self.stop_node(control_node)
        time.sleep(1)
        self.write_mnconfig(control_node)
        self.start_node(control_node)

        self.stop_node(masternode)
        time.sleep(1)
        self.write_config(masternode)
        self.start_node(masternode)

        time.sleep(1)
        self.start_masternode(control_node, masternode)

    def stop_node(self, node):
        return self.client_command(node, 'crown-cli', 'stop')

    def miners(self):
        return [k for k, v in self.network['nodes'].iteritems() if v['generate']]

    def masternode_controllers(self):
        return [k for k, v in self.network['nodes'].iteritems() if v['masternodes'] != []]

    def start_masternode(self, control_node, masternode):
        self.client_command(control_node, 'crown-cli', 'masternode', 'start-alias', masternode)

    def gen_or_reuse_address(self, node):
        if 'receive_with' in self.network['nodes'][node]:
            return self.network['nodes'][node]['receive_with']

        self.network['nodes'][node]['receive_with'] = self.gen_address(node)
        return self.network['nodes'][node]['receive_with']


def prepare(json_desc_file, target_dir, bin_dir):
    sb = Sandbox(bin_dir, target_dir, open(json_desc_file))
    sb.prepare()


def start(json_desc_file, target_dir, bin_dir, nodes):
    sb = Sandbox(bin_dir, target_dir, open(json_desc_file))
    sb.start_nodes(nodes)


def command(json_desc_file, target_dir, bin_dir, node, *args):
    sb = Sandbox(bin_dir, target_dir, open(json_desc_file))
    if node == '_':
        for node in sb.network['nodes']:
            try:
                sb.client_command(node, 'crown-cli', *args)
            except subprocess.CalledProcessError as ex:
                print ex.output
                print 'error running on ', node, ': ', ex
            except Exception as ex:
                print 'error running on ', node, ': ', ex
    else:
        out, err = sb.client_command(node, 'crown-cli', *args)
        print out
        if len(err.strip()) > 0:
            print err


def run_simulation(json_desc_file, target_dir, bin_dir):
    sb = Sandbox(bin_dir, target_dir, open(json_desc_file))

    while True:
        try:
            for node in sb.miners():
                if 'send_to' not in sb.network['nodes'][node]:
                    continue
                balance = sb.check_balance(node)
                if balance > 1000:
                    # Didn't find a good way to send all the money AND include tx fee
                    # This code will send almost all the money almost always successfully

                    # Also, it's important to send in big chunks since it will keep the collateral
                    # transaction reasonably small (and huge tx sizes are a problem since the
                    # encoded tx won't fit into pipe)

                    to = sb.network['nodes'][node]['send_to']
                    amount = int(math.floor(balance / 1000.0)) * 1000
                    print 'Sending {0} to {1}'.format(amount, to)

                    try:
                        sb.send(node, sb.gen_or_reuse_address(to), amount)
                        time.sleep(1)
                        print '{0} balance: {1}'.format(to, sb.check_balance(to))

                    except subprocess.CalledProcessError as ex:
                        print ex.output

            mn, node = sb.next_masternode_to_setup()
            if mn is not None:
                balance = sb.check_balance(node)
                if balance < 10001:
                    continue

                print 'Setting up masternode {0}'.format(mn)
                sb.setup_masternode(node, mn)
                print 'Next masternode is {0}'.format(sb.next_masternode_to_setup()[0])

        except RuntimeError:
            pass

        time.sleep(2)


if __name__ == '__main__':
    try:
        ap = argparse.ArgumentParser(description='Crown Sandbox control script')
        ap.add_argument(
            'command',
            nargs='+',
            action='store',
            help='Command to execute: ( prepare | start [NODES...] | cmd NODE [ARGUMENTS...] | sim)'
        )
        ap.add_argument('-b', '--bin-directory', action='store', help='Binaries custom directory')
        ap.add_argument('-d', '--directory', action='store', help='Root data directory for all nodes in the sandbox',
                        required=True)
        ap.add_argument('-f', '--file', action='store', help='Sandbox description file (json format)', required=True)
        parsed = ap.parse_args()

        if parsed.command[0] == 'prepare':
            prepare(parsed.file, parsed.directory, parsed.bin_directory)
        elif parsed.command[0] == 'start':
            start(parsed.file, parsed.directory, parsed.bin_directory, parsed.command[1:])
        elif parsed.command[0] == 'cmd':
            command(parsed.file, parsed.directory, parsed.bin_directory, parsed.command[1], *parsed.command[2:])
        elif parsed.command[0] == 'sim':
            run_simulation(parsed.file, parsed.directory, parsed.bin_directory)
        else:
            ap.print_help()
    except subprocess.CalledProcessError as ex:
        print ex.output
        print ex
    except RuntimeError as ex:
        print ex
