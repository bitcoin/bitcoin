// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_HIGHLIGHTLABELWIDGET_H
#define BITCOIN_QT_HIGHLIGHTLABELWIDGET_H

#include <amount.h>

#include <QLabel>
#include <QObject>

QT_BEGIN_NAMESPACE
class QFocusEvent;
class QString;
class QWidget;
QT_END_NAMESPACE

/**
 * Widget for displaying bitcoin amounts with privacy facilities
 */
class HighlightLabelWidget : public QLabel
{
    Q_OBJECT

public:
    explicit HighlightLabelWidget(QWidget* parent = nullptr);

public Q_SLOTS:
    // Hide QLabel::setText(const QString&)
    void setText(const QString& text);

protected:
    void focusInEvent(QFocusEvent* ev) override;
    void focusOutEvent(QFocusEvent* ev) override;

private:
    // Prevent zombie cursor (|).
    void Reset();
    // Select all the text (i.e., highlight it).
    void SelectAll();
};

#endif // BITCOIN_QT_HIGHLIGHTLABELWIDGET_H
