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
#ifndef APPLICATION_H
#define APPLICATION_H

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>

#include <memory>

#include "AdminServer.h"

/**
 * An application singleton that manages some application specific data.
 * The Application singleton can be used to have a single place that owns a threadpool
 * which becomes easy to reach from all your classes and thus avoids cross-dependencies.
 * The IoService is lazy-initialized on first call to IoService() and as such you
 * won't have any negative side-effects if the application does not use them (yet).
 */
class Application
{
public:
    Application();

    /// returns (and optionally creates) an instance
    static Application *instance();

    static int exec();

    static void quit(int rc = 0);

    boost::asio::io_service& ioService();

    /**
     * Creates an admin server which will immediately start to listen on the appropriate ports.
     * Notice that this method may throw a runtime_error if setup fails.
     */
    Admin::Server* adminServer();

private:
    std::shared_ptr<boost::asio::io_service> m_ioservice;
    std::unique_ptr<boost::asio::io_service::work> m_work;
    boost::thread_group m_threads;

    std::unique_ptr<Admin::Server> m_adminServer;

    int m_returnCode;
};

#endif
