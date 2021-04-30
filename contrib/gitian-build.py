#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import argparse
import os
import subprocess
import sys

def setup():
    global args, workdir
    programs = ['ruby', 'git', 'make', 'wget', 'curl']
    if args.kvm:
        programs += ['apt-cacher-ng', 'python-vm-builder', 'qemu-kvm', 'qemu-utils']
    elif args.docker:
        if not os.path.isfile('/lib/systemd/system/docker.service'):
            dockers = ['docker.io', 'docker-ce']
            for i in dockers:
                return_code = subprocess.call(['sudo', 'apt-get', 'install', '-qq', i])
                if return_code == 0:
                    break
            if return_code != 0:
                print('Cannot find any way to install Docker.', file=sys.stderr)
                sys.exit(1)
    else:
        programs += ['apt-cacher-ng', 'lxc', 'debootstrap']
    subprocess.check_call(['sudo', 'apt-get', 'install', '-qq'] + programs)
    if not os.path.isdir('gitian.sigs'):
        subprocess.check_call(['git', 'clone', 'https://github.com/syscoin-core/gitian.sigs.git'])
    if not os.path.isdir('syscoin-detached-sigs'):
        subprocess.check_call(['git', 'clone', 'https://github.com/syscoin-core/syscoin-detached-sigs.git'])
    if not os.path.isdir('gitian-builder'):
        subprocess.check_call(['git', 'clone', 'https://github.com/devrandom/gitian-builder.git'])
    if not os.path.isdir('syscoin'):
        subprocess.check_call(['git', 'clone', 'https://github.com/syscoin/syscoin.git'])
    os.chdir('gitian-builder')
    make_image_prog = ['bin/make-base-vm', '--suite', 'focal', '--arch', 'amd64']
    # SYSCOIN bump disksize to 27gb
    if args.docker:
        make_image_prog += ['--docker']
    elif not args.kvm:
        make_image_prog += ['--lxc', '--disksize', '27000']
    subprocess.check_call(make_image_prog)
    os.chdir(workdir)
    if args.is_focal and not args.kvm and not args.docker:
        subprocess.check_call(['sudo', 'sed', '-i', 's/lxcbr0/br0/', '/etc/default/lxc-net'])
        print('Reboot is required')
        sys.exit(0)

