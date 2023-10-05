#!/usr/bin/env python3
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Download or build previous releases.
# Needs curl and tar to download a release, or the build dependencies when
# building a release.

import argparse
import contextlib
from fnmatch import fnmatch
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import hashlib

SHA256_SUMS = {
    "0e2819135366f150d9906e294b61dff58fd1996ebd26c2f8e979d6c0b7a79580": {"tag": "v0.14.3", "tarball": "bitcoin-0.14.3-aarch64-linux-gnu.tar.gz"},
    "d86fc90824a85c38b25c8488115178d5785dbc975f5ff674f9f5716bc8ad6e65": {"tag": "v0.14.3", "tarball": "bitcoin-0.14.3-arm-linux-gnueabihf.tar.gz"},
    "1b0a7408c050e3d09a8be8e21e183ef7ee570385dc41216698cc3ab392a484e7": {"tag": "v0.14.3", "tarball": "bitcoin-0.14.3-osx64.tar.gz"},
    "706e0472dbc933ed2757650d54cbcd780fd3829ebf8f609b32780c7eedebdbc9": {"tag": "v0.14.3", "tarball": "bitcoin-0.14.3-x86_64-linux-gnu.tar.gz"},
    #
    "d40f18b4e43c6e6370ef7db9131f584fbb137276ec2e3dba67a4b267f81cb644": {"tag": "v0.15.2", "tarball": "bitcoin-0.15.2-aarch64-linux-gnu.tar.gz"},
    "54fb877a148a6ad189a1e1ab1ff8b11181e58ff2aaf430da55b3fd46ae549a6b": {"tag": "v0.15.2", "tarball": "bitcoin-0.15.2-arm-linux-gnueabihf.tar.gz"},
    "87e9340ff3d382d543b2b69112376077f0c8b4f7450d372e83b68f5a1e22b2df": {"tag": "v0.15.2", "tarball": "bitcoin-0.15.2-osx64.tar.gz"},
    "566be44190fd76daa01f13d428939dadfb8e3daacefc8fa17f433cad28f73bd5": {"tag": "v0.15.2", "tarball": "bitcoin-0.15.2-x86_64-linux-gnu.tar.gz"},
    #
    "0768c6c15caffbaca6524824c9563b42c24f70633c681c2744649158aa3fd484": {"tag": "v0.16.3", "tarball": "bitcoin-0.16.3-aarch64-linux-gnu.tar.gz"},
    "fb2818069854a6ad20ea03b28b55dbd35d8b1f7d453e90b83eace5d0098a2a87": {"tag": "v0.16.3", "tarball": "bitcoin-0.16.3-arm-linux-gnueabihf.tar.gz"},
    "78c3bff3b619a19aed575961ea43cc9e142959218835cf51aede7f0b764fc25d": {"tag": "v0.16.3", "tarball": "bitcoin-0.16.3-osx64.tar.gz"},
    "5d422a9d544742bc0df12427383f9c2517433ce7b58cf672b9a9b17c2ef51e4f": {"tag": "v0.16.3", "tarball": "bitcoin-0.16.3-x86_64-linux-gnu.tar.gz"},
    #
    "5a6b35d1a348a402f2d2d6ab5aed653a1a1f13bc63aaaf51605e3501b0733b7a": {"tag": "v0.17.2", "tarball": "bitcoin-0.17.2-aarch64-linux-gnu.tar.gz"},
    "d1913a5d19c8e8da4a67d1bd5205d03c8614dfd2e02bba2fe3087476643a729e": {"tag": "v0.17.2", "tarball": "bitcoin-0.17.2-arm-linux-gnueabihf.tar.gz"},
    "a783ba20706dbfd5b47fbedf42165fce70fbbc7d78003305d964f6b3da14887f": {"tag": "v0.17.2", "tarball": "bitcoin-0.17.2-osx64.tar.gz"},
    "943f9362b9f11130177839116f48f809d83478b4c28591d486ee9a7e35179da6": {"tag": "v0.17.2", "tarball": "bitcoin-0.17.2-x86_64-linux-gnu.tar.gz"},
    #
    "88f343af72803b851c7da13874cc5525026b0b55e63e1b5e1298390c4688adc6": {"tag": "v0.18.1", "tarball": "bitcoin-0.18.1-aarch64-linux-gnu.tar.gz"},
    "cc7d483e4b20c5dabd4dcaf304965214cf4934bcc029ca99cbc9af00d3771a1f": {"tag": "v0.18.1", "tarball": "bitcoin-0.18.1-arm-linux-gnueabihf.tar.gz"},
    "b7bbcee7a7540f711b171d6981f939ca8482005fde22689bc016596d80548bb1": {"tag": "v0.18.1", "tarball": "bitcoin-0.18.1-osx64.tar.gz"},
    "425ee5ec631ae8da71ebc1c3f5c0269c627cf459379b9b030f047107a28e3ef8": {"tag": "v0.18.1", "tarball": "bitcoin-0.18.1-riscv64-linux-gnu.tar.gz"},
    "600d1db5e751fa85903e935a01a74f5cc57e1e7473c15fd3e17ed21e202cfe5a": {"tag": "v0.18.1", "tarball": "bitcoin-0.18.1-x86_64-linux-gnu.tar.gz"},
    #
    "3a80431717842672df682bdb619e66523b59541483297772a7969413be3502ff": {"tag": "v0.19.1", "tarball": "bitcoin-0.19.1-aarch64-linux-gnu.tar.gz"},
    "657f28213823d240dd3324d14829702f9ad6f0710f8bdd1c379cb3c447197f48": {"tag": "v0.19.1", "tarball": "bitcoin-0.19.1-arm-linux-gnueabihf.tar.gz"},
    "1ae1b87de26487075cd2fd22e0d4ead87d969bd55c44f2f1d873ecdc6147ebb3": {"tag": "v0.19.1", "tarball": "bitcoin-0.19.1-osx64.tar.gz"},
    "aa7a9563b48aa79252c8e7b6a41c07a5441bd9f14c5e4562cc72720ea6cb0ee5": {"tag": "v0.19.1", "tarball": "bitcoin-0.19.1-riscv64-linux-gnu.tar.gz"},
    "5fcac9416e486d4960e1a946145566350ca670f9aaba99de6542080851122e4c": {"tag": "v0.19.1", "tarball": "bitcoin-0.19.1-x86_64-linux-gnu.tar.gz"},
    #
    "60c93e3462c303eb080be7cf623f1a7684b37fd47a018ad3848bc23e13c84e1c": {"tag": "v0.20.1", "tarball": "bitcoin-0.20.1-aarch64-linux-gnu.tar.gz"},
    "55b577e0fb306fb429d4be6c9316607753e8543e5946b542d75d876a2f08654c": {"tag": "v0.20.1", "tarball": "bitcoin-0.20.1-arm-linux-gnueabihf.tar.gz"},
    "b9024dde373ea7dad707363e07ec7e265383204127539ae0c234bff3a61da0d1": {"tag": "v0.20.1", "tarball": "bitcoin-0.20.1-osx64.tar.gz"},
    "fa71cb52ee5e0459cbf5248cdec72df27995840c796f58b304607a1ed4c165af": {"tag": "v0.20.1", "tarball": "bitcoin-0.20.1-riscv64-linux-gnu.tar.gz"},
    "376194f06596ecfa40331167c39bc70c355f960280bd2a645fdbf18f66527397": {"tag": "v0.20.1", "tarball": "bitcoin-0.20.1-x86_64-linux-gnu.tar.gz"},

    "43416854330914992bbba2d0e9adf2a6fff4130be9af8ae2ef1186e743d9a3fe": {"tag": "v0.21.0", "tarball": "bitcoin-0.21.0-aarch64-linux-gnu.tar.gz"},
    "f028af308eda45a3c4c90f9332f96b075bf21e3495c945ebce48597151808176": {"tag": "v0.21.0", "tarball": "bitcoin-0.21.0-arm-linux-gnueabihf.tar.gz"},
    "695fb624fa6423f5da4f443b60763dd1d77488bfe5ef63760904a7b54e91298d": {"tag": "v0.21.0", "tarball": "bitcoin-0.21.0-osx64.tar.gz"},
    "f8b2adfeae021a672effbc7bd40d5c48d6b94e53b2dd660f787340bf1a52e4e9": {"tag": "v0.21.0", "tarball": "bitcoin-0.21.0-riscv64-linux-gnu.tar.gz"},
    "da7766775e3f9c98d7a9145429f2be8297c2672fe5b118fd3dc2411fb48e0032": {"tag": "v0.21.0", "tarball": "bitcoin-0.21.0-x86_64-linux-gnu.tar.gz"},

    "ac718fed08570a81b3587587872ad85a25173afa5f9fbbd0c03ba4d1714cfa3e": {"tag": "v22.0", "tarball": "bitcoin-22.0-aarch64-linux-gnu.tar.gz"},
    "b8713c6c5f03f5258b54e9f436e2ed6d85449aa24c2c9972f91963d413e86311": {"tag": "v22.0", "tarball": "bitcoin-22.0-arm-linux-gnueabihf.tar.gz"},
    "2744d199c3343b2d94faffdfb2c94d75a630ba27301a70e47b0ad30a7e0155e9": {"tag": "v22.0", "tarball": "bitcoin-22.0-osx64.tar.gz"},
    "2cca5f99007d060aca9d8c7cbd035dfe2f040dd8200b210ce32cdf858479f70d": {"tag": "v22.0", "tarball": "bitcoin-22.0-powerpc64-linux-gnu.tar.gz"},
    "91b1e012975c5a363b5b5fcc81b5b7495e86ff703ec8262d4b9afcfec633c30d": {"tag": "v22.0", "tarball": "bitcoin-22.0-powerpc64le-linux-gnu.tar.gz"},
    "9cc3a62c469fe57e11485fdd32c916f10ce7a2899299855a2e479256ff49ff3c": {"tag": "v22.0", "tarball": "bitcoin-22.0-riscv64-linux-gnu.tar.gz"},
    "59ebd25dd82a51638b7a6bb914586201e67db67b919b2a1ff08925a7936d1b16": {"tag": "v22.0", "tarball": "bitcoin-22.0-x86_64-linux-gnu.tar.gz"},

    "06f4c78271a77752ba5990d60d81b1751507f77efda1e5981b4e92fd4d9969fb": {"tag": "v23.0", "tarball": "bitcoin-23.0-aarch64-linux-gnu.tar.gz"},
    "952c574366aff76f6d6ad1c9ee45a361d64fa04155e973e926dfe7e26f9703a3": {"tag": "v23.0", "tarball": "bitcoin-23.0-arm-linux-gnueabihf.tar.gz"},
    "7c8bc63731aa872b7b334a8a7d96e33536ad77d49029bad179b09dca32cd77ac": {"tag": "v23.0", "tarball": "bitcoin-23.0-arm64-apple-darwin.tar.gz"},
    "2caa5898399e415f61d9af80a366a3008e5856efa15aaff74b88acf429674c99": {"tag": "v23.0", "tarball": "bitcoin-23.0-powerpc64-linux-gnu.tar.gz"},
    "217dd0469d0f4962d22818c368358575f6a0abcba8804807bb75325eb2f28b19": {"tag": "v23.0", "tarball": "bitcoin-23.0-powerpc64le-linux-gnu.tar.gz"},
    "078f96b1e92895009c798ab827fb3fde5f6719eee886bd0c0e93acab18ea4865": {"tag": "v23.0", "tarball": "bitcoin-23.0-riscv64-linux-gnu.tar.gz"},
    "c816780583009a9dad426dc0c183c89be9da98906e1e2c7ebae91041c1aaaaf3": {"tag": "v23.0", "tarball": "bitcoin-23.0-x86_64-apple-darwin.tar.gz"},
    "2cca490c1f2842884a3c5b0606f179f9f937177da4eadd628e3f7fd7e25d26d0": {"tag": "v23.0", "tarball": "bitcoin-23.0-x86_64-linux-gnu.tar.gz"},

    "0b48b9e69b30037b41a1e6b78fb7cbcc48c7ad627908c99686e81f3802454609": {"tag": "v24.0.1", "tarball": "bitcoin-24.0.1-aarch64-linux-gnu.tar.gz"},
    "37d7660f0277301744e96426bbb001d2206b8d4505385dfdeedf50c09aaaef60": {"tag": "v24.0.1", "tarball": "bitcoin-24.0.1-arm-linux-gnueabihf.tar.gz"},
    "90ed59e86bfda1256f4b4cad8cc1dd77ee0efec2492bcb5af61402709288b62c": {"tag": "v24.0.1", "tarball": "bitcoin-24.0.1-arm64-apple-darwin.tar.gz"},
    "7590645e8676f8b5fda62dc20174474c4ac8fd0defc83a19ed908ebf2e94dc11": {"tag": "v24.0.1", "tarball": "bitcoin-24.0.1-powerpc64-linux-gnu.tar.gz"},
    "79e89a101f23ff87816675b98769cd1ee91059f95c5277f38f48f21a9f7f8509": {"tag": "v24.0.1", "tarball": "bitcoin-24.0.1-powerpc64le-linux-gnu.tar.gz"},
    "6b163cef7de4beb07b8cb3347095e0d76a584019b1891135cd1268a1f05b9d88": {"tag": "v24.0.1", "tarball": "bitcoin-24.0.1-riscv64-linux-gnu.tar.gz"},
    "e2f751512f3c0f00eb68ba946d9c829e6cf99422a61e8f5e0a7c109c318674d0": {"tag": "v24.0.1", "tarball": "bitcoin-24.0.1-x86_64-apple-darwin.tar.gz"},
    "49df6e444515d457ea0b885d66f521f2a26ca92ccf73d5296082e633544253bf": {"tag": "v24.0.1", "tarball": "bitcoin-24.0.1-x86_64-linux-gnu.tar.gz"},

    "3a7bdd959a0b426624f63f394f25e5b7769a5a2f96f8126dcc2ea53f3fa5212b": {"tag": "v25.0", "tarball": "bitcoin-25.0-aarch64-linux-gnu.tar.gz"},
    "e537c8630b05e63242d979c3004f851fd73c2a10b5b4fdbb161788427c7b3c0f": {"tag": "v25.0", "tarball": "bitcoin-25.0-arm-linux-gnueabihf.tar.gz"},
    "3b35075d6c1209743611c705a13575be2668bc069bc6301ce78a2e1e53ebe7cc": {"tag": "v25.0", "tarball": "bitcoin-25.0-arm64-apple-darwin.tar.gz"},
    "0c8e135a6fd297270d3b65196042d761453493a022b5ff7fb847fc911e938214": {"tag": "v25.0", "tarball": "bitcoin-25.0-powerpc64-linux-gnu.tar.gz"},
    "fa8af160782f5adfcea570f72b947073c1663b3e9c3cd0f82b216b609fe47573": {"tag": "v25.0", "tarball": "bitcoin-25.0-powerpc64le-linux-gnu.tar.gz"},
    "fe6e347a66043946920c72c9c4afca301968101e6b82fb90a63d7885ebcceb32": {"tag": "v25.0", "tarball": "bitcoin-25.0-riscv64-linux-gnu.tar.gz"},
    "5708fc639cdfc27347cccfd50db9b73b53647b36fb5f3a4a93537cbe8828c27f": {"tag": "v25.0", "tarball": "bitcoin-25.0-x86_64-apple-darwin.tar.gz"},
    "33930d432593e49d58a9bff4c30078823e9af5d98594d2935862788ce8a20aec": {"tag": "v25.0", "tarball": "bitcoin-25.0-x86_64-linux-gnu.tar.gz"},
}


