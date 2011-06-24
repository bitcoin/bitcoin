// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_RPC_H
#define BITCOIN_RPC_H

class CBlock;
class CBlockIndex;
class CWalletTx;

void ThreadRPCServer(void* parg);
int CommandLineRPC(int argc, char *argv[]);

void ThreadHTTPPOST(void* parg);

void monitorTx(const CWalletTx& tx);
void monitorBlock(const CBlock& block, const CBlockIndex* pblockindex);

#ifdef USE_SSL
#include <boost/asio.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/asio/ssl.hpp> 
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLStream;
//
// IOStream device that speaks SSL but can also speak non-SSL
//
class SSLIOStreamDevice : public boost::iostreams::device<boost::iostreams::bidirectional> {
public:
    SSLIOStreamDevice(SSLStream &streamIn, bool fUseSSLIn) : stream(streamIn)
    {
        fUseSSL = fUseSSLIn;
        fNeedHandshake = fUseSSLIn;
    }

    void handshake(boost::asio::ssl::stream_base::handshake_type role)
    {
        if (!fNeedHandshake) return;
        fNeedHandshake = false;
        stream.handshake(role);
    }
    std::streamsize read(char* s, std::streamsize n)
    {
        handshake(boost::asio::ssl::stream_base::server); // HTTPS servers read first
        if (fUseSSL) return stream.read_some(boost::asio::buffer(s, n));
        return stream.next_layer().read_some(boost::asio::buffer(s, n));
    }
    std::streamsize write(const char* s, std::streamsize n)
    {
        handshake(boost::asio::ssl::stream_base::client); // HTTPS clients write first
        if (fUseSSL) return boost::asio::write(stream, boost::asio::buffer(s, n));
        return boost::asio::write(stream.next_layer(), boost::asio::buffer(s, n));
    }
    bool connect(const std::string& server, const std::string& port)
    {
        boost::asio::ip::tcp::resolver resolver(stream.get_io_service());
        boost::asio::ip::tcp::resolver::query query(server.c_str(), port.c_str());
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::asio::ip::tcp::resolver::iterator end;
        boost::system::error_code error = boost::asio::error::host_not_found;
        while (error && endpoint_iterator != end)
        {
            stream.lowest_layer().close();
            stream.lowest_layer().connect(*endpoint_iterator++, error);
        }
        if (error)
            return false;
        return true;
    }

private:
    bool fNeedHandshake;
    bool fUseSSL;
    SSLStream& stream;
};
#endif /* USE_SSL */

#endif /* BITCOIN_RPC_H */

