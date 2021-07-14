// Copyright (c) 2014-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/initexecutor.h>

#include <interfaces/node.h>
#include <util/system.h>
#include <util/threadnames.h>

#include <exception>

#include <QDebug>
#include <QObject>
#include <QString>
#include <QThread>

InitExecutor::InitExecutor(interfaces::Node& node) :
    QObject(), m_node(node)
{
    this->moveToThread(&m_thread);
    m_thread.start();
}

InitExecutor::~InitExecutor()
{
    qDebug() << __func__ << ": Stopping thread";
    m_thread.quit();
    m_thread.wait();
    qDebug() << __func__ << ": Stopped thread";
}

void InitExecutor::handleRunawayException(const std::exception *e)
{
    PrintExceptionContinue(e, "Runaway exception");
    Q_EMIT runawayException(QString::fromStdString(m_node.getWarnings().translated));
}

void InitExecutor::initialize()
{
    try
    {
        util::ThreadRename("qt-init");
        qDebug() << __func__ << ": Running initialization in thread";
        interfaces::BlockAndHeaderTipInfo tip_info;
        bool rv = m_node.appInitMain(&tip_info);
        Q_EMIT initializeResult(rv, tip_info);
    } catch (const std::exception& e) {
        handleRunawayException(&e);
    } catch (...) {
        handleRunawayException(nullptr);
    }
}

void InitExecutor::shutdown()
{
    try
    {
        qDebug() << __func__ << ": Running Shutdown in thread";
        m_node.appShutdown();
        qDebug() << __func__ << ": Shutdown finished";
        Q_EMIT shutdownResult();
    } catch (const std::exception& e) {
        handleRunawayException(&e);
    } catch (...) {
        handleRunawayException(nullptr);
    }
}
