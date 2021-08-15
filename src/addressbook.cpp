// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The BitcoinPoS Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressbook.h"
#include <string>

namespace AddressBook {

    namespace AddressBookPurpose {
        const std::string UNKNOWN{"unknown"};
        const std::string RECEIVE{"receive"};
        const std::string SEND{"send"};
    }

    bool CAddressBookData::isSendPurpose() const
    {
        return purpose == AddressBookPurpose::SEND;
    }

    bool CAddressBookData::isReceivePurpose() const
    {
        return purpose == AddressBookPurpose::RECEIVE;
    }
}

