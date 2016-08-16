/*
 * This file is part of the bitcoin-classic project
 * Copyright (C) 2016 Tom Zander <tomz@freedommail.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <networkManager/NetworkManager.h>
#include <Application.h>
#include <Message.h>
#include <streaming/BufferPool.h>

#include <boost/test/auto_unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Connections)

BOOST_AUTO_TEST_CASE(BigMessage)
{
    auto localhost = boost::asio::ip::address_v4::loopback();
    const int PORT = 12551;

    std::list<NetworkConnection> stash;
    int messageSize = -1;

    Application app;
    NetworkManager server(app.ioService());
    server.bind(boost::asio::ip::tcp::endpoint(localhost, PORT), [&stash, &messageSize](NetworkConnection &connection) {
        connection.setOnIncomingMessage([&messageSize](const Message &message) {
            messageSize = message.body().size();
        });
        connection.accept();
        stash.push_back(std::move(connection));
    });

    NetworkManager client(app.ioService());
    EndPoint ep;
    ep.announcePort = PORT;
    ep.ipAddress = localhost;
    auto con = client.connection(ep);
    con.connect();
    const int BigSize = 500000;
    Streaming::BufferPool pool(BigSize);
    for (int i =0; i < BigSize; ++i) {
        pool.data()[i] = 0xFF & i;
    }
    Message message(pool.commit(BigSize), 1);
    con.send(message);

    /*
     * This big message should be split into lots of messages but only
     * one should arrive at the other end.
     */
    boost::this_thread::sleep_for(boost::chrono::seconds(1));
    BOOST_CHECK(messageSize == BigSize);
}

BOOST_AUTO_TEST_SUITE_END()
