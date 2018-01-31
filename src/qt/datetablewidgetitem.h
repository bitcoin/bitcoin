#ifndef DATE_TABLE_WIDGET_ITEM_H
#define DATE_TABLE_WIDGET_ITEM_H

#include <QTableWidgetItem>
#include <stdint.h>

class DateTableWidgetItem : public QTableWidgetItem
{
private:
    int64_t date;
public:
    explicit DateTableWidgetItem(int64_t date);
    bool operator< ( const QTableWidgetItem & other ) const;
};

#endif // DATE_TABLE_WIDGET_ITEM_H
