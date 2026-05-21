// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_INTRO_H
#define BITCOIN_QT_INTRO_H

#include <qt/freespacechecker.h>

#include <QDialog>
#include <QMutex>
#include <QThread>

static const bool DEFAULT_CHOOSE_DATADIR = false;

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
class Intro : public QDialog, public FreespaceChecker::PathQuery
{
    Q_OBJECT

public:
    explicit Intro(QWidget *parent = nullptr,
                   int64_t blockchain_size_gb = 0, int64_t chain_state_size_gb = 0);
    ~Intro();

    QString getDataDirectory();
    void setDataDirectory(const QString &dataDir);
    int64_t getPruneMiB() const;

    /**
     * Determine data directory. Let the user choose if the current one doesn't exist.
     * Let the user configure additional preferences such as pruning.
     *
     * @returns true if a data directory was selected, false if the user cancelled the selection
     * dialog.
     *
     * @note do NOT call global gArgs.GetDataDirNet() before calling this function, this
     * will cause the wrong path to be cached.
     */
    static bool showIfNeeded(bool& did_show_intro, int64_t& prune_MiB);

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
    bool m_prune_checkbox_is_default{true};
    QThread* thread{nullptr};
    QMutex mutex;
    bool signalled{false};
    QString pathToCheck;
    const int64_t m_blockchain_size_gb;
    const int64_t m_chain_state_size_gb;
    //! Total required space (in GB) depending on user choice (prune or not prune).
    int64_t m_required_space_gb{0};
    uint64_t m_bytes_available{0};
    int64_t m_prune_target_gb;

    void startThread();
    void checkPath(const QString &dataDir);
    QString getPathToCheck() override;
    void UpdatePruneLabels(bool prune_checked);
    void UpdateFreeSpaceLabel();

    friend class FreespaceChecker;
};

#endif // BITCOIN_QT_INTRO_H
