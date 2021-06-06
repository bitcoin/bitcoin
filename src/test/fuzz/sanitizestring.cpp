
// SEGV on SanitizeString()

// If the attacker can control the 2nd arg of SanitizeString it segfaults (reasonably)
// through call cross-reference, SanitizeString is  used on the codebase for printing/debugging
 
// possible attack surface where the control of the 2nd arg of SanitizeString seems to be controllable:

// [1]
/* net.cpp:
 *   void CConnman::PushMessage(CNode* pnode, CSerializedNetMsg&& msg) {
 *       size_t nMessageSize = msg.data.size();    <---------- // [ No boundary check for nMessageSize passed to SanitizeString ]
 *       LogPrint(BCLog::NET, "sending %s (%d bytes) peer=%d\n",  SanitizeString(msg.m_type), nMessageSize, pnode->GetId()); <----- 
 */

 // [2]
 /* net.cpp
  * Another method where the `message size` is passed to SanitizeString without check 
  * std::optional<CNetMessage> V1TransportDeserializer::GetMessage(...)
  *
  */

// NOTE: I do not know if the attacker can actually control &msg at this point, nor how to hit a breakpoint on this path. 
//       just thought important reporting because did not see a boundary check for nMessageSize on PushMessage()

// Find the harness and ASAn log `as PoC for [1]`

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <string>

static const std::string CHARS_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static const std::string SAFE_CHARS[] =
{
    CHARS_ALPHA_NUM + " .,;-_/:?@()", // SAFE_CHARS_DEFAULT
    CHARS_ALPHA_NUM + " .,;-_?@", // SAFE_CHARS_UA_COMMENT
    CHARS_ALPHA_NUM + ".-_", // SAFE_CHARS_FILENAME
    CHARS_ALPHA_NUM + "!*'();:@&=+$,/?#[]-_.~%", // SAFE_CHARS_URI
};

std::string SanitizeString(const std::string& str, int rule)
{
    std::string strResult;
    for (std::string::size_type i = 0; i < str.size(); i++)
    {
        if (SAFE_CHARS[rule].find(str[i]) != std::string::npos)
            strResult.push_back(str[i]);
    }
    return strResult;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    std::string fuzzdata(reinterpret_cast<const char*>(data), size);
    fuzzdata.push_back('\0');

    SanitizeString(fuzzdata, fuzzdata.size());

    return 0;
}


/*
================================================================
==5237==ERROR: AddressSanitizer: SEGV on unknown address 0x000000e83008 (pc 0x7fb170809b42 bp 0x7ffdfe7fa3b0 sp 0x7ffdfe7fa360 T0)
==5237==The signal is caused by a READ memory access.
    #0 0x7fb170809b42 in std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::find(char, unsigned long) const (/lib/x86_64-linux-gnu/libstdc++.so.6+0x144b42)
    #1 0x54df01 in SanitizeString(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, int) /home/nasa/BTC/SanitizeStringFuzz.cc:24:30
    #2 0x54e1a1 in LLVMFuzzerTestOneInput /home/nasa/BTC/SanitizeStringFuzz.cc:36:5
    #3 0x458b81 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) (/home/nasa/BTC/sanitizeStrfuzz+0x458b81)
    #4 0x4582c5 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool*) (/home/nasa/BTC/sanitizeStrfuzz+0x4582c5)
    #5 0x45a567 in fuzzer::Fuzzer::MutateAndTestOne() (/home/nasa/BTC/sanitizeStrfuzz+0x45a567)
    #6 0x45b265 in fuzzer::Fuzzer::Loop(std::__Fuzzer::vector<fuzzer::SizedFile, fuzzer::fuzzer_allocator<fuzzer::SizedFile> >&) (/home/nasa/BTC/sanitizeStrfuzz+0x45b265)
    #7 0x449c1e in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) (/home/nasa/BTC/sanitizeStrfuzz+0x449c1e)
    #8 0x472a62 in main (/home/nasa/BTC/sanitizeStrfuzz+0x472a62)
    #9 0x7fb170364cb1 in __libc_start_main csu/../csu/libc-start.c:314:16
    #10 0x41e99d in _start (/home/nasa/BTC/sanitizeStrfuzz+0x41e99d)
AddressSanitizer can not provide additional info.
SUMMARY: AddressSanitizer: SEGV (/lib/x86_64-linux-gnu/libstdc++.so.6+0x144b42) in std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::find(char, unsigned long) const
==5237==ABORTING
MS: 3 ChangeBinInt-ChangeBit-CrossOver-; base unit: 02f7874ec3f3cfe3af245e89dd66da30d0f0e40c
0x6a,0xf5,0x4a,0xff,0xff,0xa,0x0,0xff,0x86,0xc8,0x0,0xff,0xa,0xa,0xe1,0x86,0xff,0xff,0x0,0x0,0xff,0xff,0xff,0x0,0xff,0x86,0xc8,0x0,0xff,0xa,0xa,0xe1,0xe1,0xe1,0xe1,0xe1,0xe1,0xa,0xa,0x3a,0x3a,0xa,0x6a,0x6a,0xf5,0x4a,0xff,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0x3a,0xff,0x0,0xa,0xa,0xff,0xa,0xa,0x4a,0x3a,0x3a,0xff,0x3a,0x3a,0x3a,0xe1,0xe1,0xe1,0xe1,0x4a,0xe1,0xe1,0x0,0x4a,0x0,0x0,0x0,0x0,0x0,0x0,
j\xf5J\xff\xff\x0a\x00\xff\x86\xc8\x00\xff\x0a\x0a\xe1\x86\xff\xff\x00\x00\xff\xff\xff\x00\xff\x86\xc8\x00\xff\x0a\x0a\xe1\xe1\xe1\xe1\xe1\xe1\x0a\x0a::\x0ajj\xf5J\xff::::::::::\xff\x00\x0a\x0a\xff\x0a\x0aJ::\xff:::\xe1\xe1\xe1\xe1J\xe1\xe1\x00J\x00\x00\x00\x00\x00\x00
artifact_prefix='./'; Test unit written to ./crash-8ed09a14aaf8af1af8ed70e85b5eff8b52da73be
Base64: avVK//8KAP+GyAD/Cgrhhv//AAD///8A/4bIAP8KCuHh4eHh4QoKOjoKamr1Sv86Ojo6Ojo6Ojo6/wAKCv8KCko6Ov86Ojrh4eHhSuHhAEoAAAAAAAA=
*/
