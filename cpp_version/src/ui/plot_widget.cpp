#include "plot_widget.h"
#include <QVBoxLayout>

// ─────────────────────────────────────────────
//  RadarPlotWidget
// ─────────────────────────────────────────────
RadarPlotWidget::RadarPlotWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 150);
}

void RadarPlotWidget::applyDarkTheme(QCustomPlot *plot)
{
    if (!plot) return;

    // 전체 배경
    plot->setBackground(QBrush(QColor("#060d06")));

    // axisRect 배경
    if (plot->axisRectCount() > 0) {
        QCPAxisRect *ar = plot->axisRect(0);
        ar->setBackground(QBrush(QColor("#060d06")));

        // 격자선
        ar->axis(QCPAxis::atBottom)->grid()->setPen(QPen(QColor("#0d2a0d"), 1, Qt::SolidLine));
        ar->axis(QCPAxis::atLeft  )->grid()->setPen(QPen(QColor("#0d2a0d"), 1, Qt::SolidLine));
        ar->axis(QCPAxis::atBottom)->grid()->setSubGridPen(QPen(QColor("#0d2a0d"), 1, Qt::DotLine));
        ar->axis(QCPAxis::atLeft  )->grid()->setSubGridPen(QPen(QColor("#0d2a0d"), 1, Qt::DotLine));
        ar->axis(QCPAxis::atBottom)->grid()->setZeroLinePen(QPen(QColor("#00aa30"), 1));
        ar->axis(QCPAxis::atLeft  )->grid()->setZeroLinePen(QPen(QColor("#00aa30"), 1));

        // 축 색상
        for (QCPAxis *ax : ar->axes()) {
            ax->setBasePen(QPen(QColor("#00aa30"), 1));
            ax->setTickPen(QPen(QColor("#00aa30"), 1));
            ax->setSubTickPen(QPen(QColor("#00aa30"), 1));
            ax->setTickLabelColor(QColor("#00aa30"));
            ax->setLabelColor(QColor("#00ff41"));
        }
    }

    // 범례 배경
    plot->legend->setBrush(QBrush(QColor("#0d2a0d")));
    plot->legend->setBorderPen(QPen(QColor("#00aa30")));
    plot->legend->setTextColor(QColor("#00ff41"));
}

void RadarPlotWidget::styleAxis(QCPAxis *axis, const QString &label)
{
    if (!axis) return;

    QFont f("Courier New", 9);
    axis->setTickLabelFont(f);
    axis->setLabelFont(f);
    axis->setTickLabelColor(QColor("#00aa30"));
    axis->setLabelColor(QColor("#00aa30"));

    if (!label.isEmpty())
        axis->setLabel(label);
}

void RadarPlotWidget::setColormapRange(QCPColorMap *cm, double vmin, double vmax)
{
    if (!cm) return;

    // hot 컬러맵: 검정 → 빨강 → 노랑 → 흰색
    QCPColorGradient gradient;
    gradient.clearColorStops();
    gradient.setColorStopAt(0.00, QColor(  0,   0,   0));  // 검정
    gradient.setColorStopAt(0.33, QColor(128,   0,   0));  // 어두운 빨강
    gradient.setColorStopAt(0.60, QColor(255,   0,   0));  // 빨강
    gradient.setColorStopAt(0.80, QColor(255, 200,   0));  // 노랑
    gradient.setColorStopAt(1.00, QColor(255, 255, 255));  // 흰색

    cm->setGradient(gradient);
    cm->setDataRange(QCPRange(vmin, vmax));
}
