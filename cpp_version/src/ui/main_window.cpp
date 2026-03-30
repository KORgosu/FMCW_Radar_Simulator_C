#include "main_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QFont>
#include <QToolBar>
#include <QAction>
#include <QPixmap>
#include <QtConcurrent/QtConcurrent>

/* ── TitleBar ────────────────────────────────────────────────── */
TitleBar::TitleBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(48);
    setStyleSheet("background-color:#060d06; border-bottom:1px solid #0d2a0d;");
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);

    auto *title = new QLabel("FMCW RADAR SIMULATOR");
    title->setStyleSheet("color:#00ff41; font-family:'Courier New'; font-size:14px;"
                         "font-weight:bold; letter-spacing:4px; background:transparent;");
    auto *sub = new QLabel("77GHz mmWave · Signal Processing Pipeline");
    sub->setStyleSheet("color:#004a10; font-family:'Courier New'; font-size:9px;"
                       "letter-spacing:2px; background:transparent;");
    layout->addWidget(title);
    layout->addSpacing(16);
    layout->addWidget(sub);
    layout->addStretch();

    auto *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    infoLayout->setContentsMargins(0,0,0,0);
    auto *lab1 = new QLabel("세종대학교 국방레이더기술연구실");
    lab1->setStyleSheet("color:#00dd44; font-family:'Malgun Gothic'; font-size:13px;"
                        " font-weight:bold; background:transparent;");
    lab1->setAlignment(Qt::AlignRight);
    auto *lab2 = new QLabel("made by KORgosu (Su Hwan Park)");
    lab2->setStyleSheet("color:#00aa30; font-family:'Courier New'; font-size:11px; background:transparent;");
    lab2->setAlignment(Qt::AlignRight);
    infoLayout->addWidget(lab1);
    infoLayout->addWidget(lab2);

    /* 세종대 로고 */
    auto *logoLabel = new QLabel();
    QPixmap logo(":/images/sejong_logo.png");
    if (!logo.isNull())
        logoLabel->setPixmap(logo.scaledToHeight(36, Qt::SmoothTransformation));
    logoLabel->setStyleSheet("background:transparent;");
    logoLabel->setContentsMargins(8, 0, 0, 0);

    layout->addLayout(infoLayout);
    layout->addWidget(logoLabel);
}

/* ── StatusBar ───────────────────────────────────────────────── */
StatusBar::StatusBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(22);
    setStyleSheet("background:#060d06; border-top:1px solid #0d2a0d;");
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8,0,8,0);
    layout->setSpacing(8);
    auto *dot = new QLabel("●");
    dot->setStyleSheet("color:#00ff41; font-size:8px;");
    m_statusLbl = new QLabel("READY");
    m_statusLbl->setStyleSheet("color:#00ff41; font-size:9px; font-family:'Courier New';");
    m_paramsLbl = new QLabel();
    m_paramsLbl->setStyleSheet("color:#004a10; font-size:8px; font-family:'Courier New';");
    layout->addWidget(dot);
    layout->addWidget(m_statusLbl);
    layout->addStretch();
    layout->addWidget(m_paramsLbl);
}

void StatusBar::setStatus(const QString &msg, const RadarParams *p)
{
    m_statusLbl->setText(msg);
    if (p) {
        RadarParamsDerived d = params_derive(p);
        m_paramsLbl->setText(
            QString("fc=%1GHz  B=%2MHz  ΔR=%3m  Rmax=%4m  Vmax=±%5m/s")
                .arg(p->fc_hz/1e9, 0,'f',1)
                .arg(p->bandwidth_hz/1e6, 0,'f',0)
                .arg(d.range_resolution, 0,'f',2)
                .arg(d.range_max_m, 0,'f',1)
                .arg(d.velocity_max_mps, 0,'f',1)
        );
    }
}

/* ── MainWindow ──────────────────────────────────────────────── */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("FMCW Radar Simulator");
    resize(1440, 860);
    setMinimumSize(1100, 700);
    setStyleSheet("background:#060d06; color:#00ff41;");

    m_params  = params_default();
    m_sim     = new FMCWSimulator(this);
    m_watcher = new QFutureWatcher<SimResult>(this);

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(80);

    m_frameTick = new QTimer(this);
    m_frameTick->setInterval(200);  /* 5 FPS 멀티프레임 재생 */

    buildUI();
    connectSignals();

    m_scene->addDefaultTargets();
}

