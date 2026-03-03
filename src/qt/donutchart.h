// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_DONUTCHART_H
#define BITCOIN_QT_DONUTCHART_H

#include <QWidget>

#include <vector>

class DonutChart : public QWidget
{
    Q_OBJECT

public:
    struct Slice {
        double m_value{0.0};
        QColor m_color;
        QString m_donut_center_label;
        QString m_donut_sub_label1;
        QString m_donut_sub_label2;
    };

    struct CenterText {
        QString m_donut_center_label;
        QString m_donut_sub_label1;
        QString m_donut_sub_label2;
    };

    explicit DonutChart(QWidget* parent = nullptr);
    ~DonutChart() override;

    void setData(std::vector<Slice>&& slices, double total_capacity, CenterText&& default_text);
    void clear();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Geometry {
        int m_inner_radius;
        int m_outer_radius;
        QPoint m_center;
    };

    Geometry chartGeometry() const;
    int sliceAtPosition(const QPoint& pos) const;

private:
    CenterText m_default_center_text;
    double m_total_capacity{0};
    int m_hovered_slice{-1};
    std::vector<Slice> m_slices;
};

#endif // BITCOIN_QT_DONUTCHART_H
