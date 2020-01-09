// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/highlightlabelwidget.h>

#include <QFocusEvent>
#include <QLabel>
#include <QString>
#include <QWidget>

HighlightLabelWidget::HighlightLabelWidget(QWidget* parent)
    : QLabel(parent)
{
    setCursor(Qt::IBeamCursor);
    setFocusPolicy(Qt::StrongFocus);
    setTextFormat(Qt::PlainText);
    setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard);
}

void HighlightLabelWidget::setText(const QString& text)
{
    const bool selected = hasSelectedText();
    QLabel::setText(text);
    if (selected) {
        SelectAll();
    }
}

void HighlightLabelWidget::focusInEvent(QFocusEvent* ev)
{
    if (ev->reason() == Qt::TabFocusReason || ev->reason() == Qt::BacktabFocusReason) {
        // Highligt label when focused via keyboard.
        SelectAll();
    }
}

void HighlightLabelWidget::focusOutEvent(QFocusEvent* ev)
{
    if (ev->reason() != Qt::PopupFocusReason) {
        Reset();
    }
}

void HighlightLabelWidget::Reset()
{
    const QString content = text();
    clear();
    QLabel::setText(content);
}

void HighlightLabelWidget::SelectAll()
{
    setSelection(0, text().size());
}
