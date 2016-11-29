// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LOOKUPADDRESSDIALOG_H
#define LOOKUPADDRESSDIALOG_H

#include <QDialog>
#include <QLabel>

QT_BEGIN_NAMESPACE
class QContextMenuEvent;
class QImage;
class QMenu;
class QMouseEvent;
class QString;
class QWidget;
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

public Q_SLOTS:
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
    ~LookupAddressDialog();

    void searchAddress();

public Q_SLOTS:
    void searchButtonClicked();

private:
    Ui::LookupAddressDialog *ui;

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // LOOKUPADDRESSDIALOG_H
