#include "tab4_pipeline.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <cmath>
#include <algorithm>
#include <map>

// ─────────────────────────────────────────────

Tab4Widget::Tab4Widget(QWidget *parent)
    : RadarPlotWidget(parent)
    , m_cmBeat(nullptr), m_cmRdm(nullptr), m_cmCfar(nullptr)
    , m_objectList(nullptr)
{
    buildUI();
}

void Tab4Widget::buildUI()
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

    // ── Beat Matrix ──
    m_plotBeat = makePlot("Beat Matrix (Amplitude)");
    m_cmBeat = new QCPColorMap(m_plotBeat->xAxis, m_plotBeat->yAxis);
    {
        QCPColorScale *cs = new QCPColorScale(m_plotBeat);
        m_plotBeat->plotLayout()->addElement(1, 1, cs);
        cs->setType(QCPAxis::atRight);
        m_cmBeat->setColorScale(cs);
        setColormapRange(m_cmBeat, -80.0, 0.0);
        cs->axis()->setTickLabelColor(QColor("#00aa30"));
        cs->axis()->setBasePen(QPen(QColor("#00aa30")));
        cs->axis()->setTickPen(QPen(QColor("#00aa30")));
        cs->setLabel("dB");
        QCPMarginGroup *mg = new QCPMarginGroup(m_plotBeat);
        m_plotBeat->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
        cs->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
    }
    styleAxis(m_plotBeat->xAxis, "Sample");
    styleAxis(m_plotBeat->yAxis, "Chirp");

    // ── RDM ──
    m_plotRdm = makePlot("Range-Doppler Map");
    m_cmRdm = new QCPColorMap(m_plotRdm->xAxis, m_plotRdm->yAxis);
    {
        QCPColorScale *cs = new QCPColorScale(m_plotRdm);
        m_plotRdm->plotLayout()->addElement(1, 1, cs);
        cs->setType(QCPAxis::atRight);
        m_cmRdm->setColorScale(cs);
        setColormapRange(m_cmRdm, -80.0, 0.0);
        cs->axis()->setTickLabelColor(QColor("#00aa30"));
        cs->axis()->setBasePen(QPen(QColor("#00aa30")));
        cs->axis()->setTickPen(QPen(QColor("#00aa30")));
        cs->setLabel("dB");
        QCPMarginGroup *mg = new QCPMarginGroup(m_plotRdm);
        m_plotRdm->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
        cs->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
    }
    styleAxis(m_plotRdm->xAxis, "Range (m)");
    styleAxis(m_plotRdm->yAxis, "Velocity (m/s)");

    // ── CFAR Detection Map ──
    m_plotCfar = makePlot("CFAR Detection Map");
    m_cmCfar = new QCPColorMap(m_plotCfar->xAxis, m_plotCfar->yAxis);
    {
        QCPColorScale *cs = new QCPColorScale(m_plotCfar);
        m_plotCfar->plotLayout()->addElement(1, 1, cs);
        cs->setType(QCPAxis::atRight);
        m_cmCfar->setColorScale(cs);
        // 탐지맵: 0/1 이진 — 단순 흑/녹 그라디언트
        QCPColorGradient detGrad;
        detGrad.clearColorStops();
        detGrad.setColorStopAt(0.0, QColor("#060d06"));
        detGrad.setColorStopAt(1.0, QColor("#00ff41"));
        m_cmCfar->setGradient(detGrad);
        m_cmCfar->setDataRange(QCPRange(0.0, 1.0));
        cs->setDataRange(QCPRange(0.0, 1.0));
        cs->axis()->setTickLabelColor(QColor("#00aa30"));
        cs->axis()->setBasePen(QPen(QColor("#00aa30")));
        cs->axis()->setTickPen(QPen(QColor("#00aa30")));
        QCPMarginGroup *mg = new QCPMarginGroup(m_plotCfar);
        m_plotCfar->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
        cs->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
    }
    styleAxis(m_plotCfar->xAxis, "Range (m)");
    styleAxis(m_plotCfar->yAxis, "Velocity (m/s)");

    // ── Point Cloud + Object List ──
    m_plotCloud = makePlot("Point Cloud");
    styleAxis(m_plotCloud->xAxis, "X (m)");
    styleAxis(m_plotCloud->yAxis, "Y (m)");

    // Object List 텍스트 박스
    m_objectList = new QTextEdit(this);
    m_objectList->setReadOnly(true);
    m_objectList->setMaximumHeight(120);
    m_objectList->setStyleSheet(
        "QTextEdit { background:#060d06; color:#00ff41;"
        "  font-family:'Courier New'; font-size:8pt;"
        "  border:1px solid #00aa30; }");

    // Point Cloud + ObjectList 을 수직으로 분할
    QWidget *cloudWidget = new QWidget(this);
    QVBoxLayout *cloudLay = new QVBoxLayout(cloudWidget);
    cloudLay->setContentsMargins(0, 0, 0, 0);
    cloudLay->setSpacing(4);
    cloudLay->addWidget(m_plotCloud, 1);
    cloudLay->addWidget(m_objectList);

    grid->addWidget(m_plotBeat,  0, 0);
    grid->addWidget(m_plotRdm,   0, 1);
    grid->addWidget(m_plotCfar,  1, 0);
    grid->addWidget(cloudWidget, 1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
}

