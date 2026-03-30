#include "tab2_rdm.h"
#include <QGridLayout>
#include <cmath>

// ─────────────────────────────────────────────

Tab2Widget::Tab2Widget(QWidget *parent)
    : RadarPlotWidget(parent)
    , m_colorMap(nullptr)
    , m_colorScale(nullptr)
{
    buildUI();
}

void Tab2Widget::buildUI()
{
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(4, 4, 4, 4);
    grid->setSpacing(4);

    auto makePlot = [&](const QString &title) -> QCustomPlot* {
        QCustomPlot *plot = new QCustomPlot(this);
        applyDarkTheme(plot);
        plot->plotLayout()->insertRow(0);
        QCPTextElement *titleElem = new QCPTextElement(
            plot, title, QFont("Segoe UI", 11, QFont::Bold));
        titleElem->setTextColor(QColor("#e0ffe8"));
        titleElem->setTextFlags(Qt::AlignLeft | Qt::AlignVCenter);
        plot->plotLayout()->addElement(0, 0, titleElem);
        return plot;
    };

    m_plotRdm      = makePlot("Range-Doppler Map");
    m_plotRange    = makePlot("Range Profile (dB)");
    m_plotDoppler  = makePlot("Doppler Profile (dB)");
    m_plotEmpty    = makePlot("Params");

    // ── RDM 히트맵 초기화 ──
    m_colorMap = new QCPColorMap(m_plotRdm->xAxis, m_plotRdm->yAxis);
    m_colorScale = new QCPColorScale(m_plotRdm);
    m_plotRdm->plotLayout()->addElement(1, 1, m_colorScale);
    m_colorScale->setType(QCPAxis::atRight);
    m_colorMap->setColorScale(m_colorScale);
    setColormapRange(m_colorMap, -80.0, 0.0);
    m_colorScale->axis()->setTickLabelColor(QColor("#00aa30"));
    m_colorScale->axis()->setLabelColor(QColor("#00ff41"));
    m_colorScale->axis()->setBasePen(QPen(QColor("#00aa30")));
    m_colorScale->axis()->setTickPen(QPen(QColor("#00aa30")));
    m_colorScale->setLabel("dB");

    QCPMarginGroup *mg = new QCPMarginGroup(m_plotRdm);
    m_plotRdm->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
    m_colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, mg);

    styleAxis(m_plotRdm->xAxis, "Range (m)");
    styleAxis(m_plotRdm->yAxis, "Velocity (m/s)");

    grid->addWidget(m_plotRdm,     0, 0);
    grid->addWidget(m_plotRange,   0, 1);
    grid->addWidget(m_plotDoppler, 1, 0);
    grid->addWidget(m_plotEmpty,   1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
}

// ── updatePlots ──────────────────────────────

void Tab2Widget::updatePlots(const SimResult &res, const RadarParams &p,
                              const std::vector<Target> &targets)
{
    Q_UNUSED(p)
    Q_UNUSED(targets)
    drawRdm(res);
    drawRangeProfile(res);
    drawDopplerProfile(res);
}

// ── drawRdm ──────────────────────────────────

void Tab2Widget::drawRdm(const SimResult &res)
{
    if (res.rdm_db.empty() || res.range_axis.empty() || res.velocity_axis.empty()) {
        m_plotRdm->replot();
        return;
    }

    int rows = res.rdm_rows;  // Doppler bins
    int cols = res.rdm_cols;  // Range bins

    int nr = static_cast<int>(res.range_axis.size());
    int nv = static_cast<int>(res.velocity_axis.size());
    if (cols > nr) cols = nr;
    if (rows > nv) rows = nv;

    m_colorMap->data()->setSize(cols, rows);
    m_colorMap->data()->setRange(
        QCPRange(res.range_axis.front(), res.range_axis[cols-1]),
        QCPRange(res.velocity_axis.front(), res.velocity_axis[rows-1]));

    for (int vi = 0; vi < rows; ++vi) {
        for (int ri = 0; ri < cols; ++ri) {
            double val = res.rdm_db[vi * res.rdm_cols + ri];
            m_colorMap->data()->setCell(ri, vi, val);
        }
    }

    // 데이터 범위에서 자동 컬러 스케일
    double vmin = *std::min_element(res.rdm_db.begin(), res.rdm_db.end());
    double vmax = *std::max_element(res.rdm_db.begin(), res.rdm_db.end());
    setColormapRange(m_colorMap, vmin, vmax);
    m_colorMap->rescaleDataRange();

    m_plotRdm->rescaleAxes();
    m_plotRdm->replot();
}

// ── drawRangeProfile ─────────────────────────

void Tab2Widget::drawRangeProfile(const SimResult &res)
{
    QCustomPlot *plot = m_plotRange;
    plot->clearGraphs();

    if (res.range_axis.empty() || res.range_profile_db.empty()) {
        plot->replot();
        return;
    }

    int n = static_cast<int>(std::min(res.range_axis.size(),
                                       res.range_profile_db.size()));
    QVector<double> r(n), db(n);
    for (int i = 0; i < n; ++i) {
        r[i]  = res.range_axis[i];
        db[i] = res.range_profile_db[i];
    }

    plot->addGraph();
    plot->graph(0)->setData(r, db);
    plot->graph(0)->setPen(QPen(QColor("#00ff41"), 1.5));

    styleAxis(plot->xAxis, "Range (m)");
    styleAxis(plot->yAxis, "Power (dB)");
    plot->rescaleAxes();
    plot->replot();
}

// ── drawDopplerProfile ───────────────────────

void Tab2Widget::drawDopplerProfile(const SimResult &res)
{
    QCustomPlot *plot = m_plotDoppler;
    plot->clearGraphs();

    if (res.rdm_db.empty() || res.velocity_axis.empty()) {
        plot->replot();
        return;
    }

    // 전체 Range 에 대한 max-projection (Doppler 축)
    int rows = res.rdm_rows;
    int cols = res.rdm_cols;
    int nv   = static_cast<int>(res.velocity_axis.size());
    if (rows > nv) rows = nv;

    QVector<double> vel(rows), maxDb(rows);
    for (int vi = 0; vi < rows; ++vi) {
        vel[vi] = res.velocity_axis[vi];
        double mx = -1e30;
        for (int ri = 0; ri < cols; ++ri)
            mx = std::max(mx, res.rdm_db[vi * res.rdm_cols + ri]);
        maxDb[vi] = mx;
    }

    plot->addGraph();
    plot->graph(0)->setData(vel, maxDb);
    plot->graph(0)->setPen(QPen(QColor("#ffaa00"), 1.5));

    styleAxis(plot->xAxis, "Velocity (m/s)");
    styleAxis(plot->yAxis, "Max Power (dB)");
    plot->rescaleAxes();
    plot->replot();
}
