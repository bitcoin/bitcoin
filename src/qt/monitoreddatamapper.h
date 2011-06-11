#ifndef MONITOREDDATAMAPPER_H
#define MONITOREDDATAMAPPER_H

#include <QDataWidgetMapper>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

/* Data <-> Widget mapper that watches for changes,
   to be able to notify when 'dirty' (for example, to
   enable a commit/apply button).
 */
class MonitoredDataMapper : public QDataWidgetMapper
{
    Q_OBJECT
public:
    explicit MonitoredDataMapper(QObject *parent=0);

    void addMapping(QWidget *widget, int section);
    void addMapping(QWidget *widget, int section, const QByteArray &propertyName);
private:
    void addChangeMonitor(QWidget *widget);

signals:
    void viewModified();

};



#endif // MONITOREDDATAMAPPER_H
