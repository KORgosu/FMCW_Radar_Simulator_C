#pragma once
#ifndef TAB3_CFAR_H
#define TAB3_CFAR_H

#include "../plot_widget.h"
#include <qcustomplot.h>
#include <vector>

// ─────────────────────────────────────────────
//  Tab3Widget — CFAR 탐지 탭
//  2×2 그리드:
//    [0,0] RDM + CFAR 탐지 오버레이 (히트맵)
//    [0,1] Range Profile + CFAR 임계선
//    [1,0] CFAR 탐지맵 (2D scatter)
//    [1,1] 군집 scatter
// ─────────────────────────────────────────────
class Tab3Widget : public RadarPlotWidget
{
    Q_OBJECT
public:
    explicit Tab3Widget(QWidget *parent = nullptr);

    void updatePlots(const SimResult &res, const RadarParams &p,
                     const std::vector<Target> &targets);

private:
    void buildUI();
    void drawRdmCfar     (const SimResult &res);
    void drawRangeCfar   (const SimResult &res);
    void drawDetectionMap(const SimResult &res);
    void drawCluster     (const SimResult &res);

    QCustomPlot   *m_plotRdmCfar;
    QCustomPlot   *m_plotRangeCfar;
    QCustomPlot   *m_plotDetMap;
    QCustomPlot   *m_plotCluster;

    QCPColorMap   *m_colorMap;
    QCPColorScale *m_colorScale;
};

#endif // TAB3_CFAR_H
