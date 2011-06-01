#include "optionsdialog.h"
#include "optionsmodel.h"
#include "mainoptionspage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QStackedWidget>
#include <QDataWidgetMapper>
#include <QDebug>

OptionsDialog::OptionsDialog(QWidget *parent):
    QDialog(parent), contents_widget(0), pages_widget(0),
    main_options_page(0), model(0)
{
    contents_widget = new QListWidget();
    contents_widget->setMaximumWidth(128);

    pages_widget = new QStackedWidget();
    pages_widget->setMinimumWidth(300);

    QListWidgetItem *item_main = new QListWidgetItem(tr("Main"));
    contents_widget->addItem(item_main);
    main_options_page = new MainOptionsPage(this);
    pages_widget->addWidget(main_options_page);

    contents_widget->setCurrentRow(0);

    QHBoxLayout *main_layout = new QHBoxLayout();
    main_layout->addWidget(contents_widget);
    main_layout->addWidget(pages_widget, 1);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(main_layout);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch(1);
    QPushButton *ok_button = new QPushButton(tr("OK"));
    buttons->addWidget(ok_button);
    QPushButton *cancel_button = new QPushButton(tr("Cancel"));
    buttons->addWidget(cancel_button);
    apply_button = new QPushButton(tr("Apply"));
    apply_button->setEnabled(false);
    buttons->addWidget(apply_button);

    layout->addLayout(buttons);

    setLayout(layout);
    setWindowTitle(tr("Options"));

    /* Widget-to-option mapper */
    mapper = new QDataWidgetMapper();
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);
    connect(mapper->itemDelegate(), SIGNAL(commitData(QWidget*)), this, SLOT(enableApply()));

    /* Event bindings */
    connect(ok_button, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel_button, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(apply_button, SIGNAL(clicked()), this, SLOT(applyClicked()));
}

void OptionsDialog::setModel(OptionsModel *model)
{
    this->model = model;

    mapper->setModel(model);
    main_options_page->setMapper(mapper);

    mapper->toFirst();
}

void OptionsDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    if(current)
    {
        pages_widget->setCurrentIndex(contents_widget->row(current));
    }
}

void OptionsDialog::okClicked()
{
    mapper->submit();
    accept();
}

void OptionsDialog::cancelClicked()
{
    reject();
}

void OptionsDialog::applyClicked()
{
    mapper->submit();
    apply_button->setEnabled(false);
}

void OptionsDialog::enableApply()
{
    apply_button->setEnabled(true);
}
