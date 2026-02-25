// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/donutchart.h>

#include <qt/guiutil.h>

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

namespace {
constexpr int CHART_MARGIN{10};
constexpr int TEXT_PADDING{20};
} // anonymous namespace

DonutChart::DonutChart(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
}

DonutChart::~DonutChart() = default;

void DonutChart::setData(std::vector<Slice>&& slices, double total_capacity, CenterText&& default_text)
{
    m_slices = std::move(slices);
    m_total_capacity = total_capacity;
    m_default_center_text = std::move(default_text);
    m_hovered_slice = -1;
    update();
}

void DonutChart::clear()
{
    m_slices.clear();
    m_total_capacity = 0;
    m_hovered_slice = -1;
    m_default_center_text = {};
    update();
}

QSize DonutChart::sizeHint() const
{
    return QSize(200, 200);
}

QSize DonutChart::minimumSizeHint() const
{
    return QSize(150, 150);
}

DonutChart::Geometry DonutChart::chartGeometry() const
{
    const int effective_height{static_cast<int>(height() * 0.8)};
    const int side{qMin(width(), effective_height) - 2 * CHART_MARGIN};
    const int outer_radius{qMax(0, side / 2)};
    const int donut_thickness{qBound(25, outer_radius / 4, 60)};
    return {
        .m_inner_radius = qMax(0, outer_radius - donut_thickness),
        .m_outer_radius = outer_radius,
        .m_center = {width() / 2, height() / 2}
    };
}

void DonutChart::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);

    const auto [inner_radius, outer_radius, center] = chartGeometry();
    if (outer_radius <= 0) return;

    // Draw background circle (unallocated portion)
    painter.setPen(Qt::NoPen);
    painter.setBrush(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BORDER_WIDGET));
    painter.drawEllipse(center, outer_radius, outer_radius);

    // Draw slices (clamped so total never exceeds 360 degrees)
    if (m_total_capacity > 0 && !m_slices.empty()) {
        int start_angle{90 * 16}; // Start from top (Qt uses 1/16th of a degree)
        int remaining_angle{360 * 16};
        for (size_t idx{0}; idx < m_slices.size(); idx++) {
            if (remaining_angle <= 0) break;
            const auto& slice = m_slices[idx];
            if (slice.m_value <= 0) continue;
            const double fraction{slice.m_value / m_total_capacity};
            const int span_angle{std::max(-(remaining_angle), static_cast<int>(-fraction * 360 * 16))}; // Negative for clockwise
            QColor color{slice.m_color};
            if (static_cast<int>(idx) == m_hovered_slice) {
                color = color.lighter(130);
            }
            painter.setBrush(color);
            painter.drawPie(center.x() - outer_radius, center.y() - outer_radius,
                            outer_radius * 2, outer_radius * 2, start_angle, span_angle);
            start_angle += span_angle;
            remaining_angle += span_angle; // span_angle is negative
        }
    }

    // Draw inner circle
    painter.setBrush(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_WIDGET));
    painter.drawEllipse(center, inner_radius, inner_radius);

    // Draw center text
    painter.setPen(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::DEFAULT));

    QString line1, line2, line3;
    if (m_hovered_slice >= 0 && m_hovered_slice < static_cast<int>(m_slices.size())) {
        const auto& slice = m_slices[m_hovered_slice];
        line1 = slice.m_donut_center_label;
        line2 = slice.m_donut_sub_label1;
        line3 = slice.m_donut_sub_label2;
    } else {
        line1 = m_default_center_text.m_donut_center_label;
        line2 = m_default_center_text.m_donut_sub_label1;
        line3 = m_default_center_text.m_donut_sub_label2;
    }

    // Calculate font sizes to fit in the inner circle - scale with widget size
    const int max_text_width{inner_radius * 2 - TEXT_PADDING};
    QFont font{painter.font()};

    // Scale font sizes based on inner radius (min 8pt, max reasonable sizes)
    const int primary_font_size{qBound(9, inner_radius * 11 / 60, 20)};
    const int secondary_font_size{qBound(8, inner_radius * 11 / 80, 15)};

    // Line 1 (name or allocated amount) - larger
    font.setPointSize(primary_font_size);
    font.setBold(true);
    painter.setFont(font);
    QFontMetrics fm1{font};
    QString elided_line1{fm1.elidedText(line1, Qt::ElideRight, max_text_width)};

    // Lines 2 and 3 - smaller
    font.setPointSize(secondary_font_size);
    font.setBold(false);
    painter.setFont(font);
    QFontMetrics fm2{font};

    const int line_height{fm2.height()};
    const int total_height{fm1.height() + line_height * 2 + 4};
    int y{center.y() - total_height / 2};

    // Draw line 1
    font.setPointSize(primary_font_size);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(center.x() - max_text_width / 2, y, max_text_width, fm1.height()),
                     Qt::AlignCenter, elided_line1);
    y += fm1.height() + 2;

    // Draw lines 2 and 3
    font.setPointSize(secondary_font_size);
    font.setBold(false);
    painter.setFont(font);
    const QString elided_line2{fm2.elidedText(line2, Qt::ElideRight, max_text_width)};
    const QString elided_line3{fm2.elidedText(line3, Qt::ElideRight, max_text_width)};
    painter.drawText(QRect(center.x() - max_text_width / 2, y, max_text_width, line_height),
                     Qt::AlignCenter, elided_line2);
    y += line_height;
    painter.drawText(QRect(center.x() - max_text_width / 2, y, max_text_width, line_height),
                     Qt::AlignCenter, elided_line3);
}

int DonutChart::sliceAtPosition(const QPoint& pos) const
{
    if (m_slices.empty() || m_total_capacity <= 0) {
        return -1;
    }

    const auto [inner_radius, outer_radius, center] = chartGeometry();

    // Check if point is within the donut ring
    const int dx{pos.x() - center.x()};
    const int dy{pos.y() - center.y()};
    const double dist{qSqrt(dx * dx + dy * dy)};

    if (dist < inner_radius || dist > outer_radius) {
        return -1;
    }

    // Calculate angle (0 = top, clockwise)
    // Note: swapped and negated for top=0, clockwise
    double angle{qAtan2(dx, -dy)};
    if (angle < 0) {
        angle += 2 * M_PI;
    }

    // Find which slice this angle falls into.
    // Mirror paintEvent's clamping so hit-regions match what is painted.
    double current_angle{0};
    double remaining{2 * M_PI};
    for (size_t idx{0}; idx < m_slices.size(); idx++) {
        if (remaining <= 0) break;
        if (m_slices[idx].m_value <= 0) continue;
        const double fraction{m_slices[idx].m_value / m_total_capacity};
        const double slice_angle{std::min(fraction * 2 * M_PI, remaining)};
        if (angle >= current_angle && angle < current_angle + slice_angle) {
            return static_cast<int>(idx);
        }
        current_angle += slice_angle;
        remaining -= slice_angle;
    }

    return -1;
}

void DonutChart::mouseMoveEvent(QMouseEvent* event)
{
    const int slice{sliceAtPosition(event->pos())};
    if (slice != m_hovered_slice) {
        m_hovered_slice = slice;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void DonutChart::leaveEvent(QEvent* event)
{
    if (m_hovered_slice != -1) {
        m_hovered_slice = -1;
        update();
    }
    QWidget::leaveEvent(event);
}
