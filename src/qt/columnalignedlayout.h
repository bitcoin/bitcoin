#ifndef COLUMNALIGNEDLAYOUT_H
#define COLUMNALIGNEDLAYOUT_H

#include <QHBoxLayout>

class QHeaderView;

class ColumnAlignedLayout : public QHBoxLayout
{
    Q_OBJECT
public:
    ColumnAlignedLayout();
    explicit ColumnAlignedLayout(QWidget *parent);
    void setTableColumnsToTrack(QHeaderView *view) { headerView = view; }

private:
    void setGeometry(const QRect &r);
    QHeaderView *headerView;
};

#endif // COLUMNALIGNEDLAYOUT_H