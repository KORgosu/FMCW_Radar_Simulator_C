#pragma once
#ifndef TAB2_RDM_H
#define TAB2_RDM_H

#include "../plot_widget.h"
#include <qcustomplot.h>
#include <vector>

// ─────────────────────────────────────────────
//  Tab2Widget — Range-Doppler Map 탭
//  2×2 그리드: RDM 히트맵, Range Profile, Doppler Profile, 여백
// ─────────────────────────────────────────────
class Tab2Widget : public RadarPlotWidget
{
    Q_OBJECT
public:
    explicit Tab2Widget(QWidget *parent = nullptr);

    void updatePlots(const SimResult &res, const RadarParams &p,
                     const std::vector<Target> &targets);

private:
    void buildUI();
    void drawRdm          (const SimResult &res);
    void drawRangeProfile (const SimResult &res);
    void drawDopplerProfile(const SimResult &res);

    QCustomPlot   *m_plotRdm;
    QCustomPlot   *m_plotRange;
    QCustomPlot   *m_plotDoppler;
    QCustomPlot   *m_plotEmpty;

    QCPColorMap   *m_colorMap;
    QCPColorScale *m_colorScale;
};

#endif // TAB2_RDM_H
