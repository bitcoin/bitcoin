// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_FLAGS_H
#define BITCOIN_CONSENSUS_FLAGS_H

/** 
 * Script verification flags.
 * @TODO: decouple bit numbers from script flags.
 */
enum
{
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0), // evaluate P2SH (BIP16) subscripts
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2), // enforce strict DER (BIP66) compliance
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4), // enforce NULLDUMMY (BIP147)
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9), // enable CHECKLOCKTIMEVERIFY (BIP65)
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10), // enable CHECKSEQUENCEVERIFY (BIP112)
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS             = (1U << 11), // enable WITNESS (BIP141)
    bitcoinconsensus_LOCKTIME_VERIFY_SEQUENCE                = (1U << 16), // BIP68: Interpret sequence numbers as relative lock-time constraints
    bitcoinconsensus_TX_VERIFY_BIP30                         = (1U << 17), // BIP30: See http://r6.ca/blog/20120206T005236Z.html for more information
    bitcoinconsensus_SCRIPT_FLAGS_VERIFY_ALL                 = bitcoinconsensus_SCRIPT_FLAGS_VERIFY_P2SH | bitcoinconsensus_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                               bitcoinconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY | bitcoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                                               bitcoinconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY | bitcoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS |
                                                               bitcoinconsensus_LOCKTIME_VERIFY_SEQUENCE | bitcoinconsensus_TX_VERIFY_BIP30,
};

#endif // BITCOIN_CONSENSUS_FLAGS_H
