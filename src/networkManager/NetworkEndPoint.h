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
#ifndef NETWORKENDPOINT_H
#define NETWORKENDPOINT_H

#include <string>
#include <cstdint>

#include <boost/asio/ip/address.hpp>

/// Describes a remote server.
struct EndPoint
{
    EndPoint()
        : peerPort(0),
        announcePort(0),
        connectionId(-1)
    {
    }
    boost::asio::ip::address ipAddress;
    std::string hostname;
    std::uint16_t peerPort;
    std::uint16_t announcePort;
    int connectionId;
};

#endif
