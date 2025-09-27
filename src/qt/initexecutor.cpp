// Copyright (c) 2014-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/initexecutor.h>

#include <interfaces/node.h>
#include <util/system.h>
#include <util/threadnames.h>

#include <exception>

#include <QApplication>
#include <QDebug>
#include <QMetaObject>
#include <QObject>
#include <QProcess>
#include <QString>

InitExecutor::InitExecutor(interfaces::Node& node)
    : QObject(), m_node(node)
{
    m_context.moveToThread(&m_thread);
    m_thread.start();
}

InitExecutor::~InitExecutor()
{
    qDebug() << __func__ << ": Stopping thread";
    m_thread.quit();
    m_thread.wait();
    qDebug() << __func__ << ": Stopped thread";
}

void InitExecutor::handleRunawayException(const std::exception_ptr e)
{
    PrintExceptionContinue(e, "Runaway exception");
    Q_EMIT runawayException(QString::fromStdString(m_node.getWarnings().translated));
}

void InitExecutor::initialize()
{
    QMetaObject::invokeMethod(&m_context, [this] {
        try {
            util::ThreadRename("qt-init");
            qDebug() << "Running initialization in thread";
            interfaces::BlockAndHeaderTipInfo tip_info;
            bool rv = m_node.appInitMain(&tip_info);
            Q_EMIT initializeResult(rv, tip_info);
        } catch (...) {
            handleRunawayException(std::current_exception());
        }
    });
}

void InitExecutor::restart(QStringList args)
{
    QMetaObject::invokeMethod(&m_context, [this, args] {
        static bool executing_restart{false};

        if (!executing_restart) { // Only restart 1x, no matter how often a user clicks on a restart-button
            executing_restart = true;
            try {
                qDebug() << "Running Restart in thread";
                m_node.appPrepareShutdown();
                qDebug() << "Shutdown finished";
                Q_EMIT shutdownResult();
                CExplicitNetCleanup::callCleanup();
                QProcess::startDetached(QApplication::applicationFilePath(), args);
                qDebug() << "Restart initiated...";
                QCoreApplication::exit(0);;
            } catch (...) {
                handleRunawayException(std::current_exception());
            }
        }
    });
}

void InitExecutor::shutdown()
{
    QMetaObject::invokeMethod(&m_context, [this] {
        try {
            qDebug() << "Running Shutdown in thread";
            m_node.appShutdown();
            qDebug() << "Shutdown finished";
            Q_EMIT shutdownResult();
        } catch (...) {
            handleRunawayException(std::current_exception());
        }
    });
}
