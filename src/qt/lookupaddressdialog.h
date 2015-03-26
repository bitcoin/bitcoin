// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOOKUPADDRESSDIALOG_H
#define LOOKUPADDRESSDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>
#include <QImage>
#include <QLabel>
class OptionsModel;

QT_BEGIN_NAMESPACE
class QUrl;
class QMenu;
QT_END_NAMESPACE

namespace Ui {
    class LookupAddressDialog;
}

/* Label widget for QR code. This image can be dragged, dropped, copied and saved
 * to disk.
 */
class MPQRImageWidget : public QLabel
{
    Q_OBJECT

public:
    explicit MPQRImageWidget(QWidget *parent = 0);
    QImage exportImage();

public slots:
    void saveImage();
    void copyImage();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);

private:
    QMenu *contextMenu;
};

/** Dialog for looking up Master Protocol address */
class LookupAddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LookupAddressDialog(QWidget *parent = 0);
    void searchAddress();
    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

public slots:
    void searchButtonClicked();

private:
    Ui::LookupAddressDialog *ui;
    WalletModel *model;

//private slots:

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // LOOKUPADDRESSDIALOG_H
