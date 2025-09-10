#!/usr/bin/env python3
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
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
    "d1f7121a7d7bdd4077709284076860389d6a0f4481a934ad9acb85cae3d7b83e":  "dashcore-20.0.1-aarch64-linux-gnu.tar.gz",
    "37375229e5ab18d7050b729fb016df24acdd72d60bc3fa074270d89030a27827":  "dashcore-20.0.1-arm-linux-gnueabihf.tar.gz",
    "ab530f72d2bfbfcd7fca0644e3ea5c5b279e2204fe50ff7bd9cc452a0d413c65":  "dashcore-20.0.1-arm64-apple-darwin.dmg",
    "8f4b55e4a3d6bb38a0c1f51ece14f387fd4dcffa000aeecfbbd1f751da8b4446":  "dashcore-20.0.1-arm64-apple-darwin.tar.gz",
    "1d9cdb00d93e8babe9f54d0ecb04c55f2cd6fd6cfaa85466aa7f95a6976d040d":  "dashcore-20.0.1-riscv64-linux-gnu.tar.gz",
    "f722954c38d5b18f8290e41ca9dd833929258dcf68c9396cbbc81d241285947b":  "dashcore-20.0.1-win64-setup.exe",
    "bb6d59a3eadac316e86e073a9f7ca4d28f3a2e8a59b7109d509a7368675a6f5f":  "dashcore-20.0.1-win64.zip",
    "5373a84f49e4f76bd04987806f5fcde0b537fa1408e1f98370680f3f5134970f":  "dashcore-20.0.1-x86_64-apple-darwin.dmg",
    "0c9344961ae5800f54ffc90af63826cdbf61acc5c442f3fab6527d528f2d9323":  "dashcore-20.0.1-x86_64-apple-darwin.tar.gz",
    "7c82bdbd1c2de515d6c7245886d8c0b0044a4a9b6f74166b8d58c82cd4ae3270":  "dashcore-20.0.1-x86_64-linux-gnu.tar.gz",
    "bb898a8e4c54fd5989f114673e1fee5116bf6ffa257c63397993035c390de806":  "dashcore-20.0.1.tar.gz",
    #
    "a4b555b47f5f9a5a01fc5d3b543731088bd10a65dd7fa81fb552818146e424b5":  "dashcore-19.3.0-aarch64-linux-gnu.tar.gz",
    "531bb188c1aea808ef6f3533d71182a51958136f6e43d9fcadaef1a5fcdd0468":  "dashcore-19.3.0-osx.dmg",
    "1b4673a2bd71f9f2b593c2d71386e60f4744b59b57142707f0045ed49c92024b":  "dashcore-19.3.0-osx64.tar.gz",
    "d23cd59ab3a230ebb9bd34fa6329e0d157ecfdbd133f171dfdfa08039d0b3983":  "dashcore-19.3.0-riscv64-linux-gnu.tar.gz",
    "b5c1860440f97dbb79b1d79bcc48fb2dcc7f0915dd0c4f9fc77aba9cab0294f3":  "dashcore-19.3.0-win64-setup.exe",
    "8a288189bd4b7c23bb1f917256290dd606d9d47a533dcede0c6190a8f4722e1a":  "dashcore-19.3.0-win64.zip",
    "c2f3ff5631094abe16af8e476d1197be8685ee20601deda5cad0c34fc879c3de":  "dashcore-19.3.0-x86_64-linux-gnu.tar.gz",
    "b4bb6bec21213e47586607657e69b0a53905e3c32e2e8e650e93db54dce572d8":  "dashcore-19.3.0.tar.gz",
    #
    "2870149fda49e731fdf67951408e8b9a1f21f4d91693add0287fe6abb7f8e5b4":  "dashcore-19.1.0-aarch64-linux-gnu.tar.gz",
    "900f6209831f1a21be7ed51edd48a6312fdfb8759fac94b77b23d77484254356":  "dashcore-19.1.0-osx64.tar.gz",
    "4ff370e904f08f9b31727535c5ccdde616d7cdee2fb9396aa887910fc87702ff":  "dashcore-19.1.0-osx.dmg",
    "49fb6cc79429cd46e57b9b1197c863dac5ca56a17004596d9cc364f5fcf395f8":  "dashcore-19.1.0-riscv64-linux-gnu.tar.gz",
    "4aad6aedd3b45ae8c5279ad6ee886e7d80a1faa59be9bae882bdd6df68992990":  "dashcore-19.1.0-x86_64-linux-gnu.tar.gz",
    #
    "d7907726666e9266f5eae830789a1c36cf8c84b43bc0c0fab907317a5cc03f09":  "dashcore-18.2.2-aarch64-linux-gnu.tar.gz",
    "b70c5fb7c916f093840b9adb6f0287488843e0e69b403e99ed0bc93d34e24f85":  "dashcore-18.2.2-osx.dmg",
    "9b376e99400a3b0cb8e777477cf07567c36ed65018e4becbd98eebfbcca3efee":  "dashcore-18.2.2-osx64.tar.gz",
    "091d704b58e51171fcad7de24ea6d9db644834cfa5df610517a9528a70b9c4cf":  "dashcore-18.2.2-riscv64-linux-gnu.tar.gz",
    "be3f3c7f3f98198837dc6739cc99126f9de6ddb38c0cf6d291068a65e3c6dede":  "dashcore-18.2.2-win64-setup.exe",
    "ade7b79182443a04b101a4a51cdc402425e583873aafa44a136e4937f89bde61":  "dashcore-18.2.2-win64.zip",
    "ebe3170835232c0a1e7456712fbb285d5749cbf3cfb5de29f8db3a2ad81be1cf":  "dashcore-18.2.2-x86_64-linux-gnu.tar.gz",
    "76c5c44961d30d570c430325fe145fbd413c8038e88994dc6c93fa2da4dc2dd7":  "dashcore-18.2.2.tar.gz",
    #
    "57b7b9a7939a726afc133229dbecc4f46394bf57d32bf3e936403ade2b2a8519":  "dashcore-0.17.0.3-aarch64-linux-gnu.tar.gz",
    "024fa38c48925a684bac217c0915451a3f9d15a1048035ef41ca0c9ffa8c6801":  "dashcore-0.17.0.3-arm-linux-gnueabihf.tar.gz",
    "349c65fb6c0d2d509d338b5bd8ca25b069b84c6d57b49a9d1bc830744e09926b":  "dashcore-0.17.0.3-osx64.tar.gz",
    "5b6ce9f43fc07f5e73c0de6890929adcda31e29479f06605b4f7434e04348041":  "dashcore-0.17.0.3-osx.dmg",
    "baf1e0e24c7d2a699898a33b10f4b9c2fb6059286a6c336fd4921a58a4e8eb80":  "dashcore-0.17.0.3.tar.gz",
    "f46958c99a9d635dea81f7a76ab079f25816a428eecdb0db556bdec4c08b8418":  "dashcore-0.17.0.3-win32-setup.exe",
    "3c02bbd6e8d232b24ab72789f91c5be7197f105ee4db03a597fd6a262098c713":  "dashcore-0.17.0.3-win32.zip",
    "347cd9b1899274eef62fca55f9e0bc929dc48482866a89f2098671cf68e9ace6":  "dashcore-0.17.0.3-win64-setup.exe",
    "e606767165adc16d2a02a510d029e2bb4fc47e0beca548fd4ef5be675d3635ab":  "dashcore-0.17.0.3-win64.zip",
    "d4086b1271589e8d72e6ca151a1c8f12e4dc2878d60ec69532d0c48e99391996":  "dashcore-0.17.0.3-x86_64-linux-gnu.tar.gz",
    #
    "b0fd7b1344701f6b96f6b6978fbce7fd5d3e0310a2993e17858573d80e2941c0":  "dashcore-0.16.1.1-aarch64-linux-gnu.tar.gz",
    "6d829fb8a419db93d99f03a12d0a426292cfba916fa7173107f7a760e4d1cd56":  "dashcore-0.16.1.1-arm-linux-gnueabihf.tar.gz",
    "3f26d7da7b3ea5ce1fabf34b4086a978324d5806481dc8470b15294a0807100d":  "dashcore-0.16.1.1-osx64.tar.gz",
    "49a5ca7364b62f9908239e12da8181c9bbe8b7ca6508bc569f05907800af084c":  "dashcore-0.16.1.1-osx.dmg",
    "8cd15db027b1745a9205c2067a2e5113772696535ec521a7fc9f6d7b2583e0ea":  "dashcore-0.16.1.1.tar.gz",
    "dced7d9588e80d3d97d2c06fb2352e0ccb97e23698ed78f2db21d94c6550f2e4":  "dashcore-0.16.1.1-win32-setup.exe",
    "1c6efe0f70702f4abd6ce42e0dffe9c311f3b71174ab497defc03cd69cebcfc4":  "dashcore-0.16.1.1-win32.zip",
    "72daff27358d87c1154161e04bf4cb4091ef8cf506b196503c4e58f943913030":  "dashcore-0.16.1.1-win64-setup.exe",
    "3f9bf89da1eb0354f06020d107926ebeb799625d792954d2b9d1436dfdea014e":  "dashcore-0.16.1.1-win64.zip",
    "8803928bd18e9515f1254f97751eb6e537a084d66111ce7968eafb23e26bf3a5":  "dashcore-0.16.1.1-x86_64-linux-gnu.tar.gz",
    #
    "e2c7f2566e26420a54c8d08e1f8a8d5595bb22fba46d3a84ab931f5cd0efc7f9":  "dashcore-0.15.0.0-aarch64-linux-gnu.tar.gz",
    "be3a2054eb39826bd416252ab3c9a233e90a27b545739b15fb4c9c399b0fbe68":  "dashcore-0.15.0.0-arm-linux-gnueabihf.tar.gz",
    "f5e5d25df3d84a9e5dceef43d0bcf54fa697ea73f3e29ec39a8f9952ace8792c":  "dashcore-0.15.0.0-osx64.tar.gz",
    "09f76396217eef6e5a7ba464d9b1f5abd78925b314f663bb709fdb02013899df":  "dashcore-0.15.0.0-osx.dmg",
    "8a1088477198b3cd549017246ecbd4d03ddafae772a0344c92a0c4d9478d90b6":  "dashcore-0.15.0.0.tar.gz",
    "a647e7ba87e8e31fba0683a61a02c578a967a58901784b9b96f0df1233293241":  "dashcore-0.15.0.0-win32-setup.exe",
    "404d5451a782fabda43197d0608d7cfab1a51a02695c72cdada4507f1db5f99c":  "dashcore-0.15.0.0-win32.zip",
    "f532bc7e0360e80908eb6b9c3aeec7e0037e70e25dee3b040dbbf7a124e05619":  "dashcore-0.15.0.0-win64-setup.exe",
    "3ba6ff98113af30319fb1499d132d993633380476f9980443d630d21a40e0efb":  "dashcore-0.15.0.0-win64.zip",
    "4cc0815ebd595f3d0134a8df9e6224cbe3d79398a5a899b60ca5f4ab8a576160":  "dashcore-0.15.0.0-x86_64-linux-gnu.tar.gz",
    #
    "060c86587176ffcea5615ea46aa080152ea5ebd035d2a7e0340d312345c2eee0":  "dashcore-0.12.1.5-RPi2.tar.gz",
    "369dee3e2ae8854592ca696fd5fd7314bc8bbdd865920407bc96d59acb06f81d":  "dashcore-0.12.1.5-linux32.tar.gz",
    "81692699b99d64b50d1bb1db427f6a0d68c84b98d0e0152f854cfbe048c08fc4":  "dashcore-0.12.1.5-linux64.tar.gz",
    "b4514d4a705cc1adb400ec0c69630612fe394508decd7bf3edef068021fc47b5":  "dashcore-0.12.1.5-osx.dmg",
    "35eb7bd9f5883d986b32ae84ddd074566c9e01a438bddd0d147c4a1e96b81b02":  "dashcore-0.12.1.5-win32-setup.exe",
    "2ea20e8671dcae6ed4f980b8b7dcaa1024961e0267956f4e8bd9eea31b0030e4":  "dashcore-0.12.1.5-win32.zip",
    "db276b7697777203e98f93e1f498bdab0016f2a96f6454a56c51846120aae0b7":  "dashcore-0.12.1.5-win64-setup.exe",
    "15ee0d8405609aaf70c31ff320b0468c86e7c6bc1f061ae90d6295a540576c39":  "dashcore-0.12.1.5-win64.zip",
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
    bin_path = 'releases/download/v{}'.format(tag[1:])
    match = re.compile('v(.*)(rc[0-9]+)$').search(tag)
    if match:
        bin_path = 'releases/download/test.{}'.format(
            match.group(1), match.group(2))
    platform = args.platform
    if platform in ["x86_64-w64-mingw32"]:
        platform = "win64"
    elif tag < "v0.12.3":
        if platform in ["arm-linux-gnueabihf"]:
            platform = "RPi2"
        elif platform in ["x86_64-apple-darwin", "arm64-apple-darwin"]:
            print(f"Binaries not available for {tag} on {platform}")
            return 1
        elif platform in ["x86_64-linux-gnu"]:
            platform = "linux64"
    elif tag < "v20" and platform in ["x86_64-apple-darwin", "arm64-apple-darwin"]:
        platform = "osx64"
    tarball = 'dashcore-{tag}-{platform}.tar.gz'.format(
        tag=tag[1:], platform=platform)
    tarballUrl = 'https://github.com/dashpay/dash/{bin_path}/{tarball}'.format(
        bin_path=bin_path, tarball=tarball)

    print('Fetching: {tarballUrl}'.format(tarballUrl=tarballUrl))

    header, status = subprocess.Popen(
        ['curl', '--head', tarballUrl], stdout=subprocess.PIPE).communicate()
    if re.search("404 Not Found", header.decode("utf-8")):
        print("Binary tag was not found")
        return 1

    curlCmds = [
        ['curl', '-L', '--remote-name', tarballUrl]
    ]

    for cmd in curlCmds:
        ret = subprocess.run(cmd).returncode
        if ret:
            return ret

    hasher = hashlib.sha256()
    with open(tarball, "rb") as afile:
        hasher.update(afile.read())
    tarballHash = hasher.hexdigest()

    if tarballHash not in SHA256_SUMS or SHA256_SUMS[tarballHash] != tarball:
        if tarball in SHA256_SUMS.values():
            print("Checksum did not match")
            return 1

        print("Checksum for given version doesn't exist")
        return 1
    print("Checksum matched")

    # Extract tarball
    # special case for v17 and earlier: other name of version
    filename = tag[1:-2] if tag[1:3] == "0." else tag[1:]
    ret = subprocess.run(['tar', '-zxf', tarball, '-C', tag,
                          '--strip-components=1',
                          'dashcore-{tag}'.format(tag=filename, platform=args.platform)]).returncode
    if ret != 0:
        print(f"Failed to extract the {tag} tarball")
        return ret

    Path(tarball).unlink()

    if tag >= "v19" and platform == "arm64-apple-darwin":
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
    githubUrl = "https://github.com/dashpay/dash"
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
        files = ['dashd', 'dash-cli', 'dash-tx']
        for f in files:
            Path('src/'+f).rename('bin/'+f)
    return 0


def check_host(args) -> int:
    args.host = os.environ.get('HOST', subprocess.check_output(
        './depends/config.guess').decode())
    if args.download_binary:
        platforms = {
            'aarch64-*-linux*': 'aarch64-linux-gnu',
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
    parser.add_argument('tags', nargs='+',
                        help="release tags. e.g.: v19.1.0 v19.0.0-rc.9")
    args = parser.parse_args()
    sys.exit(main(args))