@contextlib.contextmanager
def pushd(new_dir) -> None:
    previous_dir = os.getcwd()
    os.chdir(new_dir)
    try:
        yield
    finally:
        os.chdir(previous_dir)


def download_binary(tag, args) -> int:
    if Path(tag).is_dir():
        if not args.remove_dir:
            print('Using cached {}'.format(tag))
            return 0
        shutil.rmtree(tag)
    Path(tag).mkdir()
    bin_path = 'bin/bitcoin-core-{}'.format(tag[1:])
    match = re.compile('v(.*)(rc[0-9]+)$').search(tag)
    if match:
        bin_path = 'bin/bitcoin-core-{}/test.{}'.format(
            match.group(1), match.group(2))
    platform = args.platform
    if tag < "v23" and platform in ["x86_64-apple-darwin", "arm64-apple-darwin"]:
        platform = "osx64"
    tarball = 'bitcoin-{tag}-{platform}.tar.gz'.format(
        tag=tag[1:], platform=platform)
    tarballUrl = 'https://bitcoincore.org/{bin_path}/{tarball}'.format(
        bin_path=bin_path, tarball=tarball)

    print('Fetching: {tarballUrl}'.format(tarballUrl=tarballUrl))

    header, status = subprocess.Popen(
        ['curl', '--head', tarballUrl], stdout=subprocess.PIPE).communicate()
    if re.search("404 Not Found", header.decode("utf-8")):
        print("Binary tag was not found")
        return 1

    curlCmds = [
        ['curl', '--remote-name', tarballUrl]
    ]

    for cmd in curlCmds:
        ret = subprocess.run(cmd).returncode
        if ret:
            return ret

    hasher = hashlib.sha256()
    with open(tarball, "rb") as afile:
        hasher.update(afile.read())
    tarballHash = hasher.hexdigest()

    if tarballHash not in SHA256_SUMS or SHA256_SUMS[tarballHash]['tarball'] != tarball:
        if tarball in [v['tarball'] for v in SHA256_SUMS.values()]:
            print("Checksum did not match")
            return 1

        print("Checksum for given version doesn't exist")
        return 1
    print("Checksum matched")

    # Extract tarball
    ret = subprocess.run(['tar', '-zxf', tarball, '-C', tag,
                          '--strip-components=1',
                          'bitcoin-{tag}'.format(tag=tag[1:])]).returncode
    if ret != 0:
        print(f"Failed to extract the {tag} tarball")
        return ret

    Path(tarball).unlink()

    if tag >= "v23" and platform == "arm64-apple-darwin":
        # Starting with v23 there are arm64 binaries for ARM (e.g. M1, M2) macs, but they have to be signed to run
        binary_path = f'{os.getcwd()}/{tag}/bin/'

        for arm_binary in os.listdir(binary_path):
            # Is it already signed?
            ret = subprocess.run(
                ['codesign', '-v', binary_path + arm_binary],
                stderr=subprocess.DEVNULL,  # Suppress expected stderr output
            ).returncode
            if ret == 1:
                # Have to self-sign the binary
                ret = subprocess.run(
                    ['codesign', '-s', '-', binary_path + arm_binary]
                ).returncode
                if ret != 0:
                    print(f"Failed to self-sign {tag} {arm_binary} arm64 binary")
                    return 1

                # Confirm success
                ret = subprocess.run(
                    ['codesign', '-v', binary_path + arm_binary]
                ).returncode
                if ret != 0:
                    print(f"Failed to verify the self-signed {tag} {arm_binary} arm64 binary")
                    return 1

    return 0


