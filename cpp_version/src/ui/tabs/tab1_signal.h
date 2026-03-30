#pragma once
#ifndef TAB1_SIGNAL_H
#define TAB1_SIGNAL_H

#include "../plot_widget.h"
#include <qcustomplot.h>
#include <vector>

// ─────────────────────────────────────────────
//  Tab1Widget — 신호 파형 탭
//  2×2 그리드: Chirp, TX/RX, Beat, Range Profile
// ─────────────────────────────────────────────
class Tab1Widget : public RadarPlotWidget
{
    Q_OBJECT
public:
    explicit Tab1Widget(QWidget *parent = nullptr);

    void updatePlots(const SimResult &res, const RadarParams &p,
                     const std::vector<Target> &targets);

private:
    void buildUI();
    void drawChirp (const SimResult &res, const RadarParams &p);
    void drawTxRx  (const SimResult &res, const RadarParams &p, const std::vector<Target> &targets);
    void drawBeat  (const SimResult &res, const RadarParams &p);
    void drawRange (const SimResult &res, const RadarParams &p);

    QCustomPlot *m_plotChirp;
    QCustomPlot *m_plotTxRx;
    QCustomPlot *m_plotBeat;
    QCustomPlot *m_plotRange;
};

#endif // TAB1_SIGNAL_H
