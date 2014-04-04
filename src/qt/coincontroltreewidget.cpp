// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coincontroltreewidget.h"
#include "coincontroldialog.h"

CoinControlTreeWidget::CoinControlTreeWidget(QWidget *parent) :
    QTreeWidget(parent)
{

}

void CoinControlTreeWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) // press spacebar -> select checkbox
    {
        event->ignore();
        int COLUMN_CHECKBOX = 0;
        this->currentItem()->setCheckState(COLUMN_CHECKBOX, ((this->currentItem()->checkState(COLUMN_CHECKBOX) == Qt::Checked) ? Qt::Unchecked : Qt::Checked));
    }
    else if (event->key() == Qt::Key_Escape) // press esc -> close dialog
    {
        event->ignore();
        CoinControlDialog *coinControlDialog = (CoinControlDialog*)this->parentWidget();
        coinControlDialog->done(QDialog::Accepted);
    }
    else
    {
        this->QTreeWidget::keyPressEvent(event);
    }
}