MainWindow::~MainWindow() {}

void MainWindow::buildUI()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    root->addWidget(new TitleBar());

    /* ── 메인 스플리터 ── */
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(2);
    splitter->setStyleSheet("QSplitter::handle { background:#0d2a0d; }");

    /* 좌측 패널 */
    auto *left = new QWidget();
    left->setFixedWidth(430);
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(2,4,2,4);
    leftLayout->setSpacing(4);

    m_paramPanel = new ParamPanel();
    leftLayout->addWidget(m_paramPanel);

    auto *sceneLbl = new QLabel("[ SCENE — 타겟 배치 ]");
    sceneLbl->setStyleSheet("color:#00ff41; font-size:9px; font-family:'Courier New';"
                            "letter-spacing:1px; border-top:1px solid #0d2a0d; padding-top:4px;");
    leftLayout->addWidget(sceneLbl);

    m_scene = new SceneWidget();
    m_scene->setMinimumHeight(200);
    leftLayout->addWidget(m_scene);

    m_targetInfo = new TargetInfoPanel(m_scene);
    leftLayout->addWidget(m_targetInfo);

    splitter->addWidget(left);

    /* 우측 탭 */
    m_tabs = new QTabWidget();
    m_tabs->setDocumentMode(true);
    m_tabs->setStyleSheet(
        "QTabWidget::pane { border:1px solid #0d2a0d; background:#060d06; }"
        "QTabBar::tab { background:#0d2a0d; color:#007a20; padding:4px 12px;"
        "  font-family:'Courier New'; font-size:10px; }"
        "QTabBar::tab:selected { background:#060d06; color:#00ff41; }"
        "QTabBar::tab:hover { color:#00cc44; }"
    );

    m_tab1 = new Tab1Widget();
    m_tab2 = new Tab2Widget();
    m_tab3 = new Tab3Widget();
    m_tab4 = new Tab4Widget();
    m_tab5 = new PipelineInspector();

    m_tabs->addTab(m_tab1, "[ 1 ] 신호 원리");
    m_tabs->addTab(m_tab2, "[ 2 ] Range-Doppler");
    m_tabs->addTab(m_tab3, "[ 3 ] CFAR 탐지");
    m_tabs->addTab(m_tab4, "[ 4 ] 통합 파이프라인");
    m_tabs->addTab(m_tab5, "[ 5 ] 파이프라인");

    splitter->addWidget(m_tabs);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    root->addWidget(splitter);

    /* ── 멀티프레임 컨트롤 툴바 ── */
    auto *ctrlBar = new QWidget();
    ctrlBar->setFixedHeight(28);
    ctrlBar->setStyleSheet("background:#060d06; border-top:1px solid #0d2a0d;");
    auto *ctrlLayout = new QHBoxLayout(ctrlBar);
    ctrlLayout->setContentsMargins(8,2,8,2);
    ctrlLayout->setSpacing(6);

    auto *ctrlLbl = new QLabel("MULTI-FRAME:");
    ctrlLbl->setStyleSheet("color:#007a20; font-family:'Courier New'; font-size:9px;");

    QString btnStyle = "QPushButton { background:#0d2a0d; color:#00ff41; border:1px solid #1a5a1a;"
                       " font-family:'Courier New'; font-size:10px; padding:1px 8px; }"
                       "QPushButton:hover { background:#1a5a1a; }"
                       "QPushButton:pressed { background:#00ff41; color:#000; }";

    m_btnPlay = new QPushButton("▶ 재생");
    m_btnStop = new QPushButton("■ 정지");
    m_btnStep = new QPushButton("▶| 한 프레임");
    m_btnPlay->setStyleSheet(btnStyle);
    m_btnStop->setStyleSheet(btnStyle);
    m_btnStep->setStyleSheet(btnStyle);
    m_btnStop->setEnabled(false);

    m_frameLabel = new QLabel("FRAME: 0");
    m_frameLabel->setStyleSheet("color:#004a10; font-family:'Courier New'; font-size:9px;");

    auto *btnArch = new QPushButton("[ A ] 아키텍처 뷰어");
    btnArch->setStyleSheet(btnStyle);
    connect(btnArch, &QPushButton::clicked, this, [this]() {
        if (!m_archWin)
            m_archWin = new ArchitectureWindow(this);
        m_archWin->show();
        m_archWin->raise();
        m_archWin->activateWindow();
    });

    ctrlLayout->addWidget(ctrlLbl);
    ctrlLayout->addWidget(m_btnPlay);
    ctrlLayout->addWidget(m_btnStop);
    ctrlLayout->addWidget(m_btnStep);
    ctrlLayout->addWidget(m_frameLabel);
    ctrlLayout->addStretch();
    ctrlLayout->addWidget(btnArch);

    root->addWidget(ctrlBar);

    /* ── 상태 바 ── */
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color:#0d2a0d;");
    root->addWidget(sep);

    m_statusBar = new StatusBar();
    root->addWidget(m_statusBar);

}

