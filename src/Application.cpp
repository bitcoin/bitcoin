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
#include "Application.h"

// static
Application * Application::instance()
{
    static Application s_app;
    return &s_app;
}

// static
int Application::exec()
{
    Application *app = Application::instance();
    app->m_threads.join_all();
    return app->m_returnCode;
}

// static
void Application::quit(int rc)
{
    Application *app = Application::instance();
    app->m_returnCode = rc;
    app->m_work.reset();
    app->m_ioservice->stop();
}

Application::Application()
    : m_ioservice(new boost::asio::io_service()),
    m_work(new boost::asio::io_service::work(*m_ioservice)),
    m_returnCode(0)
{
    for (int i = boost::thread::hardware_concurrency(); i > 0; --i) {
        auto ioservice(m_ioservice);
        m_threads.create_thread([ioservice] {
            while(true) {
                try {
                    ioservice->run();
                    return;
                } catch (const boost::thread_interrupted&) {
                    return;
                } catch (const std::exception& ex) {
                    std::cout << "Threadgroup: uncaught exception: " << ex.what() << std::endl;
                }
            }
        });
    }
}

boost::asio::io_service& Application::ioService()
{
    return *m_ioservice;
}

Admin::Server* Application::adminServer()
{
    if (m_adminServer == nullptr) {
        try {
            m_adminServer.reset(new Admin::Server(*m_ioservice));
        } catch (const std::exception &e) {
            std::cout << "Failed to start adminServer " << e.what() << std::endl;
        }
    }
    return m_adminServer.get();
}
