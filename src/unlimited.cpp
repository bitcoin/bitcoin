// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>

#include "util.h"
#include "unlimited.h"
#include "clientversion.h"
#include "rpcserver.h"
#include "utilstrencodings.h"
#include "tinyformat.h"

uint64_t maxGeneratedBlock = DEFAULT_MAX_GENERATED_BLOCK_SIZE;


void UnlimitedSetup(void)
{
   maxGeneratedBlock = GetArg("-blockmaxsize", DEFAULT_MAX_GENERATED_BLOCK_SIZE);
}



std::string LicenseInfo()
{
    return FormatParagraph(strprintf(_("Copyright (C) 2009-%i The Bitcoin Unlimited Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           FormatParagraph(strprintf(_("Portions Copyright (C) 2009-%i The Bitcoin Core Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           FormatParagraph(strprintf(_("Portions Copyright (C) 2009-%i The Bitcoin XT Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           "\n" +
           FormatParagraph(_("This is experimental software.")) + "\n" +
           "\n" +
           FormatParagraph(_("Distributed under the MIT software license, see the accompanying file COPYING or <http://www.opensource.org/licenses/mit-license.php>.")) + "\n" +
           "\n" +
           FormatParagraph(_("This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit <https://www.openssl.org/> and cryptographic software written by Eric Young and UPnP software written by Thomas Bernard.")) +
           "\n";
}
