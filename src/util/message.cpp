// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/message.h>

#include <hash.h>
#include <serialize.h>

#include <string>

// Text used to signify that a signed message follows and to prevent
// inadvertently signing a transaction.
static const std::string MESSAGE_PREAMBLE = "Bitcoin Signed Message:\n";

uint256 HashMessage(const std::string& message)
{
    CHashWriter hasher(SER_GETHASH, 0);
    hasher << MESSAGE_PREAMBLE << message;

    return hasher.GetHash();
}
