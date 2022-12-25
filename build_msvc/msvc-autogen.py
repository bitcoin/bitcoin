#!/usr/bin/env python3
# Copyright (c) 2016-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
import re
import argparse
from shutil import copyfile

SOURCE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'src'))
DEFAULT_PLATFORM_TOOLSET = R'v143'

libs = [
    'libsyscoin_cli',
    'libsyscoin_common',
    'libsyscoin_crypto',
    'libsyscoin_node',
    'libsyscoin_util',
    'libsyscoin_wallet_tool',
    'libsyscoin_wallet',
    'libsyscoin_zmq',
    'bench_syscoin',
    'libtest_util',
]

ignore_list = [
]

lib_sources = {}


def parse_makefile(makefile):
    with open(makefile, 'r', encoding='utf-8') as file:
        current_lib = ''
        for line in file.read().splitlines():
            if current_lib:
                source = line.split()[0]
                if source.endswith('.cpp') and not source.startswith('$') and source not in ignore_list:
                    source_filename = source.replace('/', '\\')
                    object_filename = source.replace('/', '_')[:-4] + ".obj"
                    lib_sources[current_lib].append((source_filename, object_filename))
                if not line.endswith('\\'):
                    current_lib = ''
                continue
            for lib in libs:
                _lib = lib.replace('-', '_')
                if re.search(_lib + '.*_SOURCES \\= \\\\', line):
                    current_lib = lib
                    lib_sources[current_lib] = []
                    break

def parse_config_into_btc_config():
    def find_between( s, first, last ):
        try:
            start = s.index( first ) + len( first )
            end = s.index( last, start )
            return s[start:end]
        except ValueError:
            return ""

    config_info = []
    with open(os.path.join(SOURCE_DIR,'../configure.ac'), encoding="utf8") as f:
        for line in f:
            if line.startswith("define"):
                config_info.append(find_between(line, "(_", ")"))

    config_info = [c for c in config_info if not c.startswith("COPYRIGHT_HOLDERS")]

    config_dict = dict(item.split(", ") for item in config_info)
    config_dict["PACKAGE_VERSION"] = f"\"{config_dict['CLIENT_VERSION_MAJOR']}.{config_dict['CLIENT_VERSION_MINOR']}.{config_dict['CLIENT_VERSION_BUILD']}\""
    version = config_dict["PACKAGE_VERSION"].strip('"')
    config_dict["PACKAGE_STRING"] = f"\"Syscoin Core {version}\""

    with open(os.path.join(SOURCE_DIR,'../build_msvc/syscoin_config.h.in'), "r", encoding="utf8") as template_file:
        template = template_file.readlines()

    for index, line in enumerate(template):
        header = ""
        if line.startswith("#define"):
            header = line.split(" ")[1]
        if header in config_dict:
            template[index] = line.replace("$", f"{config_dict[header]}")

    with open(os.path.join(SOURCE_DIR,'../build_msvc/syscoin_config.h'), "w", encoding="utf8") as btc_config:
        btc_config.writelines(template)

def set_properties(vcxproj_filename, placeholder, content):
    with open(vcxproj_filename + '.in', 'r', encoding='utf-8') as vcxproj_in_file:
        with open(vcxproj_filename, 'w', encoding='utf-8') as vcxproj_file:
            vcxproj_file.write(vcxproj_in_file.read().replace(placeholder, content))

def main():
    parser = argparse.ArgumentParser(description='Syscoin-core msbuild configuration initialiser.')
    parser.add_argument('-toolset', nargs='?', default=DEFAULT_PLATFORM_TOOLSET,
        help='Optionally sets the msbuild platform toolset, e.g. v143 for Visual Studio 2022.'
         ' default is %s.'%DEFAULT_PLATFORM_TOOLSET)
    args = parser.parse_args()
    set_properties(os.path.join(SOURCE_DIR, '../build_msvc/common.init.vcxproj'), '@TOOLSET@', args.toolset)

    for makefile_name in os.listdir(SOURCE_DIR):
        if 'Makefile' in makefile_name:
            parse_makefile(os.path.join(SOURCE_DIR, makefile_name))
    for key, value in lib_sources.items():
        vcxproj_filename = os.path.abspath(os.path.join(os.path.dirname(__file__), key, key + '.vcxproj'))
        content = ''
        for source_filename, object_filename in value:
            content += '    <ClCompile Include="..\\..\\src\\' + source_filename + '">\n'
            content += '      <ObjectFileName>$(IntDir)' + object_filename + '</ObjectFileName>\n'
            content += '    </ClCompile>\n'
        set_properties(vcxproj_filename, '@SOURCE_FILES@\n', content)
    parse_config_into_btc_config()
    copyfile(os.path.join(SOURCE_DIR,'../build_msvc/syscoin_config.h'), os.path.join(SOURCE_DIR, 'config/syscoin-config.h'))
    copyfile(os.path.join(SOURCE_DIR,'../build_msvc/libsecp256k1_config.h'), os.path.join(SOURCE_DIR, 'secp256k1/src/libsecp256k1-config.h'))

if __name__ == '__main__':
    main()
