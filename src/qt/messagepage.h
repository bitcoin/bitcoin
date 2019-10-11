#ifndef MESSAGEPAGE_H
#define MESSAGEPAGE_H

#include <QWidget>

namespace Ui {
    class MessagePage;
}
class MessageModel;
class PlatformStyle;

QT_BEGIN_NAMESPACE
class QTableView;
class QItemSelection;
class QSortFilterProxyModel;
class QMenu;
class QModelIndex;
QT_END_NAMESPACE


/** Widget that shows a list of sending or receiving addresses.
  */
class MessagePage : public QWidget
{
    Q_OBJECT

public:

    explicit MessagePage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~MessagePage();

    void setModel(MessageModel *model);

public Q_SLOTS:
    void exportClicked();

private:
    Ui::MessagePage *ui;
    MessageModel *model;
    QSortFilterProxyModel *proxyModel;
    QMenu *contextMenu;
    QAction *replyAction;
    QAction *copyFromAddressAction;
    QAction *copyToAddressAction;
    QAction *deleteAction;
    const PlatformStyle *platformStyle;

private Q_SLOTS:
    void on_replyButton_clicked();
    void on_copyFromAddressButton_clicked();
    void on_copyToAddressButton_clicked();
    void on_deleteButton_clicked();
    void on_newmessageButton_clicked();
    void selectionChanged();
    /** Spawn contextual menu (right mouse menu) for address book entry */
    void contextualMenu(const QPoint &point);

Q_SIGNALS:
};

#endif // MESSAGEPAGE_H
