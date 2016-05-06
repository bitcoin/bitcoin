// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HTTPRPC_H
#define BITCOIN_HTTPRPC_H

#include <string>
#include <map>

class HTTPRequest;
class UniValue;

/** Start HTTP RPC subsystem.
 * Precondition; HTTP and RPC has been started.
 */
bool StartHTTPRPC();
/** Interrupt HTTP RPC subsystem.
 */
void InterruptHTTPRPC();
/** Stop HTTP RPC subsystem.
 * Precondition; HTTP and RPC has been stopped.
 */
void StopHTTPRPC();

/** Start HTTP REST subsystem.
 * Precondition; HTTP and RPC has been started.
 */
bool StartREST();
/** Interrupt RPC REST subsystem.
 */
void InterruptREST();
/** Stop HTTP REST subsystem.
 * Precondition; HTTP and RPC has been stopped.
 */
void StopREST();

/** Handles authentication, checks httpbase-auth header.
 * Write authorize header if required.
 * Responses true/false and populates the HTTPRequest
 */
bool HTTPReq_HandleAuth(HTTPRequest* req);

/** populates HTTPRequest with the UniValue objError and the JSONRPC request id */
void JSONErrorReply(HTTPRequest* req, const UniValue& objError, const UniValue& id);

#endif