def build():
    global args, workdir

    os.makedirs('syscoin-binaries/' + args.version, exist_ok=True)
    print('\nBuilding Dependencies\n')
    os.chdir('gitian-builder')
    os.makedirs('inputs', exist_ok=True)

    subprocess.check_call(['wget', '-O', 'inputs/osslsigncode-2.0.tar.gz', 'https://github.com/mtrojnar/osslsigncode/archive/2.0.tar.gz'])
    subprocess.check_call(["echo '5a60e0a4b3e0b4d655317b2f12a810211c50242138322b16e7e01c6fbb89d92f inputs/osslsigncode-2.0.tar.gz' | sha256sum -c"], shell=True)
    subprocess.check_call(['make', '-C', '../syscoin/depends', 'download', 'SOURCES_PATH=' + os.getcwd() + '/cache/common'])

    if args.linux:
        print('\nCompiling ' + args.version + ' Linux')
        subprocess.check_call(['bin/gbuild', '-j', args.jobs, '-m', args.memory, '--commit', 'syscoin='+args.commit, '--url', 'syscoin='+args.url, '../syscoin/contrib/gitian-descriptors/gitian-linux.yml'])
        subprocess.check_call(['bin/gsign', '-p', args.sign_prog, '--signer', args.signer, '--release', args.version+'-linux', '--destination', '../gitian.sigs/', '../syscoin/contrib/gitian-descriptors/gitian-linux.yml'])
        subprocess.check_call('mv build/out/syscoin-*.tar.gz build/out/src/syscoin-*.tar.gz ../syscoin-binaries/'+args.version, shell=True)

    if args.windows:
        print('\nCompiling ' + args.version + ' Windows')
        subprocess.check_call(['bin/gbuild', '-j', args.jobs, '-m', args.memory, '--commit', 'syscoin='+args.commit, '--url', 'syscoin='+args.url, '../syscoin/contrib/gitian-descriptors/gitian-win.yml'])
        subprocess.check_call(['bin/gsign', '-p', args.sign_prog, '--signer', args.signer, '--release', args.version+'-win-unsigned', '--destination', '../gitian.sigs/', '../syscoin/contrib/gitian-descriptors/gitian-win.yml'])
        subprocess.check_call('mv build/out/syscoin-*-win-unsigned.tar.gz inputs/', shell=True)
        subprocess.check_call('mv build/out/syscoin-*.zip build/out/syscoin-*.exe build/out/src/syscoin-*.tar.gz ../syscoin-binaries/'+args.version, shell=True)

    if args.macos:
        print('\nCompiling ' + args.version + ' MacOS')
        subprocess.check_call(['bin/gbuild', '-j', args.jobs, '-m', args.memory, '--commit', 'syscoin='+args.commit, '--url', 'syscoin='+args.url, '../syscoin/contrib/gitian-descriptors/gitian-osx.yml'])
        subprocess.check_call(['bin/gsign', '-p', args.sign_prog, '--signer', args.signer, '--release', args.version+'-osx-unsigned', '--destination', '../gitian.sigs/', '../syscoin/contrib/gitian-descriptors/gitian-osx.yml'])
        subprocess.check_call('mv build/out/syscoin-*-osx-unsigned.tar.gz inputs/', shell=True)
        subprocess.check_call('mv build/out/syscoin-*.tar.gz build/out/syscoin-*.dmg build/out/src/syscoin-*.tar.gz ../syscoin-binaries/'+args.version, shell=True)

    os.chdir(workdir)

    if args.commit_files:
        print('\nCommitting '+args.version+' Unsigned Sigs\n')
        os.chdir('gitian.sigs')
        subprocess.check_call(['git', 'add', args.version+'-linux/'+args.signer])
        subprocess.check_call(['git', 'add', args.version+'-win-unsigned/'+args.signer])
        subprocess.check_call(['git', 'add', args.version+'-osx-unsigned/'+args.signer])
        subprocess.check_call(['git', 'commit', '-m', 'Add '+args.version+' unsigned sigs for '+args.signer])
        os.chdir(workdir)

def sign():
    global args, workdir
    os.chdir('gitian-builder')

    if args.windows:
        print('\nSigning ' + args.version + ' Windows')
        subprocess.check_call('cp inputs/syscoin-' + args.version + '-win-unsigned.tar.gz inputs/syscoin-win-unsigned.tar.gz', shell=True)
        subprocess.check_call(['bin/gbuild', '--skip-image', '--upgrade', '--commit', 'signature='+args.commit, '../syscoin/contrib/gitian-descriptors/gitian-win-signer.yml'])
        subprocess.check_call(['bin/gsign', '-p', args.sign_prog, '--signer', args.signer, '--release', args.version+'-win-signed', '--destination', '../gitian.sigs/', '../syscoin/contrib/gitian-descriptors/gitian-win-signer.yml'])
        subprocess.check_call('mv build/out/syscoin-*win64-setup.exe ../syscoin-binaries/'+args.version, shell=True)

    if args.macos:
        print('\nSigning ' + args.version + ' MacOS')
        subprocess.check_call('cp inputs/syscoin-' + args.version + '-osx-unsigned.tar.gz inputs/syscoin-osx-unsigned.tar.gz', shell=True)
        subprocess.check_call(['bin/gbuild', '--skip-image', '--upgrade', '--commit', 'signature='+args.commit, '../syscoin/contrib/gitian-descriptors/gitian-osx-signer.yml'])
        subprocess.check_call(['bin/gsign', '-p', args.sign_prog, '--signer', args.signer, '--release', args.version+'-osx-signed', '--destination', '../gitian.sigs/', '../syscoin/contrib/gitian-descriptors/gitian-osx-signer.yml'])
        subprocess.check_call('mv build/out/syscoin-osx-signed.dmg ../syscoin-binaries/'+args.version+'/syscoin-'+args.version+'-osx.dmg', shell=True)

    os.chdir(workdir)

    if args.commit_files:
        print('\nCommitting '+args.version+' Signed Sigs\n')
        os.chdir('gitian.sigs')
        subprocess.check_call(['git', 'add', args.version+'-win-signed/'+args.signer])
        subprocess.check_call(['git', 'add', args.version+'-osx-signed/'+args.signer])
        subprocess.check_call(['git', 'commit', '-a', '-m', 'Add '+args.version+' signed binary sigs for '+args.signer])
        os.chdir(workdir)