void MainWindow::connectSignals()
{
    connect(m_paramPanel, &ParamPanel::paramsChanged, this, &MainWindow::onParamsChanged);
    connect(m_scene, &SceneWidget::targetsChanged,   this, &MainWindow::onTargetsChanged);
    connect(m_tabs,  &QTabWidget::currentChanged,    this, &MainWindow::onTabChanged);
    connect(m_debounce, &QTimer::timeout, this, [this]{ runUpdate(false); });
    connect(m_watcher, &QFutureWatcher<SimResult>::finished, this, &MainWindow::onComputeFinished);
    connect(m_btnPlay, &QPushButton::clicked, this, &MainWindow::onPlay);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_btnStep, &QPushButton::clicked, this, &MainWindow::onStepFrame);
    connect(m_frameTick, &QTimer::timeout, this, &MainWindow::onFrameTick);
    connect(m_sim, &FMCWSimulator::simResultReady,
            m_tab5, &PipelineInspector::onSimResultReady);
}

void MainWindow::onParamsChanged(RadarParams p)
{
    m_params = p;
    scheduleUpdate();
}

void MainWindow::onTargetsChanged(std::vector<Target> targets)
{
    m_targets = targets;
    scheduleUpdate();
}

void MainWindow::onTabChanged(int)
{
    if (!m_lastResult.range_axis.empty())
        applyResult(m_lastResult);
}

void MainWindow::scheduleUpdate()
{
    if (!m_playing) m_debounce->start();
}

void MainWindow::runUpdate(bool is_step)
{
    if (m_computing) return;
    m_computing = true;
    m_statusBar->setStatus("COMPUTING...");

    RadarParams  p = m_params;
    std::vector<Target> t = m_targets;
    bool step = is_step;
    FMCWSimulator *sim = m_sim;

    auto future = QtConcurrent::run([sim, t, p, step]() -> SimResult {
        return step ? sim->step(t, p) : sim->compute(t, p);
    });
    m_watcher->setFuture(future);
}

void MainWindow::onComputeFinished()
{
    m_computing = false;
    SimResult result = m_watcher->result();
    m_lastResult = result;
    applyResult(result);
    emit m_sim->simResultReady(result);

    int n = (int)result.objects.size();
    m_statusBar->setStatus(QString("OK  — 탐지: %1개  트랙: %2개")
                               .arg(result.detection_points.size())
                               .arg(n), &m_params);
}

void MainWindow::applyResult(const SimResult &result)
{
    int idx = m_tabs->currentIndex();
    switch (idx) {
        case 0: m_tab1->updatePlots(result, m_params, m_targets); break;
        case 1: m_tab2->updatePlots(result, m_params, m_targets); break;
        case 2: m_tab3->updatePlots(result, m_params, m_targets); break;
        case 3: m_tab4->updatePlots(result, m_params, m_targets); break;
        case 4: m_tab5->onSimResultReady(result); break;
    }
}

void MainWindow::onPlay()
{
    m_sim->resetTracker();
    m_frameCount = 0;
    m_playing = true;
    m_btnPlay->setEnabled(false);
    m_btnStop->setEnabled(true);
    m_frameTick->start();
}

void MainWindow::onStop()
{
    m_frameTick->stop();
    m_playing = false;
    m_btnPlay->setEnabled(true);
    m_btnStop->setEnabled(false);
}

void MainWindow::onStepFrame()
{
    m_frameCount++;
    m_frameLabel->setText(QString("FRAME: %1").arg(m_frameCount));
    runUpdate(true);
}

void MainWindow::onFrameTick()
{
    m_frameCount++;
    m_frameLabel->setText(QString("FRAME: %1").arg(m_frameCount));
    runUpdate(true);
}
