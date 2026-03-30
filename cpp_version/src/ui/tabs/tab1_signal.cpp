#include "tab1_signal.h"
#include <QGridLayout>
#include <cmath>

// 타겟별 색상 팔레트
static const QColor TARGET_COLORS[] = {
    QColor("#00ff41"),
    QColor("#ffaa00"),
    QColor("#00ffff"),
    QColor("#ff4466"),
    QColor("#aa44ff"),
};
static constexpr int N_TARGET_COLORS = 5;

// ─────────────────────────────────────────────

Tab1Widget::Tab1Widget(QWidget *parent)
    : RadarPlotWidget(parent)
{
    buildUI();
}

void Tab1Widget::buildUI()
{
    QGridLayout *grid = new QGridLayout(this);
    grid->setContentsMargins(4, 4, 4, 4);
    grid->setSpacing(4);

    auto makePlot = [&](const QString &title) -> QCustomPlot* {
        QCustomPlot *plot = new QCustomPlot(this);
        applyDarkTheme(plot);
        plot->plotLayout()->insertRow(0);
        QCPTextElement *titleElem = new QCPTextElement(
            plot, title,
            QFont("Segoe UI", 11, QFont::Bold));
        titleElem->setTextColor(QColor("#e0ffe8"));
        titleElem->setTextFlags(Qt::AlignLeft | Qt::AlignVCenter);
        plot->plotLayout()->addElement(0, 0, titleElem);
        return plot;
    };

    m_plotChirp = makePlot("Chirp (Instantaneous Freq)");
    m_plotTxRx  = makePlot("TX / RX Frequency");
    m_plotBeat  = makePlot("Beat Signal (1st chirp)");
    m_plotRange = makePlot("Range Profile (dB)");

    grid->addWidget(m_plotChirp, 0, 0);
    grid->addWidget(m_plotTxRx,  0, 1);
    grid->addWidget(m_plotBeat,  1, 0);
    grid->addWidget(m_plotRange, 1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
}

// ── updatePlots ──────────────────────────────

void Tab1Widget::updatePlots(const SimResult &res, const RadarParams &p,
                              const std::vector<Target> &targets)
{
    drawChirp(res, p);
    drawTxRx(res, p, targets);
    drawBeat(res, p);
    drawRange(res, p);
}

// ── drawChirp ────────────────────────────────

void Tab1Widget::drawChirp(const SimResult &res, const RadarParams &p)
{
    QCustomPlot *plot = m_plotChirp;
    plot->clearGraphs();
    plot->clearItems();

    if (res.t_fast.empty() || res.tx_freq.empty()) {
        plot->replot();
        return;
    }

    int n = static_cast<int>(std::min(res.t_fast.size(), res.tx_freq.size()));
    QVector<double> t(n), f(n);
    for (int i = 0; i < n; ++i) {
        t[i] = res.t_fast[i] * 1e6;             // μs
        f[i] = res.tx_freq[i] / 1e9;            // GHz
    }

    plot->addGraph();
    plot->graph(0)->setData(t, f);
    plot->graph(0)->setPen(QPen(QColor("#00ff41"), 1.5));
    plot->graph(0)->setName("TX Freq");

    styleAxis(plot->xAxis, "Time (μs)");
    styleAxis(plot->yAxis, "Freq (GHz)");
    plot->rescaleAxes();

    // 대역폭 화살표 주석
    double t_mid = t[n/2];
    double f_lo  = f[0];
    double f_hi  = f[n-1];
    QCPItemText *bwLabel = new QCPItemText(plot);
    bwLabel->position->setType(QCPItemPosition::ptPlotCoords);
    bwLabel->position->setCoords(t_mid, (f_lo + f_hi) / 2.0);
    bwLabel->setText(QString("B=%1MHz").arg(p.bandwidth_hz/1e6, 0,'f',0));
    bwLabel->setFont(QFont("Courier New", 8));
    bwLabel->setColor(QColor("#ffaa00"));

    plot->replot();
}

// ── drawTxRx ─────────────────────────────────

void Tab1Widget::drawTxRx(const SimResult &res, const RadarParams &p,
                           const std::vector<Target> &targets)
{
    QCustomPlot *plot = m_plotTxRx;
    plot->clearGraphs();

    if (res.t_fast.empty() || res.tx_freq.empty()) {
        plot->replot();
        return;
    }

    int n = static_cast<int>(std::min(res.t_fast.size(), res.tx_freq.size()));
    QVector<double> t(n), fTx(n);
    for (int i = 0; i < n; ++i) {
        t[i]   = res.t_fast[i] * 1e6;
        fTx[i] = res.tx_freq[i] / 1e9;
    }

    // TX 라인
    plot->addGraph();
    plot->graph(0)->setData(t, fTx);
    plot->graph(0)->setPen(QPen(QColor("#00ff41"), 1.5));
    plot->graph(0)->setName("TX");

    // 타겟별 RX (beat 오프셋 적용)
    double mu = p.bandwidth_hz / p.chirp_dur_s;  // chirp rate (Hz/s)
    for (int ti = 0; ti < static_cast<int>(targets.size()); ++ti) {
        const Target &tgt = targets[ti];
        double tau  = 2.0 * tgt.range_m / 3e8;
        double fbeat = mu * tau;                 // range beat freq (Hz)

        QVector<double> fRx(n);
        for (int i = 0; i < n; ++i)
            fRx[i] = fTx[i] - fbeat / 1e9;

        QColor col = TARGET_COLORS[ti % N_TARGET_COLORS];
        int gIdx = plot->graphCount();
        plot->addGraph();
        plot->graph(gIdx)->setData(t, fRx);
        plot->graph(gIdx)->setPen(QPen(col, 1, Qt::DashLine));
        plot->graph(gIdx)->setName(QString("RX T%1").arg(ti+1));
    }

    plot->legend->setVisible(true);
    styleAxis(plot->xAxis, "Time (μs)");
    styleAxis(plot->yAxis, "Freq (GHz)");
    plot->rescaleAxes();
    plot->replot();
}

// ── drawBeat ─────────────────────────────────

void Tab1Widget::drawBeat(const SimResult &res, const RadarParams &p)
{
    Q_UNUSED(p)
    QCustomPlot *plot = m_plotBeat;
    plot->clearGraphs();

    if (res.beat_one_re.empty()) {
        plot->replot();
        return;
    }

    int n = static_cast<int>(res.beat_one_re.size());
    QVector<double> idx(n), re(n), im(n);
    for (int i = 0; i < n; ++i) {
        idx[i] = static_cast<double>(i);
        re[i]  = res.beat_one_re[i];
        im[i]  = (i < static_cast<int>(res.beat_one_im.size())) ? res.beat_one_im[i] : 0.0;
    }

    plot->addGraph();
    plot->graph(0)->setData(idx, re);
    plot->graph(0)->setPen(QPen(QColor("#00ff41"), 1));
    plot->graph(0)->setName("Re");

    plot->addGraph();
    plot->graph(1)->setData(idx, im);
    plot->graph(1)->setPen(QPen(QColor("#00ffff"), 1));
    plot->graph(1)->setName("Im");

    plot->legend->setVisible(true);
    styleAxis(plot->xAxis, "Sample");
    styleAxis(plot->yAxis, "Amplitude");
    plot->rescaleAxes();
    plot->replot();
}

// ── drawRange ────────────────────────────────

void Tab1Widget::drawRange(const SimResult &res, const RadarParams &p)
{
    Q_UNUSED(p)
    QCustomPlot *plot = m_plotRange;
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

    plot->addGraph();
    plot->graph(0)->setData(r, db);
    plot->graph(0)->setPen(QPen(QColor("#00ff41"), 1.5));

    // ΔR 레이블
    double range_res = (n > 1) ? (r[1] - r[0]) : 0.0;
    QCPItemText *drLabel = new QCPItemText(plot);
    drLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
    drLabel->position->setCoords(0.95, 0.08);
    drLabel->setPositionAlignment(Qt::AlignRight | Qt::AlignTop);
    drLabel->setText(QString("ΔR=%1m").arg(range_res, 0,'f',2));
    drLabel->setFont(QFont("Courier New", 8));
    drLabel->setColor(QColor("#ffaa00"));

    styleAxis(plot->xAxis, "Range (m)");
    styleAxis(plot->yAxis, "Power (dB)");
    plot->rescaleAxes();
    plot->replot();
}
