#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QList>

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QListWidget;
class QListWidgetItem;
class QPushButton;
QT_END_NAMESPACE
class OptionsModel;
class OptionsPage;
class MonitoredDataMapper;

/** Preferences dialog. */
class OptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptionsDialog(QWidget *parent=0);

    void setModel(OptionsModel *model);

signals:

public slots:
    /** Change the current page to \a index. */
    void changePage(int index);

private slots:
    void okClicked();
    void cancelClicked();
    void applyClicked();
    void enableApply();
    void disableApply();

private:
    QListWidget *contents_widget;
    QStackedWidget *pages_widget;
    OptionsModel *model;
    MonitoredDataMapper *mapper;
    QPushButton *apply_button;

    QList<OptionsPage*> pages;
};

#endif // OPTIONSDIALOG_H
