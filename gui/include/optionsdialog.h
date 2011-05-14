#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

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
