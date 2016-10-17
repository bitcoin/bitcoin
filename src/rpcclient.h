// Copyright (c) 2010 Satoshi Nakamoto
<<<<<<< HEAD
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCCLIENT_H
#define BITCOIN_RPCCLIENT_H
=======
// Copyright (c) 2009-2013 The Crowncoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _CROWNCOINRPC_CLIENT_H_
#define _CROWNCOINRPC_CLIENT_H_ 1
>>>>>>> origin/dirty-merge-dash-0.11.0

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"

json_spirit::Array RPCConvertValues(const std::string& strMethod, const std::vector<std::string>& strParams);

<<<<<<< HEAD
#endif // BITCOIN_RPCCLIENT_H
=======
json_spirit::Array RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams);

/** Show help message for crowncoin-cli.
 * The mainProgram argument is used to determine whether to show this message as main program
 * (and include some common options) or as sub-header of another help message.
 *
 * @note the argument can be removed once crowncoin-cli functionality is removed from crowncoind
 */
std::string HelpMessageCli(bool mainProgram);

#endif
>>>>>>> origin/dirty-merge-dash-0.11.0
