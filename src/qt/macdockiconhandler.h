#ifndef MACDOCKICONHANDLER_H
#define MACDOCKICONHANDLER_H

#include <QObject>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QMenu;
class QIcon;
class QWidget;
QT_END_NAMESPACE

#ifdef __OBJC__
@class DockIconClickEventHandler;
#else
class DockIconClickEventHandler;
#endif

/** Macintosh-specific dock icon handler.
 */
class MacDockIconHandler : public QObject
{
    Q_OBJECT

public:
    ~MacDockIconHandler();

    QMenu *dockMenu();
    void setIcon(const QIcon &icon);
    void setMainWindow(QMainWindow *window);
    static MacDockIconHandler *instance();

    void handleDockIconClickEvent();

signals:
    void dockIconClicked();

private:
    MacDockIconHandler();

    DockIconClickEventHandler *m_dockIconClickEventHandler;
    QWidget *m_dummyWidget;
    QMenu *m_dockMenu;
    QMainWindow *mainWindow;
};

#endif // MACDOCKICONCLICKHANDLER_H
