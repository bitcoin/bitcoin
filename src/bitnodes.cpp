// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"
#include <boost/asio.hpp>
#include <boost/asio/basic_deadline_timer.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <istream>
#include <ostream>
#include <stdint.h>
#include <string>
#include <univalue.h>

using namespace std;

/**
 * Get nodes from Bitnodes.21.co using public API.
 */

class client
{
public:
    client(boost::asio::io_service &io_service,
        boost::asio::ssl::context &context,
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
        std::string &cert_hostname,
        std::string &url_host,
        std::string &url_path,
        int timeout_seconds)
        : socket_(io_service, context)
    {
        socket_.set_verify_mode(boost::asio::ssl::verify_peer);

        // Use custom verifier as default rfc2818_verification does not appear to handle SNI
        // socket_.set_verify_callback(boost::asio::ssl::rfc2818_verification(cert_hostname));
        socket_.set_verify_callback(boost::bind(&client::verify_certificate, this, _1, _2));

        boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
            boost::bind(&client::handle_connect, this, boost::asio::placeholders::error));

        timer_.reset(new boost::asio::deadline_timer(io_service, boost::posix_time::seconds(timeout_seconds)));

        url_path_ = url_path;
        url_host_ = url_host;
        cert_hostname_ = cert_hostname;
        found_cert_ = false;
    }

    // Custom verifier to search for a CN name and set member variable if found.
    bool verify_certificate(bool preverified, boost::asio::ssl::verify_context &ctx)
    {
        char subject_name[256];
        X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        std::string s(subject_name);
        std::string pattern = "/CN=" + cert_hostname_;

        if (s.find(pattern) != std::string::npos)
        {
            found_cert_ = true;
        }
        else
        {
            GENERAL_NAMES *altNames = (GENERAL_NAMES *)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
            int numNames = sk_GENERAL_NAME_num(altNames);
            for (int i = 0; i < numNames; ++i)
            {
                GENERAL_NAME *generalName = sk_GENERAL_NAME_value(altNames, i);
                if ((generalName->type == GEN_URI) || (generalName->type == GEN_DNS))
                {
                    std::string san = std::string(
                        reinterpret_cast<char *>(ASN1_STRING_data(generalName->d.uniformResourceIdentifier)),
                        ASN1_STRING_length(generalName->d.uniformResourceIdentifier));
                    if (san.find(cert_hostname_) != std::string::npos)
                    {
                        found_cert_ = true;
                        break;
                    }
                }
                else
                {
                    LogPrintf("Unknown Subject Alternate Name type: %d.  This may cause a bitnodes cert error.\n",
                        generalName->type);
                }
            }
        }

        return true;
    }

    ~client() { timer_.get()->cancel(); }
    void run(boost::asio::io_service &io_service)
    {
        timer_.get()->async_wait(boost::bind(&client::timeout_handler, this, boost::asio::placeholders::error));
        io_service.run();
        timer_.get()->cancel();
    }

    void timeout_handler(const boost::system::error_code &error)
    {
        if (!error)
        {
            LogPrintf("Bitnodes connection timed out.\n");
            socket_.lowest_layer().cancel();
        }
    }

    void handle_connect(const boost::system::error_code &error)
    {
        if (!error)
        {
            socket_.async_handshake(boost::asio::ssl::stream_base::client,
                boost::bind(&client::handle_handshake, this, boost::asio::placeholders::error));
        }
        else
        {
            throw runtime_error(strprintf("Bitnodes connect failure: %s\n", error.message().c_str()));
        }
    }

    void handle_handshake(const boost::system::error_code &error)
    {
        // Throw error if custom verifier did not find a match for the certificate hostname
        if (!found_cert_)
        {
            boost::system::error_code ec;
            socket_.shutdown(ec);
            socket_.lowest_layer().cancel();
            throw runtime_error(strprintf("Bitnodes cert failure: could not match CN: %s\n", cert_hostname_.c_str()));
        }

        if (!error)
        {
            std::stringstream request_;
            // we don't want HTTP/1.1 chunked encoding
            request_ << "GET " << url_path_ << " HTTP/1.0\r\n";
            request_ << "Host: " << url_host_ << "\r\n";
            request_ << "Accept: */*\r\n";
            request_ << "Connection: close\r\n";
            request_ << "\r\n";
            boost::asio::async_write(socket_, boost::asio::buffer(request_.str()),
                boost::bind(&client::handle_write, this, boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            throw runtime_error(strprintf("Bitnodes handshake failure: %s\n", error.message().c_str()));
        }
    }

    void handle_write(const boost::system::error_code &error, size_t bytes_transferred)
    {
        if (!error)
        {
            boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
                boost::bind(&client::handle_read_status_and_headers, this, boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            throw runtime_error(strprintf("Bitnodes HTTP write error: %s\n", error.message().c_str()));
        }
    }

    void handle_read_status_and_headers(const boost::system::error_code &error, size_t bytes_transferred)
    {
        if (!error)
        {
            std::istream response_stream(&response_);
            std::string http_version;
            unsigned int status_code;
            response_stream >> http_version;
            response_stream >> status_code;
            // std::string status_message;
            // std::getline(response_stream, status_message);
            if (!response_stream || http_version.substr(0, 5) != "HTTP/")
            {
                throw runtime_error("Bitnodes response not HTTP");
            }
            if (status_code != 200)
            {
                throw runtime_error(strprintf("Bitnodes returned HTTP status %d\n", status_code));
            }

            // Skip all headers
            string header;
            while (std::getline(response_stream, header) && header != "\r")
                ;

            // Read until EOF
            boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(1),
                boost::bind(&client::handle_read, this, boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            throw runtime_error(strprintf("Bitnodes HTTP read headers error: %s\n", error.message().c_str()));
        }
    }


    void handle_read(const boost::system::error_code &error, size_t bytes_transferred)
    {
        // If EOF / no bytes transferred, exit to avoid "short read" error
        if (bytes_transferred == 0)
        {
            return;
        }
        if (!error)
        {
            std::ostringstream ss;
            ss << &response_;
            content_.append(ss.str());

            // Read until EOF.
            boost::asio::async_read(socket_, response_, boost::asio::transfer_at_least(1),
                boost::bind(&client::handle_read, this, boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            timer_.get()->cancel();
            if (error != boost::asio::error::eof)
            {
                throw runtime_error(strprintf(
                    "Bitnodes HTTP read error: %s. Bytes received: %d\n", error.message().c_str(), bytes_transferred));
            }
        }
    }

    std::string getContent() const { return content_; }
private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::streambuf response_;
    std::string content_;
    boost::scoped_ptr<boost::asio::deadline_timer> timer_;
    std::string url_path_;
    std::string url_host_;
    std::string cert_hostname_;
    bool found_cert_;
};


/**
 * Pass in an empty string vector to be populated with ip:port strings
 */
bool GetLeaderboardFromBitnodes(vector<string> &vIPs)
{
    // Bitnodes connection parameters
    string url_host = "bitnodes.21.co";
    string url_port = "443";
    string url_path = "/api/v1/nodes/leaderboard/?limit=100";
    string cert_hostname = "21.co";
    int timeout = 30;

    int count = 0;
    try
    {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(url_host, url_port);
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

        // Force TLS 1.2
        boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
        ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::no_sslv3 | boost::asio::ssl::context::no_tlsv1
#if (BOOST_VERSON >= 105900)
                        | boost::asio::ssl::context::no_tlsv1_1
#endif
            );

        ctx.set_default_verify_paths();
        client c(io_service, ctx, iterator, cert_hostname, url_host, url_path, timeout);
        c.run(io_service);
        string response = c.getContent();

        // Parse Response
        UniValue valReply(UniValue::VSTR);
        if (!valReply.read(response))
        {
            throw runtime_error("Bitnodes: couldn't parse reply from server");
        }
        const UniValue &reply = valReply.get_obj();
        if (reply.empty())
        {
            throw runtime_error("Bitnodes: reply from server is empty");
        }

        // Parse Leaderboard
        const UniValue &result = find_value(reply, "results");
        if (result.isNull() || !result.isArray())
        {
            throw runtime_error("Bitnodes: server returned invalid results");
        }

        if (result.isArray())
        {
            std::vector<UniValue> v = result.getValues();
            for (std::vector<UniValue>::iterator it = v.begin(); it != v.end(); ++it)
            {
                const UniValue &o = *it;
                const UniValue &result = find_value(o, "node");
                if (result.isStr())
                {
                    string s = result.get_str();
                    vIPs.push_back(s);
                    count++;
                }
            }
        }
    }
    catch (std::exception &e)
    {
        LogPrintf("Bitnodes Exception: %s\n", e.what());
    }

    return (count > 0);
}
