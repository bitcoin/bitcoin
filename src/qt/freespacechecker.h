// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_FREESPACECHECKER_H
#define BITCOIN_QT_FREESPACECHECKER_H

#include <QObject>
#include <QString>
#include <QtGlobal>

/* Check free space asynchronously to prevent hanging the UI thread.

   Up to one request to check a path is in flight to this thread; when the check()
   function runs, the current path is requested from the associated Intro object.
   The reply is sent back through a signal.

   This ensures that no queue of checking requests is built up while the user is
   still entering the path, and that always the most recently entered path is checked as
   soon as the thread becomes available.
*/
class FreespaceChecker : public QObject
{
    Q_OBJECT

public:
    class PathQuery
    {
    public:
        virtual QString getPathToCheck() = 0;
    };

    explicit FreespaceChecker(PathQuery* intro) : intro{intro} {}

    enum Status {
        ST_OK,
        ST_ERROR
    };

public Q_SLOTS:
    void check();

Q_SIGNALS:
    void reply(int status, const QString &message, quint64 available);

private:
    PathQuery* intro;
};

#endif // BITCOIN_QT_FREESPACECHECKER_H