def verify():
    global args, workdir
    rc = 0
    os.chdir('gitian-builder')

    print('\nVerifying v'+args.version+' Linux\n')
    if subprocess.call(['bin/gverify', '-v', '-d', '../gitian.sigs/', '-r', args.version+'-linux', '../syscoin/contrib/gitian-descriptors/gitian-linux.yml']):
        print('Verifying v'+args.version+' Linux FAILED\n')
        rc = 1

    print('\nVerifying v'+args.version+' Windows\n')
    if subprocess.call(['bin/gverify', '-v', '-d', '../gitian.sigs/', '-r', args.version+'-win-unsigned', '../syscoin/contrib/gitian-descriptors/gitian-win.yml']):
        print('Verifying v'+args.version+' Windows FAILED\n')
        rc = 1

    print('\nVerifying v'+args.version+' MacOS\n')
    if subprocess.call(['bin/gverify', '-v', '-d', '../gitian.sigs/', '-r', args.version+'-osx-unsigned', '../syscoin/contrib/gitian-descriptors/gitian-osx.yml']):
        print('Verifying v'+args.version+' MacOS FAILED\n')
        rc = 1

    print('\nVerifying v'+args.version+' Signed Windows\n')
    if subprocess.call(['bin/gverify', '-v', '-d', '../gitian.sigs/', '-r', args.version+'-win-signed', '../syscoin/contrib/gitian-descriptors/gitian-win-signer.yml']):
        print('Verifying v'+args.version+' Signed Windows FAILED\n')
        rc = 1

    print('\nVerifying v'+args.version+' Signed MacOS\n')
    if subprocess.call(['bin/gverify', '-v', '-d', '../gitian.sigs/', '-r', args.version+'-osx-signed', '../syscoin/contrib/gitian-descriptors/gitian-osx-signer.yml']):
        print('Verifying v'+args.version+' Signed MacOS FAILED\n')
        rc = 1

    os.chdir(workdir)
    return rc

