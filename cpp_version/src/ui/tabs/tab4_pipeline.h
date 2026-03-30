#pragma once
#ifndef TAB4_PIPELINE_H
#define TAB4_PIPELINE_H

#include "../plot_widget.h"
#include <qcustomplot.h>
#include <QTextEdit>
#include <vector>

// ─────────────────────────────────────────────
//  Tab4Widget — 전체 파이프라인 탭
//  2×2 그리드:
//    [0,0] Beat Matrix 히트맵
//    [0,1] RDM 히트맵
//    [1,0] CFAR 탐지맵
//    [1,1] Point Cloud scatter + Object List 텍스트
// ─────────────────────────────────────────────
class Tab4Widget : public RadarPlotWidget
{
    Q_OBJECT
public:
    explicit Tab4Widget(QWidget *parent = nullptr);

    void updatePlots(const SimResult &res, const RadarParams &p,
                     const std::vector<Target> &targets);

private:
    void buildUI();
    void drawBeatMatrix (const SimResult &res);
    void drawRdm        (const SimResult &res);
    void drawCfarMap    (const SimResult &res);
    void drawPointCloud (const SimResult &res);

    QCustomPlot   *m_plotBeat;
    QCustomPlot   *m_plotRdm;
    QCustomPlot   *m_plotCfar;
    QCustomPlot   *m_plotCloud;
    QTextEdit     *m_objectList;

    QCPColorMap   *m_cmBeat;
    QCPColorMap   *m_cmRdm;
    QCPColorMap   *m_cmCfar;
};

#endif // TAB4_PIPELINE_H
