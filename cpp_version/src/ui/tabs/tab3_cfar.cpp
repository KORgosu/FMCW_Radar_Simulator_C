#include "tab3_cfar.h"
#include <QGridLayout>
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────

Tab3Widget::Tab3Widget(QWidget *parent)
    : RadarPlotWidget(parent)
    , m_colorMap(nullptr)
    , m_colorScale(nullptr)
{
    buildUI();
}

void Tab3Widget::buildUI()
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

    m_plotRdmCfar   = makePlot("RDM + CFAR Detections");
    m_plotRangeCfar = makePlot("Range Profile + CFAR Threshold");
    m_plotDetMap    = makePlot("Detection Map");
    m_plotCluster   = makePlot("Cluster Scatter");

    // ── RDM 히트맵 초기화 ──
    m_colorMap   = new QCPColorMap(m_plotRdmCfar->xAxis, m_plotRdmCfar->yAxis);
    m_colorScale = new QCPColorScale(m_plotRdmCfar);
    m_plotRdmCfar->plotLayout()->addElement(1, 1, m_colorScale);
    m_colorScale->setType(QCPAxis::atRight);
    m_colorMap->setColorScale(m_colorScale);
    setColormapRange(m_colorMap, -80.0, 0.0);
    m_colorScale->axis()->setTickLabelColor(QColor("#00aa30"));
    m_colorScale->axis()->setLabelColor(QColor("#00ff41"));
    m_colorScale->axis()->setBasePen(QPen(QColor("#00aa30")));
    m_colorScale->axis()->setTickPen(QPen(QColor("#00aa30")));
    m_colorScale->setLabel("dB");

    QCPMarginGroup *mg = new QCPMarginGroup(m_plotRdmCfar);
    m_plotRdmCfar->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
    m_colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, mg);

    styleAxis(m_plotRdmCfar->xAxis, "Range (m)");
    styleAxis(m_plotRdmCfar->yAxis, "Velocity (m/s)");

    grid->addWidget(m_plotRdmCfar,   0, 0);
    grid->addWidget(m_plotRangeCfar, 0, 1);
    grid->addWidget(m_plotDetMap,    1, 0);
    grid->addWidget(m_plotCluster,   1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
}

// ── updatePlots ──────────────────────────────

void Tab3Widget::updatePlots(const SimResult &res, const RadarParams &p,
                              const std::vector<Target> &targets)
{
    Q_UNUSED(p)
    Q_UNUSED(targets)
    drawRdmCfar(res);
    drawRangeCfar(res);
    drawDetectionMap(res);
    drawCluster(res);
}

// ── drawRdmCfar ──────────────────────────────

void Tab3Widget::drawRdmCfar(const SimResult &res)
{
    QCustomPlot *plot = m_plotRdmCfar;

    // RDM 히트맵 채우기
    if (!res.rdm_db.empty() && !res.range_axis.empty() && !res.velocity_axis.empty()) {
        int rows = res.rdm_rows;
        int cols = res.rdm_cols;
        int nr   = static_cast<int>(res.range_axis.size());
        int nv   = static_cast<int>(res.velocity_axis.size());
        if (cols > nr) cols = nr;
        if (rows > nv) rows = nv;

        m_colorMap->data()->setSize(cols, rows);
        m_colorMap->data()->setRange(
            QCPRange(res.range_axis.front(), res.range_axis[cols-1]),
            QCPRange(res.velocity_axis.front(), res.velocity_axis[rows-1]));

        for (int vi = 0; vi < rows; ++vi)
            for (int ri = 0; ri < cols; ++ri)
                m_colorMap->data()->setCell(ri, vi, res.rdm_db[vi * res.rdm_cols + ri]);

        double vmin = *std::min_element(res.rdm_db.begin(), res.rdm_db.end());
        double vmax = *std::max_element(res.rdm_db.begin(), res.rdm_db.end());
        setColormapRange(m_colorMap, vmin, vmax);
        m_colorMap->rescaleDataRange();
    }

    // 탐지 포인트 오버레이 scatter
    // 기존 그래프 제거 (colormap 제외)
    while (plot->graphCount() > 0)
        plot->removeGraph(0);

    if (!res.detection_points.empty() &&
        !res.range_axis.empty() && !res.velocity_axis.empty()) {

        QVector<double> rx, rv;
        for (const auto &dp : res.detection_points) {
            rx.append(dp.range_m);
            rv.append(dp.velocity_mps);
        }

        plot->addGraph();
        plot->graph(0)->setData(rx, rv);
        plot->graph(0)->setLineStyle(QCPGraph::lsNone);
        plot->graph(0)->setScatterStyle(
            QCPScatterStyle(QCPScatterStyle::ssCircle, QColor("#00ffff"), QColor("#00ffff"), 8));
        plot->graph(0)->setName("Detections");
    }

    plot->rescaleAxes();
    plot->replot();
}

// ── drawRangeCfar ────────────────────────────

