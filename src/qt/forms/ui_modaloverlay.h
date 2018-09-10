/********************************************************************************
** Form generated from reading UI file 'modaloverlay.ui'
**
** Created by: Qt User Interface Compiler version 5.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MODALOVERLAY_H
#define UI_MODALOVERLAY_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "modaloverlay.h"

QT_BEGIN_NAMESPACE

class Ui_ModalOverlay
{
public:
    QVBoxLayout *verticalLayout;
    QWidget *bgWidget;
    QVBoxLayout *verticalLayoutMain;
    QWidget *contentWidget;
    QVBoxLayout *verticalLayoutSub;
    QHBoxLayout *horizontalLayoutIconText;
    QVBoxLayout *verticalLayoutIcon;
    QPushButton *warningIcon;
    QSpacerItem *verticalSpacerWarningIcon;
    QVBoxLayout *verticalLayoutInfoText;
    QLabel *infoText;
    QLabel *infoTextStrong;
    QSpacerItem *verticalSpacerInTextSpace;
    QSpacerItem *verticalSpacerAfterText;
    QFormLayout *formLayout;
    QLabel *labelNumberOfBlocksLeft;
    QLabel *numberOfBlocksLeft;
    QLabel *labelLastBlockTime;
    QLabel *newestBlockDate;
    QLabel *labelSyncDone;
    QHBoxLayout *horizontalLayoutSync;
    QLabel *percentageProgress;
    QProgressBar *progressBar;
    QLabel *labelProgressIncrease;
    QLabel *progressIncreasePerH;
    QLabel *labelEstimatedTimeLeft;
    QLabel *expectedTimeLeft;
    QHBoxLayout *horizontalLayoutButtons;
    QSpacerItem *horizontalSpacer;
    QPushButton *closeButton;

    void setupUi(ModalOverlay *ModalOverlay)
    {
        if (ModalOverlay->objectName().isEmpty())
            ModalOverlay->setObjectName(QStringLiteral("ModalOverlay"));
        ModalOverlay->resize(640, 385);
        verticalLayout = new QVBoxLayout(ModalOverlay);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        bgWidget = new QWidget(ModalOverlay);
        bgWidget->setObjectName(QStringLiteral("bgWidget"));
        bgWidget->setStyleSheet(QStringLiteral("#bgWidget { background: rgba(0,0,0,220); }"));
        verticalLayoutMain = new QVBoxLayout(bgWidget);
        verticalLayoutMain->setObjectName(QStringLiteral("verticalLayoutMain"));
        verticalLayoutMain->setContentsMargins(60, 60, 60, 60);
        contentWidget = new QWidget(bgWidget);
        contentWidget->setObjectName(QStringLiteral("contentWidget"));
        contentWidget->setStyleSheet(QLatin1String("#contentWidget { background: rgba(255,255,255,240); border-radius: 6px; }\n"
"\n"
"QLabel { color: rgb(40,40,40);  }"));
        verticalLayoutSub = new QVBoxLayout(contentWidget);
        verticalLayoutSub->setSpacing(0);
        verticalLayoutSub->setObjectName(QStringLiteral("verticalLayoutSub"));
        verticalLayoutSub->setContentsMargins(10, 10, 10, 10);
        horizontalLayoutIconText = new QHBoxLayout();
        horizontalLayoutIconText->setObjectName(QStringLiteral("horizontalLayoutIconText"));
        horizontalLayoutIconText->setContentsMargins(-1, 20, -1, -1);
        verticalLayoutIcon = new QVBoxLayout();
        verticalLayoutIcon->setObjectName(QStringLiteral("verticalLayoutIcon"));
        verticalLayoutIcon->setContentsMargins(0, -1, -1, -1);
        warningIcon = new QPushButton(contentWidget);
        warningIcon->setObjectName(QStringLiteral("warningIcon"));
        warningIcon->setEnabled(false);
        QIcon icon;
        icon.addFile(QStringLiteral(":/icons/warning"), QSize(), QIcon::Normal, QIcon::Off);
        icon.addFile(QStringLiteral(":/icons/warning"), QSize(), QIcon::Disabled, QIcon::Off);
        warningIcon->setIcon(icon);
        warningIcon->setIconSize(QSize(48, 48));
        warningIcon->setFlat(true);

        verticalLayoutIcon->addWidget(warningIcon);

        verticalSpacerWarningIcon = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayoutIcon->addItem(verticalSpacerWarningIcon);


        horizontalLayoutIconText->addLayout(verticalLayoutIcon);

        verticalLayoutInfoText = new QVBoxLayout();
        verticalLayoutInfoText->setObjectName(QStringLiteral("verticalLayoutInfoText"));
        verticalLayoutInfoText->setContentsMargins(0, 0, -1, -1);
        infoText = new QLabel(contentWidget);
        infoText->setObjectName(QStringLiteral("infoText"));
        infoText->setTextFormat(Qt::RichText);
        infoText->setWordWrap(true);

        verticalLayoutInfoText->addWidget(infoText);

        infoTextStrong = new QLabel(contentWidget);
        infoTextStrong->setObjectName(QStringLiteral("infoTextStrong"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        infoTextStrong->setFont(font);
        infoTextStrong->setTextFormat(Qt::RichText);
        infoTextStrong->setWordWrap(true);

        verticalLayoutInfoText->addWidget(infoTextStrong);

        verticalSpacerInTextSpace = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayoutInfoText->addItem(verticalSpacerInTextSpace);


        horizontalLayoutIconText->addLayout(verticalLayoutInfoText);

        horizontalLayoutIconText->setStretch(1, 1);

        verticalLayoutSub->addLayout(horizontalLayoutIconText);

        verticalSpacerAfterText = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayoutSub->addItem(verticalSpacerAfterText);

        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
        formLayout->setHorizontalSpacing(6);
        formLayout->setVerticalSpacing(6);
        formLayout->setContentsMargins(-1, 10, -1, -1);
        labelNumberOfBlocksLeft = new QLabel(contentWidget);
        labelNumberOfBlocksLeft->setObjectName(QStringLiteral("labelNumberOfBlocksLeft"));
        labelNumberOfBlocksLeft->setFont(font);

        formLayout->setWidget(0, QFormLayout::LabelRole, labelNumberOfBlocksLeft);

        numberOfBlocksLeft = new QLabel(contentWidget);
        numberOfBlocksLeft->setObjectName(QStringLiteral("numberOfBlocksLeft"));

        formLayout->setWidget(0, QFormLayout::FieldRole, numberOfBlocksLeft);

        labelLastBlockTime = new QLabel(contentWidget);
        labelLastBlockTime->setObjectName(QStringLiteral("labelLastBlockTime"));
        labelLastBlockTime->setFont(font);

        formLayout->setWidget(1, QFormLayout::LabelRole, labelLastBlockTime);

        newestBlockDate = new QLabel(contentWidget);
        newestBlockDate->setObjectName(QStringLiteral("newestBlockDate"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(newestBlockDate->sizePolicy().hasHeightForWidth());
        newestBlockDate->setSizePolicy(sizePolicy);

        formLayout->setWidget(1, QFormLayout::FieldRole, newestBlockDate);

        labelSyncDone = new QLabel(contentWidget);
        labelSyncDone->setObjectName(QStringLiteral("labelSyncDone"));
        labelSyncDone->setFont(font);

        formLayout->setWidget(2, QFormLayout::LabelRole, labelSyncDone);

        horizontalLayoutSync = new QHBoxLayout();
        horizontalLayoutSync->setObjectName(QStringLiteral("horizontalLayoutSync"));
        percentageProgress = new QLabel(contentWidget);
        percentageProgress->setObjectName(QStringLiteral("percentageProgress"));
        percentageProgress->setText(QStringLiteral("~"));

        horizontalLayoutSync->addWidget(percentageProgress);

        progressBar = new QProgressBar(contentWidget);
        progressBar->setObjectName(QStringLiteral("progressBar"));
        progressBar->setValue(24);

        horizontalLayoutSync->addWidget(progressBar);

        horizontalLayoutSync->setStretch(1, 1);

        formLayout->setLayout(2, QFormLayout::FieldRole, horizontalLayoutSync);

        labelProgressIncrease = new QLabel(contentWidget);
        labelProgressIncrease->setObjectName(QStringLiteral("labelProgressIncrease"));
        labelProgressIncrease->setFont(font);

        formLayout->setWidget(4, QFormLayout::LabelRole, labelProgressIncrease);

        progressIncreasePerH = new QLabel(contentWidget);
        progressIncreasePerH->setObjectName(QStringLiteral("progressIncreasePerH"));

        formLayout->setWidget(4, QFormLayout::FieldRole, progressIncreasePerH);

        labelEstimatedTimeLeft = new QLabel(contentWidget);
        labelEstimatedTimeLeft->setObjectName(QStringLiteral("labelEstimatedTimeLeft"));
        labelEstimatedTimeLeft->setFont(font);

        formLayout->setWidget(5, QFormLayout::LabelRole, labelEstimatedTimeLeft);

        expectedTimeLeft = new QLabel(contentWidget);
        expectedTimeLeft->setObjectName(QStringLiteral("expectedTimeLeft"));

        formLayout->setWidget(5, QFormLayout::FieldRole, expectedTimeLeft);


        verticalLayoutSub->addLayout(formLayout);

        horizontalLayoutButtons = new QHBoxLayout();
        horizontalLayoutButtons->setObjectName(QStringLiteral("horizontalLayoutButtons"));
        horizontalLayoutButtons->setContentsMargins(10, 10, -1, -1);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayoutButtons->addItem(horizontalSpacer);

        closeButton = new QPushButton(contentWidget);
        closeButton->setObjectName(QStringLiteral("closeButton"));

        horizontalLayoutButtons->addWidget(closeButton);


        verticalLayoutSub->addLayout(horizontalLayoutButtons);

        verticalLayoutSub->setStretch(0, 1);

        verticalLayoutMain->addWidget(contentWidget);

        verticalLayoutMain->setStretch(0, 1);

        verticalLayout->addWidget(bgWidget);


        retranslateUi(ModalOverlay);

        QMetaObject::connectSlotsByName(ModalOverlay);
    } // setupUi

    void retranslateUi(ModalOverlay *ModalOverlay)
    {
        ModalOverlay->setWindowTitle(QApplication::translate("ModalOverlay", "Form", nullptr));
        warningIcon->setText(QString());
        infoText->setText(QApplication::translate("ModalOverlay", "Recent transactions may not yet be visible, and therefore your wallet's balance might be incorrect. This information will be correct once your wallet has finished synchronizing with the MicroBitcoin network, as detailed below.", nullptr));
        infoTextStrong->setText(QApplication::translate("ModalOverlay", "Attempting to spend microbitcoins that are affected by not-yet-displayed transactions will not be accepted by the network.", nullptr));
        labelNumberOfBlocksLeft->setText(QApplication::translate("ModalOverlay", "Number of blocks left", nullptr));
        numberOfBlocksLeft->setText(QApplication::translate("ModalOverlay", "Unknown...", nullptr));
        labelLastBlockTime->setText(QApplication::translate("ModalOverlay", "Last block time", nullptr));
        newestBlockDate->setText(QApplication::translate("ModalOverlay", "Unknown...", nullptr));
        labelSyncDone->setText(QApplication::translate("ModalOverlay", "Progress", nullptr));
        progressBar->setFormat(QString());
        labelProgressIncrease->setText(QApplication::translate("ModalOverlay", "Progress increase per hour", nullptr));
        progressIncreasePerH->setText(QApplication::translate("ModalOverlay", "calculating...", nullptr));
        labelEstimatedTimeLeft->setText(QApplication::translate("ModalOverlay", "Estimated time left until synced", nullptr));
        expectedTimeLeft->setText(QApplication::translate("ModalOverlay", "calculating...", nullptr));
        closeButton->setText(QApplication::translate("ModalOverlay", "Hide", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ModalOverlay: public Ui_ModalOverlay {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MODALOVERLAY_H
