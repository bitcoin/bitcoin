// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/receiverequestdialog.h>
#include <qt/forms/ui_receiverequestdialog.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/qrimagewidget.h>
#include <qt/walletmodel.h>

#include <QDialog>
#include <QLayoutItem>
#include <QString>
#include <QTextEdit>

#include <bitcoin-build-config.h> // IWYU pragma: keep

ReceiveRequestDialog::ReceiveRequestDialog(QWidget* parent)
    : QDialog(parent, GUIUtil::dialog_flags),
      ui(new Ui::ReceiveRequestDialog)
{
    ui->setupUi(this);

    while (QLayoutItem * const child = ui->gridLayout->itemAt(1)) {
        if (child == ui->horizontalLayout) break;  // stop at buttons
        ui->gridLayout->removeItem(child);
        auto child_widget = child->widget();
        // NOTE: Unparenting causes isHidden to be unconditionally true, so just make it sizeless and exclude it from the layout
        child_widget->setMaximumSize(0, 0);
        m_info_grid.append(child_widget);
        delete child;
    }
    m_info_widget = new QTextEdit(this);
    m_info_widget->setMinimumSize(0, 50);
    m_info_widget->setFrameShape(QFrame::NoFrame);
    m_info_widget->setFrameShadow(QFrame::Plain);
    m_info_widget->setTabChangesFocus(true);
    m_info_widget->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);
    ui->gridLayout->addWidget(m_info_widget, 1, 0, 1, 2);

    GUIUtil::handleCloseWindowShortcut(this);
}

ReceiveRequestDialog::~ReceiveRequestDialog()
{
    for (auto& widget : m_info_grid) {
        delete widget;
    }
    delete ui;
}

void ReceiveRequestDialog::updateInfoWidget()
{
    QString html;
    html += "<html><font face='verdana, arial, helvetica, sans-serif'>";
    int i = 0;
    for (auto& widget : m_info_grid) {
        ++i;
        QLabel * const label = dynamic_cast<QLabel*>(widget);
        assert(label);
        QString text = label->text();

        if (!label->isHidden()) {
            if (i == 1) {
                html += "<b>" + text + "</b><br>";
            } else if (i % 2 == 0) {
                assert(text.endsWith(":"));
                text.chop(1);
                html += "<b>" + text + "</b>: ";
            } else {
                html += text + "<br>";
            }
        }
    }
    html += "</font></html>";
    m_info_widget->setText(html);
}

void ReceiveRequestDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if (_model)
    {
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &ReceiveRequestDialog::updateDisplayUnit);
        connect(_model->getOptionsModel(), &OptionsModel::fontForMoneyChanged, this, &ReceiveRequestDialog::updateDisplayUnit);
    }

    // update the display unit if necessary
    update();
}

void ReceiveRequestDialog::setInfo(const SendCoinsRecipient &_info)
{
    this->info = _info;
    setWindowTitle(tr("Request payment to %1").arg(info.label.isEmpty() ? info.address : info.label));
    QString uri = GUIUtil::formatBitcoinURI(info);

#ifdef USE_QRCODE
    if (ui->qr_code->setQR(uri, info.address, model->getOptionsModel()->getFontChoiceForQRCodes())) {
        connect(ui->btnSaveAs, &QPushButton::clicked, ui->qr_code, &QRImageWidget::saveImage);
        connect(model->getOptionsModel(), &OptionsModel::fontForQRCodesChanged, this, [&](const OptionsModel::FontChoice& fontchoice){
            ui->qr_code->setQR(uri, info.address, fontchoice);
        });
    } else {
        ui->btnSaveAs->setEnabled(false);
    }
#else
    ui->btnSaveAs->hide();
    ui->qr_code->hide();
#endif

    ui->uri_content->setText("<a href=\"" + uri + "\">" + GUIUtil::HtmlEscape(uri) + "</a>");
    ui->address_content->setText(info.address);

    if (!info.amount) {
        ui->amount_tag->hide();
        ui->amount_content->hide();
    } // Amount is set in updateDisplayUnit() slot.
    updateDisplayUnit();

    if (!info.label.isEmpty()) {
        ui->label_content->setText(info.label);
    } else {
        ui->label_tag->hide();
        ui->label_content->hide();
    }

    if (!info.message.isEmpty()) {
        ui->message_content->setText(info.message);
    } else {
        ui->message_tag->hide();
        ui->message_content->hide();
    }

    if (!model->getWalletName().isEmpty()) {
        ui->wallet_content->setText(model->getWalletName());
    } else {
        ui->wallet_tag->hide();
        ui->wallet_content->hide();
    }

    updateInfoWidget();

    ui->btnVerify->setVisible(model->wallet().hasExternalSigner());

    connect(ui->btnVerify, &QPushButton::clicked, [this] {
        model->displayAddress(info.address.toStdString());
    });
}

void ReceiveRequestDialog::updateDisplayUnit()
{
    if (!model) return;
    ui->amount_content->setText(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getFontForMoney(), model->getOptionsModel()->getDisplayUnit(), info.amount));
    updateInfoWidget();
}

void ReceiveRequestDialog::on_btnCopyURI_clicked()
{
    GUIUtil::setClipboard(GUIUtil::formatBitcoinURI(info));
}

void ReceiveRequestDialog::on_btnCopyAddress_clicked()
{
    GUIUtil::setClipboard(info.address);
}
