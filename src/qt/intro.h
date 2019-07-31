// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_INTRO_H
#define BITCOIN_QT_INTRO_H

#include <QDialog>
#include <QMutex>
#include <QThread>

static const bool DEFAULT_CHOOSE_DATADIR = false;

class FreespaceChecker;

namespace interfaces {
    class Node;
}

namespace Ui {
    class Intro;
}

/** Introduction screen (pre-GUI startup).
  Allows the user to choose a data directory,
  in which the wallet and block chain will be stored.
 */
class Intro : public QDialog
{
    Q_OBJECT

public:
    explicit Intro(QWidget *parent = nullptr,
                   uint64_t blockchain_size = 0, uint64_t chain_state_size = 0);
    ~Intro();

    QString getDataDirectory();
    void setDataDirectory(const QString &dataDir);

    /**
     * Determine data directory. Let the user choose if the current one doesn't exist.
     *
     * @returns true if a data directory was selected, false if the user cancelled the selection
     * dialog.
     *
     * @note do NOT call global GetDataDir() before calling this function, this
     * will cause the wrong path to be cached.
     */
    static bool pickDataDirectory(interfaces::Node& node);

    /**
     * Determine default data directory for operating system.
     */
    static QString getDefaultDataDirectory();

Q_SIGNALS:
    void requestCheck();

public Q_SLOTS:
    void setStatus(int status, const QString &message, quint64 bytesAvailable);

private Q_SLOTS:
    void on_dataDirectory_textChanged(const QString &arg1);
    void on_ellipsisButton_clicked();
    void on_dataDirDefault_clicked();
    void on_dataDirCustom_clicked();

private:
    Ui::Intro *ui;
    QThread *thread;
    QMutex mutex;
    bool signalled;
    QString pathToCheck;
    uint64_t m_blockchain_size;
    uint64_t m_chain_state_size;

    void startThread();
    void checkPath(const QString &dataDir);
    QString getPathToCheck();

    friend class FreespaceChecker;
};

#endif // BITCOIN_QT_INTRO_H
