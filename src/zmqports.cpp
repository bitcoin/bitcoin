// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "zmqports.h"

#if ENABLE_ZMQ
#include <zmq.h>
#endif

#include "core.h"
#include "netbase.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;
using namespace std;

// Global state
bool fZMQPub = false;

#if ENABLE_ZMQ
// ZMQ related file scope variables
static void *zmqContext;
static void *zmqPubSocket;

// Internal utility functions
static void zmqError(const char *str)
{
  LogPrint("ZMQ error: %s, errno=%s\n", str, zmq_strerror(errno));
}
#endif

// Called at startup to conditionally set up ZMQ socket(s)
void ZMQInitialize(const std::string &endp)
{
#if ENABLE_ZMQ
  zmqContext = zmq_init(1);
  if (!zmqContext) {
    zmqError("Unable to initialize ZMQ context");
    return;
  }

  zmqPubSocket = zmq_socket(zmqContext, ZMQ_PUB);
  if (!zmqPubSocket) {
    zmqError("Unable to open ZMQ pub socket");
    return;
  }

  int rc = zmq_bind(zmqPubSocket, endp.c_str());
  if (rc != 0) {
    zmqError("Unable to bind ZMQ socket");
    zmq_close(zmqPubSocket);
    zmqPubSocket = 0;
    return;
  }

  fZMQPub = true;
  LogPrint("zmq", "PUB socket listening at %s\n", endp);
#endif
}

#if ENABLE_ZMQ
// Internal function to publish a serialized data stream on a given
// topic
//
// Note: assumes topic is a valid null terminated C string
static void zmqPublish(const char *topic, const CDataStream &ss)
{
  zmq_msg_t msg;
  const unsigned int topiclen = strlen(topic);
  const unsigned int msglen = ss.size() + topiclen;

  // Initialize a new zmq_msg_t to hold serialized content
  int rc = zmq_msg_init_size(&msg, msglen);
  if (rc) {
    zmqError("Unable to initialize ZMQ msg");
    return;
  }

  // Copy topic and serialized TX into message buffer
  unsigned char *buf = (unsigned char *)zmq_msg_data(&msg);
  memcpy(&buf[0], topic, topiclen); // omits null trailer
  memcpy(&buf[topiclen], &ss[0], ss.size());

  // Fire-and-forget
  rc = zmq_msg_send(&msg, zmqPubSocket, 0);
  if (rc == -1) {
    zmqError("Unable to send ZMQ message");
    return;
  }

  LogPrint("zmq", "Published to topic %s\n", topic);
}
#endif

// Called after all transaction relay checks are completed
void ZMQPublishTransaction(const CTransaction &tx)
{
#if ENABLE_ZMQ
  if (!zmqPubSocket)
    return;

  // Serialize transaction
  CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
  ss.reserve(10000); // FIXME used defined constant
  ss << tx;

  zmqPublish("TXN", ss);
#endif
}

// Called after all block checks completed and successfully added to
// disk index
void ZMQPublishBlock(const CBlock &blk)
{
#if ENABLE_ZMQ
  if (!zmqPubSocket)
    return;

  // Serialize block
  CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
  ss.reserve(1000000); // FIXME use defined constant
  ss << blk;

  zmqPublish("BLK", ss);
#endif
}

// Called during shutdown sequence
void ZMQShutdown()
{
#if ENABLE_ZMQ
  if (zmqContext) {
    if (zmqPubSocket) {
      // Discard any unread messages and close socket
      int linger = 0;
      zmq_setsockopt(zmqPubSocket, ZMQ_LINGER, &linger, sizeof(linger));
      zmq_close(zmqPubSocket);
      zmqPubSocket = 0;
    }

    zmq_ctx_destroy(zmqContext);
    zmqContext = 0;
  }
#endif

  fZMQPub = false;
}

Value getzmqurl(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getzmqurl\n"
            "Returns an object containing ZMQ notification endpoint.\n"
            "\nResult:\n"
            "{\n"
            "  \"zmqurl\": \"tcp//xxx:xx\",   (string) the ZMQ endpoint specifier\n"
            "}\n"
        );

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj;
    obj.push_back(Pair("zmqurl", mapArgs["-zmqpub"]));
    return obj;
}
