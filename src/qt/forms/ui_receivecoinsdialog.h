/********************************************************************************
** Form generated from reading UI file 'receivecoinsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RECEIVECOINSDIALOG_H
#define UI_RECEIVECOINSDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "bitcoinamountfield.h"

QT_BEGIN_NAMESPACE

class Ui_ReceiveCoinsDialog
{
public:
    QVBoxLayout *verticalLayout;
    QFrame *frame2;
    QVBoxLayout *verticalLayout_3;
    QGridLayout *gridLayout;
    QCheckBox *reuseAddress;
    QLabel *label_4;
    QLabel *label_3;
    QLineEdit *reqLabel;
    QLineEdit *reqMessage;
    QLabel *label_5;
    QLabel *label_2;
    QLabel *label;
    BitcoinAmountField *reqAmount;
    QHBoxLayout *horizontalLayout;
    QPushButton *receiveButton;
    QPushButton *clearButton;
    QSpacerItem *horizontalSpacer;
    QLabel *label_7;
    QSpacerItem *verticalSpacer_2;
    QFrame *frame;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_6;
    QTableView *recentRequestsView;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *showRequestButton;
    QPushButton *removeRequestButton;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QWidget *ReceiveCoinsDialog)
    {
        if (ReceiveCoinsDialog->objectName().isEmpty())
            ReceiveCoinsDialog->setObjectName(QStringLiteral("ReceiveCoinsDialog"));
        ReceiveCoinsDialog->resize(776, 364);
        verticalLayout = new QVBoxLayout(ReceiveCoinsDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        frame2 = new QFrame(ReceiveCoinsDialog);
        frame2->setObjectName(QStringLiteral("frame2"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(frame2->sizePolicy().hasHeightForWidth());
        frame2->setSizePolicy(sizePolicy);
        frame2->setFrameShape(QFrame::StyledPanel);
        frame2->setFrameShadow(QFrame::Sunken);
        verticalLayout_3 = new QVBoxLayout(frame2);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        reuseAddress = new QCheckBox(frame2);
        reuseAddress->setObjectName(QStringLiteral("reuseAddress"));

        gridLayout->addWidget(reuseAddress, 7, 2, 1, 1);

        label_4 = new QLabel(frame2);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 7, 0, 1, 1);

        label_3 = new QLabel(frame2);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_3, 6, 0, 1, 1);

        reqLabel = new QLineEdit(frame2);
        reqLabel->setObjectName(QStringLiteral("reqLabel"));

        gridLayout->addWidget(reqLabel, 4, 2, 1, 1);

        reqMessage = new QLineEdit(frame2);
        reqMessage->setObjectName(QStringLiteral("reqMessage"));

        gridLayout->addWidget(reqMessage, 6, 2, 1, 1);

        label_5 = new QLabel(frame2);
        label_5->setObjectName(QStringLiteral("label_5"));

        gridLayout->addWidget(label_5, 2, 2, 1, 1);

        label_2 = new QLabel(frame2);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label_2, 4, 0, 1, 1);

        label = new QLabel(frame2);
        label->setObjectName(QStringLiteral("label"));
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(label, 5, 0, 1, 1);

        reqAmount = new BitcoinAmountField(frame2);
        reqAmount->setObjectName(QStringLiteral("reqAmount"));
        reqAmount->setMinimumSize(QSize(80, 0));

        gridLayout->addWidget(reqAmount, 5, 2, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        receiveButton = new QPushButton(frame2);
        receiveButton->setObjectName(QStringLiteral("receiveButton"));
        receiveButton->setMinimumSize(QSize(150, 0));
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/receiving_addresses"), QSize(), QIcon::Normal, QIcon::Off);
        receiveButton->setIcon(icon);

        horizontalLayout->addWidget(receiveButton);

        clearButton = new QPushButton(frame2);
        clearButton->setObjectName(QStringLiteral("clearButton"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(clearButton->sizePolicy().hasHeightForWidth());
        clearButton->setSizePolicy(sizePolicy1);
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/remove"), QSize(), QIcon::Normal, QIcon::Off);
        clearButton->setIcon(icon1);
        clearButton->setAutoDefault(false);

        horizontalLayout->addWidget(clearButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        gridLayout->addLayout(horizontalLayout, 8, 2, 1, 1);

        label_7 = new QLabel(frame2);
        label_7->setObjectName(QStringLiteral("label_7"));

        gridLayout->addWidget(label_7, 8, 0, 1, 1);


        verticalLayout_3->addLayout(gridLayout);


        verticalLayout->addWidget(frame2);

        verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        frame = new QFrame(ReceiveCoinsDialog);
        frame->setObjectName(QStringLiteral("frame"));
        sizePolicy.setHeightForWidth(frame->sizePolicy().hasHeightForWidth());
        frame->setSizePolicy(sizePolicy);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        verticalLayout_2 = new QVBoxLayout(frame);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        label_6 = new QLabel(frame);
        label_6->setObjectName(QStringLiteral("label_6"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        label_6->setFont(font);

        verticalLayout_2->addWidget(label_6);

        recentRequestsView = new QTableView(frame);
        recentRequestsView->setObjectName(QStringLiteral("recentRequestsView"));
        recentRequestsView->setContextMenuPolicy(Qt::CustomContextMenu);
        recentRequestsView->setTabKeyNavigation(false);
        recentRequestsView->setSortingEnabled(true);

        verticalLayout_2->addWidget(recentRequestsView);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        showRequestButton = new QPushButton(frame);
        showRequestButton->setObjectName(QStringLiteral("showRequestButton"));
        showRequestButton->setEnabled(false);
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/icons/edit"), QSize(), QIcon::Normal, QIcon::Off);
        showRequestButton->setIcon(icon2);
        showRequestButton->setAutoDefault(false);

        horizontalLayout_2->addWidget(showRequestButton);

        removeRequestButton = new QPushButton(frame);
        removeRequestButton->setObjectName(QStringLiteral("removeRequestButton"));
        removeRequestButton->setEnabled(false);
        removeRequestButton->setIcon(icon1);
        removeRequestButton->setAutoDefault(false);

        horizontalLayout_2->addWidget(removeRequestButton);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);


        verticalLayout_2->addLayout(horizontalLayout_2);


        verticalLayout->addWidget(frame);

        verticalLayout->setStretch(2, 1);
#ifndef QT_NO_SHORTCUT
        label_3->setBuddy(reqMessage);
        label_2->setBuddy(reqLabel);
        label->setBuddy(reqAmount);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(reqLabel, reqAmount);
        QWidget::setTabOrder(reqAmount, reqMessage);
        QWidget::setTabOrder(reqMessage, reuseAddress);
        QWidget::setTabOrder(reuseAddress, receiveButton);
        QWidget::setTabOrder(receiveButton, clearButton);
        QWidget::setTabOrder(clearButton, recentRequestsView);
        QWidget::setTabOrder(recentRequestsView, showRequestButton);
        QWidget::setTabOrder(showRequestButton, removeRequestButton);

        retranslateUi(ReceiveCoinsDialog);

        QMetaObject::connectSlotsByName(ReceiveCoinsDialog);
    } // setupUi

    void retranslateUi(QWidget *ReceiveCoinsDialog)
    {
#ifndef QT_NO_TOOLTIP
        reuseAddress->setToolTip(QApplication::translate("ReceiveCoinsDialog", "Reuse one of the previously used receiving addresses. Reusing addresses has security and privacy issues. Do not use this unless re-generating a payment request made before.", nullptr));
#endif // QT_NO_TOOLTIP
        reuseAddress->setText(QApplication::translate("ReceiveCoinsDialog", "R&euse an existing receiving address (not recommended)", nullptr));
        label_4->setText(QString());
#ifndef QT_NO_TOOLTIP
        label_3->setToolTip(QApplication::translate("ReceiveCoinsDialog", "An optional message to attach to the payment request, which will be displayed when the request is opened. Note: The message will not be sent with the payment over the MicroBitcoin network.", nullptr));
#endif // QT_NO_TOOLTIP
        label_3->setText(QApplication::translate("ReceiveCoinsDialog", "&Message:", nullptr));
#ifndef QT_NO_TOOLTIP
        reqLabel->setToolTip(QApplication::translate("ReceiveCoinsDialog", "An optional label to associate with the new receiving address.", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        reqMessage->setToolTip(QApplication::translate("ReceiveCoinsDialog", "An optional message to attach to the payment request, which will be displayed when the request is opened. Note: The message will not be sent with the payment over the MicroBitcoin network.", nullptr));
#endif // QT_NO_TOOLTIP
        label_5->setText(QApplication::translate("ReceiveCoinsDialog", "Use this form to request payments. All fields are <b>optional</b>.", nullptr));
#ifndef QT_NO_TOOLTIP
        label_2->setToolTip(QApplication::translate("ReceiveCoinsDialog", "An optional label to associate with the new receiving address.", nullptr));
#endif // QT_NO_TOOLTIP
        label_2->setText(QApplication::translate("ReceiveCoinsDialog", "&Label:", nullptr));
#ifndef QT_NO_TOOLTIP
        label->setToolTip(QApplication::translate("ReceiveCoinsDialog", "An optional amount to request. Leave this empty or zero to not request a specific amount.", nullptr));
#endif // QT_NO_TOOLTIP
        label->setText(QApplication::translate("ReceiveCoinsDialog", "&Amount:", nullptr));
#ifndef QT_NO_TOOLTIP
        reqAmount->setToolTip(QApplication::translate("ReceiveCoinsDialog", "An optional amount to request. Leave this empty or zero to not request a specific amount.", nullptr));
#endif // QT_NO_TOOLTIP
        receiveButton->setText(QApplication::translate("ReceiveCoinsDialog", "&Request payment", nullptr));
#ifndef QT_NO_TOOLTIP
        clearButton->setToolTip(QApplication::translate("ReceiveCoinsDialog", "Clear all fields of the form.", nullptr));
#endif // QT_NO_TOOLTIP
        clearButton->setText(QApplication::translate("ReceiveCoinsDialog", "Clear", nullptr));
        label_7->setText(QString());
        label_6->setText(QApplication::translate("ReceiveCoinsDialog", "Requested payments history", nullptr));
#ifndef QT_NO_TOOLTIP
        showRequestButton->setToolTip(QApplication::translate("ReceiveCoinsDialog", "Show the selected request (does the same as double clicking an entry)", nullptr));
#endif // QT_NO_TOOLTIP
        showRequestButton->setText(QApplication::translate("ReceiveCoinsDialog", "Show", nullptr));
#ifndef QT_NO_TOOLTIP
        removeRequestButton->setToolTip(QApplication::translate("ReceiveCoinsDialog", "Remove the selected entries from the list", nullptr));
#endif // QT_NO_TOOLTIP
        removeRequestButton->setText(QApplication::translate("ReceiveCoinsDialog", "Remove", nullptr));
        Q_UNUSED(ReceiveCoinsDialog);
    } // retranslateUi

};

namespace Ui {
    class ReceiveCoinsDialog: public Ui_ReceiveCoinsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RECEIVECOINSDIALOG_H