def build_release(tag, args) -> int:
    githubUrl = "https://github.com/bitcoin/bitcoin"
    if args.remove_dir:
        if Path(tag).is_dir():
            shutil.rmtree(tag)
    if not Path(tag).is_dir():
        # fetch new tags
        subprocess.run(
            ["git", "fetch", githubUrl, "--tags"])
        output = subprocess.check_output(['git', 'tag', '-l', tag])
        if not output:
            print('Tag {} not found'.format(tag))
            return 1
    ret = subprocess.run([
        'git', 'clone', f'--branch={tag}', '--depth=1', githubUrl, tag
    ]).returncode
    if ret:
        return ret
    with pushd(tag):
        host = args.host
        if args.depends:
            with pushd('depends'):
                ret = subprocess.run(['make', 'NO_QT=1']).returncode
                if ret:
                    return ret
                host = os.environ.get(
                    'HOST', subprocess.check_output(['./config.guess']))
        config_flags = '--prefix={pwd}/depends/{host} '.format(
            pwd=os.getcwd(),
            host=host) + args.config_flags
        cmds = [
            './autogen.sh',
            './configure {}'.format(config_flags),
            'make',
        ]
        for cmd in cmds:
            ret = subprocess.run(cmd.split()).returncode
            if ret:
                return ret
        # Move binaries, so they're in the same place as in the
        # release download
        Path('bin').mkdir(exist_ok=True)
        files = ['bitcoind', 'bitcoin-cli', 'bitcoin-tx']
        for f in files:
            Path('src/'+f).rename('bin/'+f)
    return 0


