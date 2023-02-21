# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(CheckIncludeFileCXX)
include(TestBigEndian)

test_big_endian(WORDS_BIGENDIAN)

# The following HAVE_{HEADER}_H variables go to the bitcoin-config.h header.
check_include_file_cxx(byteswap.h HAVE_BYTESWAP_H)
check_include_file_cxx(endian.h HAVE_ENDIAN_H)
check_include_file_cxx(sys/endian.h HAVE_SYS_ENDIAN_H)
check_include_file_cxx(sys/prctl.h HAVE_SYS_PRCTL_H)
check_include_file_cxx(sys/resources.h HAVE_SYS_RESOURCES_H)
check_include_file_cxx(sys/vmmeter.h HAVE_SYS_VMMETER_H)
check_include_file_cxx(vm/vm_param.h HAVE_VM_VM_PARAM_H)
