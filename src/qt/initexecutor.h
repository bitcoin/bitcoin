// Copyright (c) 2014-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_INITEXECUTOR_H
#define BITCOIN_QT_INITEXECUTOR_H

#include <interfaces/node.h>

#include <exception>

#include <QObject>
#include <QThread>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

/** Class encapsulating Bitcoin Core startup and shutdown.
 * Allows running startup and shutdown in a different thread from the UI thread.
 */
class InitExecutor : public QObject
{
    Q_OBJECT
public:
    explicit InitExecutor(interfaces::Node& node);
    ~InitExecutor();

public Q_SLOTS:
    void initialize();
    void shutdown(bool node_shutdown);

Q_SIGNALS:
    void initializeResult(bool success, interfaces::BlockAndHeaderTipInfo tip_info);
    void shutdownResult();
    void runawayException(const QString& message);

private:
    /// Pass fatal exception message to UI thread
    void handleRunawayException(const std::exception* e);

    interfaces::Node& m_node;
    QObject m_context;
    QThread m_thread;
};

#endif // BITCOIN_QT_INITEXECUTOR_H
