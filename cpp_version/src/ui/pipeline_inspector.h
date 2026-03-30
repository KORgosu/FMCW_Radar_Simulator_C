#pragma once
#ifndef PIPELINE_INSPECTOR_H
#define PIPELINE_INSPECTOR_H

#include <QWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QTextEdit>
#include <qcustomplot.h>
#include "../core/sim_result.h"
#include "../dsp/radar_types.h"

// ─────────────────────────────────────────────────────────────
//  StageView : INPUT → PROCESS → OUTPUT 3단 레이아웃
// ─────────────────────────────────────────────────────────────
class StageView : public QWidget
{
    Q_OBJECT
public:
    explicit StageView(QWidget *parent = nullptr);

    /* 스테이지 메타 정보 설정 */
    void setMeta(const QString &stageName,
                 const QString &inputShape,
                 const QString &formula,
                 const QString &description,
                 const QString &outputShape);

    QCustomPlot *inputPlot()  { return m_inputPlot;  }
    QCustomPlot *outputPlot() { return m_outputPlot; }

    static void applyDarkTheme(QCustomPlot *plot);

private:
    /* INPUT 패널 */
    QLabel      *m_inputShapeLbl;
    QCustomPlot *m_inputPlot;

    /* PROCESS 패널 */
    QLabel      *m_stageNameLbl;
    QLabel      *m_formulaLbl;
    QLabel      *m_descLbl;
    QLabel      *m_transformLbl;

    /* OUTPUT 패널 */
    QLabel      *m_outputShapeLbl;
    QCustomPlot *m_outputPlot;

    static QWidget *makePanelBox(const QString &title,
                                 const QColor  &borderColor,
                                 QWidget       *content);
};

// ─────────────────────────────────────────────────────────────
//  PipelineInspector
// ─────────────────────────────────────────────────────────────
class PipelineInspector : public QWidget
{
    Q_OBJECT
public:
    explicit PipelineInspector(QWidget *parent = nullptr);

public slots:
    void onSimResultReady(SimResult result);

private slots:
    void onStageSelected(int row);

private:
    void buildUI();
    void updateStage(int stage, const SimResult &res);

    void showStage0(const SimResult &res);
    void showStage1(const SimResult &res);
    void showStage2(const SimResult &res);
    void showStage3(const SimResult &res);
    void showStage4(const SimResult &res);
    void showStage5(const SimResult &res);
    void showStage6(const SimResult &res);

    /* 공용 헬퍼 */
    static void fillColorMap(QCustomPlot *plot,
                             const std::vector<double> &data,
                             int rows, int cols,
                             double xMin, double xMax,
                             double yMin, double yMax,
                             const QString &xLabel,
                             const QString &yLabel,
                             QCPColorGradient::GradientPreset preset
                                 = QCPColorGradient::gpHot);

    QListWidget    *m_stageList;
    QStackedWidget *m_stack;
    StageView      *m_views[7];
    SimResult       m_lastResult;
    int             m_currentStage = 0;
};

#endif /* PIPELINE_INSPECTOR_H */
