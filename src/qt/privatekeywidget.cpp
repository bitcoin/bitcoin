#include "privatekeywidget.h"

PrivateKeyWidget::PrivateKeyWidget(QString privKey, QWidget *parent) 
    : QWidget(parent)
{
    layout = new QHBoxLayout();
    layout->setMargin(0);

    lineEdit = new QLineEdit(privKey);
    lineEdit->setEchoMode(QLineEdit::Password);
    lineEdit->setReadOnly(true);

    show = new QPushButton();
    copy = new QPushButton();
    show->setMinimumWidth(30);
    copy->setMinimumWidth(30);
    show->setCheckable(true);
    show->setToolTip("Show");
    copy->setToolTip("Copy To Clipboard");

    show->setIcon(QIcon(":/icons/hide"));
    show->setIconSize(QSize(20, 20));
    show->setStyleSheet("border: none;");

    copy->setIcon(QIcon(":/icons/copy"));
    copy->setIconSize(QSize(20, 20));
    copy->setStyleSheet("border: none;");

    this->setLayout(layout);
    layout->addWidget(lineEdit);
    layout->addWidget(show);
    layout->addWidget(copy);

    connect(show, SIGNAL (toggled(bool)), this, SLOT (showClicked(bool)));
    connect(copy, SIGNAL (clicked()), this, SLOT (copyClicked()));
}

void PrivateKeyWidget::showClicked(bool show_hide)
{
    if (show_hide)
    {
        show->setIcon(QIcon(":/icons/show"));
        lineEdit->setEchoMode(QLineEdit::Normal);
        lineEdit->setReadOnly(false);
        show->setToolTip("Hide");
    } else {
        show->setIcon(QIcon(":/icons/hide"));
        lineEdit->setEchoMode(QLineEdit::Password);
        lineEdit->setReadOnly(true);
        show->setToolTip("Show");
    }
}

void PrivateKeyWidget::copyClicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(lineEdit->text());
    QToolTip::showText(QCursor::pos(), "Copied!");
}
