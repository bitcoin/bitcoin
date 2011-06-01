#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QListWidget;
class QListWidgetItem;
class QDataWidgetMapper;
class QPushButton;
QT_END_NAMESPACE
class OptionsModel;
class MainOptionsPage;

class OptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionsDialog(QWidget *parent=0);

    void setModel(OptionsModel *model);

signals:

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
private slots:
    void okClicked();
    void cancelClicked();
    void applyClicked();
    void enableApply();
private:
    QListWidget *contents_widget;
    QStackedWidget *pages_widget;
    MainOptionsPage *main_options_page;
    OptionsModel *model;
    QDataWidgetMapper *mapper;
    QPushButton *apply_button;

    void setupMainPage();
};

#endif // OPTIONSDIALOG_H