void Tab3Widget::drawRangeCfar(const SimResult &res)
{
    QCustomPlot *plot = m_plotRangeCfar;
    plot->clearGraphs();
    plot->clearItems();

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

    // Range profile
    plot->addGraph();
    plot->graph(0)->setData(r, db);
    plot->graph(0)->setPen(QPen(QColor("#00ff41"), 1.5));
    plot->graph(0)->setName("Range Profile");

    // CFAR 임계선: cfar_threshold의 첫 번째 Doppler 행 평균을 dB로 변환
    if (!res.cfar_threshold.empty() && res.rdm_cols > 0) {
        int cols = res.rdm_cols;
        int numCols = std::min(cols, static_cast<int>(res.cfar_threshold.size()));

        // 첫 번째 Doppler 행의 평균 임계값
        double sumThresh = 0.0;
        QVector<double> threshDb(n);
        for (int ri = 0; ri < numCols && ri < n; ++ri) {
            // 선형 → dB
            double lin = res.cfar_threshold[ri];
            threshDb[ri] = (lin > 0.0) ? 10.0 * std::log10(lin) : -100.0;
            sumThresh += threshDb[ri];
        }
        // 나머지 채우기
        double avgThresh = (numCols > 0) ? sumThresh / numCols : -40.0;
        for (int ri = numCols; ri < n; ++ri)
            threshDb[ri] = avgThresh;

        plot->addGraph();
        plot->graph(1)->setData(r, threshDb);
        plot->graph(1)->setPen(QPen(QColor("#ffaa00"), 1.5, Qt::DashLine));
        plot->graph(1)->setName("CFAR Threshold");
    }

    plot->legend->setVisible(true);
    styleAxis(plot->xAxis, "Range (m)");
    styleAxis(plot->yAxis, "Power (dB)");
    plot->rescaleAxes();
    plot->replot();
}

// ── drawDetectionMap ─────────────────────────

void Tab3Widget::drawDetectionMap(const SimResult &res)
{
    QCustomPlot *plot = m_plotDetMap;
    plot->clearGraphs();

    if (res.cfar_detections.empty() || res.range_axis.empty() || res.velocity_axis.empty()) {
        plot->replot();
        return;
    }

    int rows = res.rdm_rows;
    int cols = res.rdm_cols;
    int nr   = static_cast<int>(res.range_axis.size());
    int nv   = static_cast<int>(res.velocity_axis.size());
    if (cols > nr) cols = nr;
    if (rows > nv) rows = nv;

    QVector<double> rx, ry;
    for (int vi = 0; vi < rows; ++vi) {
        for (int ri = 0; ri < cols; ++ri) {
            if (vi * res.rdm_cols + ri < static_cast<int>(res.cfar_detections.size())) {
                if (res.cfar_detections[vi * res.rdm_cols + ri]) {
                    rx.append(res.range_axis[ri]);
                    ry.append(res.velocity_axis[vi]);
                }
            }
        }
    }

    plot->addGraph();
    plot->graph(0)->setData(rx, ry);
    plot->graph(0)->setLineStyle(QCPGraph::lsNone);
    plot->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssSquare, QColor("#00ff41"), QColor("#00ff41"), 6));
    plot->graph(0)->setName("Detected");

    styleAxis(plot->xAxis, "Range (m)");
    styleAxis(plot->yAxis, "Velocity (m/s)");
    plot->xAxis->setRange(res.range_axis.front(), res.range_axis.back());
    plot->yAxis->setRange(res.velocity_axis.front(), res.velocity_axis.back());
    plot->replot();
}

// ── drawCluster ──────────────────────────────

void Tab3Widget::drawCluster(const SimResult &res)
{
    QCustomPlot *plot = m_plotCluster;
    plot->clearGraphs();

    if (res.clusters.empty()) {
        // 탐지 포인트라도 표시
        if (!res.detection_points.empty()) {
            QVector<double> rx, rv;
            for (const auto &dp : res.detection_points) {
                rx.append(dp.range_m);
                rv.append(dp.velocity_mps);
            }
            plot->addGraph();
            plot->graph(0)->setData(rx, rv);
            plot->graph(0)->setLineStyle(QCPGraph::lsNone);
            plot->graph(0)->setScatterStyle(
                QCPScatterStyle(QCPScatterStyle::ssCross, QColor("#00aa30"), 8));
        }
        plot->replot();
        return;
    }

    // 군집별 scatter
    const QColor clusterColors[] = {
        QColor("#00ff41"), QColor("#ffaa00"), QColor("#00ffff"),
        QColor("#ff4466"), QColor("#aa44ff"), QColor("#44aaff"),
    };
    constexpr int NC = 6;

    // 군집 ID 별 분류
    std::map<int, QVector<double>> rxMap, rvMap;
    for (const auto &cl : res.clusters) {
        rxMap[cl.cluster_id].append(cl.range_m);
        rvMap[cl.cluster_id].append(cl.velocity_mps);
    }

    int ci = 0;
    for (auto &kv : rxMap) {
        QColor col = clusterColors[ci % NC];
        int gi = plot->graphCount();
        plot->addGraph();
        plot->graph(gi)->setData(kv.second, rvMap[kv.first]);
        plot->graph(gi)->setLineStyle(QCPGraph::lsNone);
        plot->graph(gi)->setScatterStyle(
            QCPScatterStyle(QCPScatterStyle::ssCircle, col, col, 10));
        plot->graph(gi)->setName(QString("Cluster %1").arg(kv.first));
        ++ci;
    }

    plot->legend->setVisible(true);
    styleAxis(plot->xAxis, "Range (m)");
    styleAxis(plot->yAxis, "Velocity (m/s)");
    plot->rescaleAxes();
    plot->replot();
}