def main():
    global args, workdir

    parser = argparse.ArgumentParser(description='Script for running full Gitian builds.')
    parser.add_argument('-c', '--commit', action='store_true', dest='commit', help='Indicate that the version argument is for a commit or branch')
    parser.add_argument('-p', '--pull', action='store_true', dest='pull', help='Indicate that the version argument is the number of a github repository pull request')
    parser.add_argument('-u', '--url', dest='url', default='https://github.com/syscoin/syscoin', help='Specify the URL of the repository. Default is %(default)s')
    parser.add_argument('-v', '--verify', action='store_true', dest='verify', help='Verify the Gitian build')
    parser.add_argument('-b', '--build', action='store_true', dest='build', help='Do a Gitian build')
    parser.add_argument('-s', '--sign', action='store_true', dest='sign', help='Make signed binaries for Windows and MacOS')
    parser.add_argument('-B', '--buildsign', action='store_true', dest='buildsign', help='Build both signed and unsigned binaries')
    parser.add_argument('-o', '--os', dest='os', default='lwm', help='Specify which Operating Systems the build is for. Default is %(default)s. l for Linux, w for Windows, m for MacOS')
    parser.add_argument('-j', '--jobs', dest='jobs', default='2', help='Number of processes to use. Default %(default)s')
    parser.add_argument('-m', '--memory', dest='memory', default='2000', help='Memory to allocate in MiB. Default %(default)s')
    parser.add_argument('-k', '--kvm', action='store_true', dest='kvm', help='Use KVM instead of LXC')
    parser.add_argument('-d', '--docker', action='store_true', dest='docker', help='Use Docker instead of LXC')
    parser.add_argument('-S', '--setup', action='store_true', dest='setup', help='Set up the Gitian building environment. Only works on Debian-based systems (Ubuntu, Debian)')
    parser.add_argument('-D', '--detach-sign', action='store_true', dest='detach_sign', help='Create the assert file for detached signing. Will not commit anything.')
    parser.add_argument('-n', '--no-commit', action='store_false', dest='commit_files', help='Do not commit anything to git')
    parser.add_argument('signer', nargs='?', help='GPG signer to sign each build assert file')
    parser.add_argument('version', nargs='?', help='Version number, commit, or branch to build. If building a commit or branch, the -c option must be specified')

    args = parser.parse_args()
    workdir = os.getcwd()

    args.is_focal = b'focal' in subprocess.check_output(['lsb_release', '-cs'])

    if args.kvm and args.docker:
        raise Exception('Error: cannot have both kvm and docker')

    # Ensure no more than one environment variable for gitian-builder (USE_LXC, USE_VBOX, USE_DOCKER) is set as they
    # can interfere (e.g., USE_LXC being set shadows USE_DOCKER; for details see gitian-builder/libexec/make-clean-vm).
    os.environ['USE_LXC'] = ''
    os.environ['USE_VBOX'] = ''
    os.environ['USE_DOCKER'] = ''
    if args.docker:
        os.environ['USE_DOCKER'] = '1'
    elif not args.kvm:
        os.environ['USE_LXC'] = '1'
        if 'GITIAN_HOST_IP' not in os.environ.keys():
            os.environ['GITIAN_HOST_IP'] = '10.0.3.1'
        if 'LXC_GUEST_IP' not in os.environ.keys():
            os.environ['LXC_GUEST_IP'] = '10.0.3.5'

    if args.setup:
        setup()

    if args.buildsign:
        args.build = True
        args.sign = True

    if not args.build and not args.sign and not args.verify:
        sys.exit(0)

    args.linux = 'l' in args.os
    args.windows = 'w' in args.os
    args.macos = 'm' in args.os

    # Disable for MacOS if no SDK found
    if args.macos and not os.path.isfile('gitian-builder/inputs/Xcode-11.3.1-11C505-extracted-SDK-with-libcxx-headers.tar.gz'):
        print('Cannot build for MacOS, SDK does not exist. Will build for other OSes')
        args.macos = False

    args.sign_prog = 'true' if args.detach_sign else 'gpg --detach-sign'

    script_name = os.path.basename(sys.argv[0])
    if not args.signer:
        print(script_name+': Missing signer')
        print('Try '+script_name+' --help for more information')
        sys.exit(1)
    if not args.version:
        print(script_name+': Missing version')
        print('Try '+script_name+' --help for more information')
        sys.exit(1)

    # Add leading 'v' for tags
    if args.commit and args.pull:
        raise Exception('Cannot have both commit and pull')
    args.commit = ('' if args.commit else 'v') + args.version

    os.chdir('syscoin')
    if args.pull:
        subprocess.check_call(['git', 'fetch', args.url, 'refs/pull/'+args.version+'/merge'])
        os.chdir('../gitian-builder/inputs/syscoin')
        subprocess.check_call(['git', 'fetch', args.url, 'refs/pull/'+args.version+'/merge'])
        args.commit = subprocess.check_output(['git', 'show', '-s', '--format=%H', 'FETCH_HEAD'], universal_newlines=True, encoding='utf8').strip()
        args.version = 'pull-' + args.version
    print(args.commit)
    subprocess.check_call(['git', 'fetch'])
    subprocess.check_call(['git', 'checkout', args.commit])
    os.chdir(workdir)

    os.chdir('gitian-builder')
    subprocess.check_call(['git', 'pull'])
    os.chdir(workdir)

    if args.build:
        build()

    if args.sign:
        sign()

    if args.verify:
        os.chdir('gitian.sigs')
        subprocess.check_call(['git', 'pull'])
        os.chdir(workdir)
        sys.exit(verify())

if __name__ == '__main__':
    main()
