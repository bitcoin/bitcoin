#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

namespace Ui {
    class OverviewPage;
}

class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

public slots:
    void setBalance(qint64 balance);
    void setNumTransactions(int count);

private:
    Ui::OverviewPage *ui;

};

#endif // OVERVIEWPAGE_H