// ── updatePlots ──────────────────────────────

void Tab4Widget::updatePlots(const SimResult &res, const RadarParams &p,
                              const std::vector<Target> &targets)
{
    Q_UNUSED(p)
    Q_UNUSED(targets)
    drawBeatMatrix(res);
    drawRdm(res);
    drawCfarMap(res);
    drawPointCloud(res);
}

// ── drawBeatMatrix ───────────────────────────

void Tab4Widget::drawBeatMatrix(const SimResult &res)
{
    if (res.beat_mat_re.empty() || res.beat_rows == 0 || res.beat_cols == 0) {
        m_plotBeat->replot();
        return;
    }

    int rows = res.beat_rows;
    int cols = res.beat_cols;

    m_cmBeat->data()->setSize(cols, rows);
    m_cmBeat->data()->setRange(
        QCPRange(0, cols),
        QCPRange(0, rows));

    double vmax = 0.0;
    for (int vi = 0; vi < rows; ++vi) {
        for (int ri = 0; ri < cols; ++ri) {
            double re = res.beat_mat_re[vi * cols + ri];
            double im = (static_cast<int>(res.beat_mat_im.size()) > vi * cols + ri)
                        ? res.beat_mat_im[vi * cols + ri] : 0.0;
            double amp = std::sqrt(re*re + im*im);
            if (amp > vmax) vmax = amp;
            m_cmBeat->data()->setCell(ri, vi, amp);
        }
    }

    // 진폭 → dB 재계산
    for (int vi = 0; vi < rows; ++vi) {
        for (int ri = 0; ri < cols; ++ri) {
            double amp;
            amp = m_cmBeat->data()->cell(ri, vi);
            double db = (amp > 0.0 && vmax > 0.0)
                        ? 20.0 * std::log10(amp / vmax)
                        : -80.0;
            m_cmBeat->data()->setCell(ri, vi, db);
        }
    }

    setColormapRange(m_cmBeat, -80.0, 0.0);
    m_plotBeat->rescaleAxes();
    m_plotBeat->replot();
}

// ── drawRdm ──────────────────────────────────

void Tab4Widget::drawRdm(const SimResult &res)
{
    if (res.rdm_db.empty() || res.range_axis.empty() || res.velocity_axis.empty()) {
        m_plotRdm->replot();
        return;
    }

    int rows = res.rdm_rows;
    int cols = res.rdm_cols;
    int nr   = static_cast<int>(res.range_axis.size());
    int nv   = static_cast<int>(res.velocity_axis.size());
    if (cols > nr) cols = nr;
    if (rows > nv) rows = nv;

    m_cmRdm->data()->setSize(cols, rows);
    m_cmRdm->data()->setRange(
        QCPRange(res.range_axis.front(), res.range_axis[cols-1]),
        QCPRange(res.velocity_axis.front(), res.velocity_axis[rows-1]));

    for (int vi = 0; vi < rows; ++vi)
        for (int ri = 0; ri < cols; ++ri)
            m_cmRdm->data()->setCell(ri, vi, res.rdm_db[vi * res.rdm_cols + ri]);

    double vmin = *std::min_element(res.rdm_db.begin(), res.rdm_db.end());
    double vmax = *std::max_element(res.rdm_db.begin(), res.rdm_db.end());
    setColormapRange(m_cmRdm, vmin, vmax);
    m_cmRdm->rescaleDataRange();
    m_plotRdm->rescaleAxes();
    m_plotRdm->replot();
}

// ── drawCfarMap ──────────────────────────────

