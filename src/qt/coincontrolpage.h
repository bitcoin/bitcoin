#ifndef COINCONTROLPAGE_H
#define COINCONTROLPAGE_H

class BitcoinGUI;

#include <QWidget>
#include <QTableWidget>

class CoinControlPage : public QWidget {
  Q_OBJECT

public:
  CoinControlPage(QWidget *parent);
  void UpdateTable();
  std::string selectedAddresses();
  void clearSelection();

private:
  QTableWidget *table;
  BitcoinGUI *gui;

private slots:
  void sendFromSelectedAddress(QTableWidgetItem *item);
};

#endif
