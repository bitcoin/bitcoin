// Copyright (c) 2011-2015 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "unlimiteddialog.h"
#include "unlimitedmodel.h"

#include "bitcoinunits.h"
#include "guiutil.h"

#include "main.h" // for DEFAULT_SCRIPTCHECK_THREADS and MAX_SCRIPTCHECK_THREADS
#include "netbase.h"
#include "net.h"  // for access to the network traffic shapers
#include "txdb.h" // for -dbcache defaults

#ifdef ENABLE_WALLET
#include "wallet/wallet.h" // for CWallet::minTxFee
#endif

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

#include <QDataWidgetMapper>
#include <QDir>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QTimer>


UnlimitedDialog::UnlimitedDialog(QWidget* parent,UnlimitedModel* mdl):
  QDialog(parent),
  model(mdl)
{
  ui.setupUi(this);
  //ui.maxMinedBlock->setRange(0, 0xffffffffUL);
  mapper.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
  mapper.setOrientation(Qt::Vertical);
  setMapper();

  connect(ui.okButton, SIGNAL(clicked(bool)), this, SLOT(on_okButton_clicked()));

  ui.miningMaxBlock->setText(QString(boost::lexical_cast<std::string>(model->maxGeneratedBlock).c_str()));
}


UnlimitedDialog::~UnlimitedDialog()
{
}

#if 0
void UnlimitedDialog::setModel(UnlimitedModel* mdl)
{
    model = mdl;

    if (model)
      {
        /* check if client restart is needed and show persistent message */
        //if (model->isRestartRequired())
        //    showRestartWarning(true);

        QString strLabel; //  = model->getOverriddenByCommandLine();
        if (strLabel.isEmpty())
            strLabel = tr("none");
        ui.overriddenByCommandLineLabel->setText(strLabel);

        mapper.setModel(model);
        setMapper();
        mapper.toFirst();
    }
#endif    

void UnlimitedDialog::setMapper()
{
    mapper.setModel(model);
    /* Network */

    mapper.addMapping(ui.sendShapingEnable, UnlimitedModel::UseSendShaping);
    mapper.addMapping(ui.sendBurstEdit, UnlimitedModel::SendBurst);
    mapper.addMapping(ui.sendAveEdit, UnlimitedModel::SendAve);
    mapper.addMapping(ui.recvShapingEnable, UnlimitedModel::UseReceiveShaping);
    mapper.addMapping(ui.recvBurstEdit, UnlimitedModel::ReceiveBurst);
    mapper.addMapping(ui.recvAveEdit, UnlimitedModel::ReceiveAve);

    mapper.addMapping(ui.miningMaxBlock,UnlimitedModel::MaxGeneratedBlock);
}
    

void UnlimitedDialog::on_okButton_clicked()
{
    mapper.submit();
    accept();
}

void UnlimitedDialog::on_cancelButton_clicked()
{
    reject();
}
