// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The BitcoinPoS Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRESSBOOK_H
#define BITCOIN_ADDRESSBOOK_H

#include <map>
#include <string>

namespace AddressBook {

    namespace AddressBookPurpose {
        extern const std::string UNKNOWN;
        extern const std::string RECEIVE;
        extern const std::string SEND;
    }

    /** Address book data */
    class CAddressBookData
    {
    private:
        bool m_change{true};
        std::string m_label;
    public:
        std::string purpose;

        CAddressBookData() : purpose(AddressBook::AddressBookPurpose::UNKNOWN) {}

        typedef std::map<std::string, std::string> StringMap;
        StringMap destdata;

        bool IsChange() const { return m_change; }
        const std::string& GetLabel() const { return m_label; }
        void SetLabel(const std::string& label) {
            m_change = false;
            m_label = label;
        }
        bool isSendPurpose() const;
        bool isReceivePurpose() const;
    };

}

#endif //BITCOIN_ADDRESSBOOK_H
