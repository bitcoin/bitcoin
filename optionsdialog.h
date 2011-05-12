#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QStackedWidget>
#include <QListWidget>

class OptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionsDialog(QWidget *parent = 0);

signals:

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
private:
    QListWidget *contents_widget;
    QStackedWidget *pages_widget;

    void setupMainPage();
};

#endif // OPTIONSDIALOG_H