def check_host(args) -> int:
    args.host = os.environ.get('HOST', subprocess.check_output(
        './depends/config.guess').decode())
    if args.download_binary:
        platforms = {
            'aarch64-*-linux*': 'aarch64-linux-gnu',
            'powerpc64le-*-linux-*': 'powerpc64le-linux-gnu',
            'riscv64-*-linux*': 'riscv64-linux-gnu',
            'x86_64-*-linux*': 'x86_64-linux-gnu',
            'x86_64-apple-darwin*': 'x86_64-apple-darwin',
            'aarch64-apple-darwin*': 'arm64-apple-darwin',
        }
        args.platform = ''
        for pattern, target in platforms.items():
            if fnmatch(args.host, pattern):
                args.platform = target
        if not args.platform:
            print('Not sure which binary to download for {}'.format(args.host))
            return 1
    return 0


def main(args) -> int:
    Path(args.target_dir).mkdir(exist_ok=True, parents=True)
    print("Releases directory: {}".format(args.target_dir))
    ret = check_host(args)
    if ret:
        return ret
    if args.download_binary:
        with pushd(args.target_dir):
            for tag in args.tags:
                ret = download_binary(tag, args)
                if ret:
                    return ret
        return 0
    args.config_flags = os.environ.get('CONFIG_FLAGS', '')
    args.config_flags += ' --without-gui --disable-tests --disable-bench'
    with pushd(args.target_dir):
        for tag in args.tags:
            ret = build_release(tag, args)
            if ret:
                return ret
    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-r', '--remove-dir', action='store_true',
                        help='remove existing directory.')
    parser.add_argument('-d', '--depends', action='store_true',
                        help='use depends.')
    parser.add_argument('-b', '--download-binary', action='store_true',
                        help='download release binary.')
    parser.add_argument('-t', '--target-dir', action='store',
                        help='target directory.', default='releases')
    all_tags = sorted([*set([v['tag'] for v in SHA256_SUMS.values()])])
    parser.add_argument('tags', nargs='*', default=all_tags,
                        help='release tags. e.g.: v0.18.1 v0.20.0rc2 '
                        '(if not specified, the full list needed for '
                        'backwards compatibility tests will be used)'
                        )
    args = parser.parse_args()
    sys.exit(main(args))
