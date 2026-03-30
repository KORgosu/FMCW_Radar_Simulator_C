#pragma once
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QSplitter>
#include <QTabWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

#include "../core/simulator.h"
#include "param_panel.h"
#include "scene_widget.h"
#include "pipeline_inspector.h"
#include "architecture_window.h"
#include "tabs/tab1_signal.h"
#include "tabs/tab2_rdm.h"
#include "tabs/tab3_cfar.h"
#include "tabs/tab4_pipeline.h"

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
};

class StatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);
    void setStatus(const QString &msg, const RadarParams *p = nullptr);
private:
    QLabel *m_statusLbl;
    QLabel *m_paramsLbl;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onParamsChanged(RadarParams p);
    void onTargetsChanged(std::vector<Target> targets);
    void onTabChanged(int idx);
    void onComputeFinished();
    void scheduleUpdate();

    /* 멀티프레임 컨트롤 */
    void onPlay();
    void onStop();
    void onStepFrame();
    void onFrameTick();

private:
    void buildUI();
    void connectSignals();
    void runUpdate(bool is_step = false);
    void applyResult(const SimResult &result);

    /* DSP */
    FMCWSimulator                    *m_sim;
    RadarParams                       m_params;
    std::vector<Target>               m_targets;
    SimResult                         m_lastResult;
    QFutureWatcher<SimResult>        *m_watcher;

    /* 타이머 */
    QTimer *m_debounce;
    QTimer *m_frameTick;    /* 멀티프레임 재생 타이머 */
    bool    m_computing = false;
    bool    m_playing   = false;

    /* UI */
    ParamPanel          *m_paramPanel;
    SceneWidget         *m_scene;
    TargetInfoPanel     *m_targetInfo;
    QTabWidget          *m_tabs;
    Tab1Widget          *m_tab1;
    Tab2Widget          *m_tab2;
    Tab3Widget          *m_tab3;
    Tab4Widget          *m_tab4;
    PipelineInspector   *m_tab5;
    StatusBar           *m_statusBar;

    /* 멀티프레임 컨트롤 위젯 */
    QPushButton *m_btnPlay, *m_btnStop, *m_btnStep;
    QLabel      *m_frameLabel;
    int          m_frameCount = 0;

    /* 아키텍처 뷰어 */
    ArchitectureWindow *m_archWin = nullptr;
};

#endif /* MAIN_WINDOW_H */