void Tab4Widget::drawCfarMap(const SimResult &res)
{
    if (res.cfar_detections.empty() || res.range_axis.empty() || res.velocity_axis.empty()) {
        m_plotCfar->replot();
        return;
    }

    int rows = res.rdm_rows;
    int cols = res.rdm_cols;
    int nr   = static_cast<int>(res.range_axis.size());
    int nv   = static_cast<int>(res.velocity_axis.size());
    if (cols > nr) cols = nr;
    if (rows > nv) rows = nv;

    m_cmCfar->data()->setSize(cols, rows);
    m_cmCfar->data()->setRange(
        QCPRange(res.range_axis.front(), res.range_axis[cols-1]),
        QCPRange(res.velocity_axis.front(), res.velocity_axis[rows-1]));

    for (int vi = 0; vi < rows; ++vi) {
        for (int ri = 0; ri < cols; ++ri) {
            double val = 0.0;
            int idx = vi * res.rdm_cols + ri;
            if (idx < static_cast<int>(res.cfar_detections.size()))
                val = res.cfar_detections[idx] ? 1.0 : 0.0;
            m_cmCfar->data()->setCell(ri, vi, val);
        }
    }

    m_plotCfar->rescaleAxes();
    m_plotCfar->replot();
}

// ── drawPointCloud ───────────────────────────

void Tab4Widget::drawPointCloud(const SimResult &res)
{
    QCustomPlot *plot = m_plotCloud;
    plot->clearGraphs();

    // detection_points: 극좌표 → 직교 변환
    if (!res.detection_points.empty()) {
        QVector<double> px, py;
        for (const auto &dp : res.detection_points) {
            double ang_rad = dp.angle_deg * M_PI / 180.0;
            px.append(dp.range_m * std::sin(ang_rad));
            py.append(dp.range_m * std::cos(ang_rad));
        }
        plot->addGraph();
        plot->graph(0)->setData(px, py);
        plot->graph(0)->setLineStyle(QCPGraph::lsNone);
        plot->graph(0)->setScatterStyle(
            QCPScatterStyle(QCPScatterStyle::ssCross, QColor("#00aa30"), 6));
        plot->graph(0)->setName("Points");
    }

    // clusters
    if (!res.clusters.empty()) {
        QVector<double> cx, cy;
        for (const auto &cl : res.clusters) {
            double ang_rad = cl.angle_deg * M_PI / 180.0;
            cx.append(cl.range_m * std::sin(ang_rad));
            cy.append(cl.range_m * std::cos(ang_rad));
        }
        int gi = plot->graphCount();
        plot->addGraph();
        plot->graph(gi)->setData(cx, cy);
        plot->graph(gi)->setLineStyle(QCPGraph::lsNone);
        plot->graph(gi)->setScatterStyle(
            QCPScatterStyle(QCPScatterStyle::ssCircle, QColor("#ffaa00"), QColor("#ffaa00"), 12));
        plot->graph(gi)->setName("Clusters");
    }

    // objects (EKF 트랙)
    if (!res.objects.empty()) {
        QVector<double> ox, oy;
        for (const auto &obj : res.objects) {
            if (!obj.confirmed) continue;
            double ang_rad = obj.angle_deg * M_PI / 180.0;
            ox.append(obj.range_m * std::sin(ang_rad));
            oy.append(obj.range_m * std::cos(ang_rad));
        }
        if (!ox.isEmpty()) {
            int gi = plot->graphCount();
            plot->addGraph();
            plot->graph(gi)->setData(ox, oy);
            plot->graph(gi)->setLineStyle(QCPGraph::lsNone);
            plot->graph(gi)->setScatterStyle(
                QCPScatterStyle(QCPScatterStyle::ssStar, QColor("#00ffff"), 14));
            plot->graph(gi)->setName("Tracks");
        }
    }

    plot->legend->setVisible(true);
    styleAxis(plot->xAxis, "X (m)");
    styleAxis(plot->yAxis, "Y (m)");
    if (plot->graphCount() > 0)
        plot->rescaleAxes();
    else {
        plot->xAxis->setRange(-50, 50);
        plot->yAxis->setRange(0, 200);
    }
    plot->replot();

    // ── Object List 텍스트 ──
    QString text;
    if (res.objects.empty()) {
        text = "No tracked objects.";
    } else {
        text = "ID  Range    Vel     Angle   Conf\n";
        text += QString("-").repeated(48) + "\n";
        for (const auto &obj : res.objects) {
            text += QString("%1   %2m   %3m/s   %4°   %5\n")
                .arg(obj.id, 3)
                .arg(obj.range_m, 7, 'f', 1)
                .arg(obj.velocity_mps, 6, 'f', 1)
                .arg(obj.angle_deg, 5, 'f', 1)
                .arg(obj.confirmed ? "YES" : "no ");
        }
    }
    m_objectList->setText(text);
}
