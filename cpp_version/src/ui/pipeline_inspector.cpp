#include "pipeline_inspector.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QFrame>
#include <QFont>
#include <QGroupBox>
#include <cmath>
#include <algorithm>

/* ── 팔레트 ─────────────────────────────────────────────────── */
static const QColor COL_BG    ("#060d06");
static const QColor COL_GRID  ("#0d2a0d");
static const QColor COL_GREEN ("#00ff41");
static const QColor COL_DIM   ("#007a20");
static const QColor COL_MID   ("#00aa30");
static const QColor COL_CYAN  ("#00ffff");
static const QColor COL_ORANGE("#ffaa00");
static const QColor COL_PANEL ("#0a180a");

static const QString STYLE_SHAPE =
    "color:#00ccff; font-family:'Courier New'; font-size:9pt; font-weight:bold;"
    " background:#061206; border:1px solid #0d3a3a; padding:3px 6px;";

static const QString STYLE_FORMULA =
    "color:#00ff41; font-family:'Courier New'; font-size:9pt;"
    " background:#070f07; border:1px solid #0d2a0d;"
    " padding:6px; line-height:160%;";

static const QString STYLE_DESC =
    "color:#00aa30; font-family:'Courier New'; font-size:8pt;"
    " background:transparent; padding:2px 0px;";

static const QString STYLE_STAGE_TITLE =
    "color:#00ffcc; font-family:'Segoe UI'; font-size:12pt; font-weight:bold;"
    " background:transparent; padding:2px 0px;";

static const QString STYLE_TRANSFORM =
    "color:#ffaa00; font-family:'Courier New'; font-size:9pt;"
    " background:#120e00; border:1px solid #3a2a00; padding:4px 8px;";

/* ═══════════════════════════════════════════════════════════════
   StageView
   ═══════════════════════════════════════════════════════════════ */

void StageView::applyDarkTheme(QCustomPlot *plot)
{
    plot->setBackground(QBrush(COL_BG));
    plot->axisRect()->setBackground(QBrush(COL_BG));
    for (auto *axis : plot->axisRect()->axes()) {
        axis->setBasePen(QPen(COL_DIM));
        axis->setTickPen(QPen(COL_DIM));
        axis->setSubTickPen(QPen(COL_DIM));
        axis->setTickLabelColor(COL_DIM);
        axis->setLabelColor(COL_MID);
        axis->grid()->setPen(QPen(COL_GRID, 1, Qt::DotLine));
        axis->grid()->setSubGridVisible(false);
    }
}

/* 패널 박스 (테두리 색 구분) */
QWidget *StageView::makePanelBox(const QString &title,
                                  const QColor  &borderColor,
                                  QWidget       *content)
{
    auto *box = new QGroupBox(title);
    QString hex = borderColor.name();
    box->setStyleSheet(QString(
        "QGroupBox { color:%1; font-family:'Segoe UI'; font-size:10pt; font-weight:bold;"
        " border:2px solid %1; border-radius:4px; margin-top:8px; background:#060d06; }"
        "QGroupBox::title { subcontrol-origin:margin; left:10px; padding:0 4px; }").arg(hex));
    auto *lay = new QVBoxLayout(box);
    lay->setContentsMargins(6, 8, 6, 6);
    lay->addWidget(content);
    return box;
}

StageView::StageView(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background:#060d06;");

    /* ── INPUT 패널 내용 ── */
    auto *inputContent = new QWidget();
    auto *inLay = new QVBoxLayout(inputContent);
    inLay->setContentsMargins(0,0,0,0);
    inLay->setSpacing(4);
    m_inputShapeLbl = new QLabel("...");
    m_inputShapeLbl->setStyleSheet(STYLE_SHAPE);
    m_inputShapeLbl->setAlignment(Qt::AlignCenter);
    m_inputPlot = new QCustomPlot();
    m_inputPlot->setMinimumHeight(180);
    applyDarkTheme(m_inputPlot);
    inLay->addWidget(m_inputShapeLbl);
    inLay->addWidget(m_inputPlot, 1);

    /* ── PROCESS 패널 내용 ── */
    auto *procContent = new QWidget();
    auto *prLay = new QVBoxLayout(procContent);
    prLay->setContentsMargins(4, 4, 4, 4);
    prLay->setSpacing(8);

    m_stageNameLbl = new QLabel();
    m_stageNameLbl->setStyleSheet(STYLE_STAGE_TITLE);
    m_stageNameLbl->setAlignment(Qt::AlignCenter);
    m_stageNameLbl->setWordWrap(true);

    auto *arrowTop = new QLabel("▼  INPUT");
    arrowTop->setStyleSheet("color:#007a20; font-family:'Courier New';"
                            " font-size:8pt; background:transparent;");
    arrowTop->setAlignment(Qt::AlignCenter);

    m_formulaLbl = new QLabel();
    m_formulaLbl->setStyleSheet(STYLE_FORMULA);
    m_formulaLbl->setWordWrap(true);
    m_formulaLbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_formulaLbl->setMinimumHeight(80);

    m_descLbl = new QLabel();
    m_descLbl->setStyleSheet(STYLE_DESC);
    m_descLbl->setWordWrap(true);

    m_transformLbl = new QLabel();
    m_transformLbl->setStyleSheet(STYLE_TRANSFORM);
    m_transformLbl->setWordWrap(true);
    m_transformLbl->setAlignment(Qt::AlignCenter);

    auto *arrowBot = new QLabel("▼  OUTPUT");
    arrowBot->setStyleSheet(arrowTop->styleSheet());
    arrowBot->setAlignment(Qt::AlignCenter);

    prLay->addWidget(m_stageNameLbl);
    prLay->addWidget(arrowTop);
    prLay->addWidget(m_formulaLbl);
    prLay->addWidget(m_descLbl);
    prLay->addStretch();
    prLay->addWidget(m_transformLbl);
    prLay->addWidget(arrowBot);

    /* ── OUTPUT 패널 내용 ── */
    auto *outputContent = new QWidget();
    auto *outLay = new QVBoxLayout(outputContent);
    outLay->setContentsMargins(0,0,0,0);
    outLay->setSpacing(4);
    m_outputShapeLbl = new QLabel("...");
    m_outputShapeLbl->setStyleSheet(STYLE_SHAPE);
    m_outputShapeLbl->setAlignment(Qt::AlignCenter);
    m_outputPlot = new QCustomPlot();
    m_outputPlot->setMinimumHeight(180);
    applyDarkTheme(m_outputPlot);
    outLay->addWidget(m_outputShapeLbl);
    outLay->addWidget(m_outputPlot, 1);

    /* ── 3단 스플리터 ── */
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(2);
    splitter->setStyleSheet("QSplitter::handle { background:#0d2a0d; }");
    splitter->addWidget(makePanelBox("  INPUT",   QColor("#00aaff"), inputContent));
    splitter->addWidget(makePanelBox("  PROCESS", QColor("#00cc66"), procContent));
    splitter->addWidget(makePanelBox("  OUTPUT",  QColor("#ffaa00"), outputContent));
    splitter->setSizes({380, 280, 380});

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->addWidget(splitter);
}

void StageView::setMeta(const QString &stageName,
                         const QString &inputShape,
                         const QString &formula,
                         const QString &description,
                         const QString &outputShape)
{
    m_stageNameLbl->setText(stageName);
    m_inputShapeLbl->setText(inputShape);
    m_formulaLbl->setText(formula);
    m_descLbl->setText(description);
    m_transformLbl->setText(inputShape + "  →  " + outputShape);
    m_outputShapeLbl->setText(outputShape);
}

/* ═══════════════════════════════════════════════════════════════
   PipelineInspector
   ═══════════════════════════════════════════════════════════════ */

/* 공용 컬러맵 채우기 헬퍼 */
void PipelineInspector::fillColorMap(QCustomPlot *plot,
                                      const std::vector<double> &data,
                                      int rows, int cols,
                                      double xMin, double xMax,
                                      double yMin, double yMax,
                                      const QString &xLabel,
                                      const QString &yLabel,
                                      QCPColorGradient::GradientPreset preset)
{
    plot->clearPlottables();
    if (data.empty() || rows <= 0 || cols <= 0) { plot->replot(); return; }
    auto *cm = new QCPColorMap(plot->xAxis, plot->yAxis);
    cm->data()->setSize(cols, rows);
    cm->data()->setRange(QCPRange(xMin, xMax), QCPRange(yMin, yMax));
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            cm->data()->setCell(c, r, data[r * cols + c]);
    QCPColorGradient grad;
    grad.loadPreset(preset);
    cm->setGradient(grad);
    cm->rescaleDataRange();
    plot->xAxis->setLabel(xLabel);
    plot->yAxis->setLabel(yLabel);
    plot->rescaleAxes();
    plot->replot();
}

PipelineInspector::PipelineInspector(QWidget *parent) : QWidget(parent)
{
    setStyleSheet("background:#060d06; color:#00ff41;");
    buildUI();
}

void PipelineInspector::buildUI()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(6);

    /* ── 좌측 단계 목록 ── */
    m_stageList = new QListWidget();
    m_stageList->setFixedWidth(170);
    m_stageList->setStyleSheet(
        "QListWidget { background:#060d06; border:1px solid #0d2a0d;"
        " color:#007a20; font-family:'Courier New'; font-size:9pt; }"
        "QListWidget::item { padding:6px 8px; border-bottom:1px solid #0a1a0a; }"
        "QListWidget::item:selected { background:#0d2a0d; color:#00ff41; }"
        "QListWidget::item:hover { background:#0a1a0a; color:#00cc44; }");

    const struct { const char *name; const char *sub; } stageInfo[] = {
        { "Stage 0", "RF Frontend"    },
        { "Stage 1", "Range FFT"      },
        { "Stage 2", "Doppler FFT"    },
        { "Stage 3", "Angle FFT"      },
        { "Stage 4", "CFAR"           },
        { "Stage 5", "Clustering"     },
        { "Stage 6", "EKF Tracking"   },
    };
    for (const auto &s : stageInfo) {
        auto *item = new QListWidgetItem(
            QString("%1\n  %2").arg(s.name, s.sub));
        item->setSizeHint(QSize(160, 44));
        m_stageList->addItem(item);
    }
    m_stageList->setCurrentRow(0);

    /* ── 우측 스택 ── */
    m_stack = new QStackedWidget();

    struct StageMeta {
        const char *name, *inputShape, *formula, *desc, *outputShape;
    } metas[] = {
        {
            "Stage 0 — RF Frontend (Beat Signal 생성)",
            "RadarParams + Targets\n(fc, B, Tc, Nc, Ns, Nv)",
            "TX: s_tx(t) = exp(j·2π·(fc·t + μ·t²/2))\n"
            "Beat: s_beat(t) = A·exp(j·2π·fb·t + jφ)\n"
            "fb = 2R·μ/c,   φ = −4πR/λ\n"
            "μ = B/Tc  (처프 레이트)",
            "· TX 처프 신호와 지연된 RX 신호를\n  믹서로 곱해 Beat 신호 생성\n"
            "· 각 타겟마다 고유한 beat 주파수 fb\n"
            "· MIMO 가상 배열 Nv = Ntx × Nrx",
            "Beat Cube [Nc × Ns × Nv]\n(chirp × sample × virtual antenna)"
        },
        {
            "Stage 1 — Range FFT",
            "Beat Cube [Nc × Ns × Nv]\n채널 0 추출 → Beat Matrix [Nc × Ns]",
            "w[n] = 0.5·(1 − cos(2πn/(N−1)))  ← Hanning\n"
            "X[k] = FFT(w·s_beat, Nfft)\n"
            "R[k] = k·c / (2·μ·Nfft·Ts)\n"
            "Nr = Nfft/2  (양의 주파수만)",
            "· Fast-time 방향(샘플 축)으로 FFT\n"
            "· Hanning 윈도우로 사이드로브 억제\n"
            "· 각 처프마다 독립 수행 (Nc번)\n"
            "· 거리 분해능 ΔR = c/(2B)",
            "Range FFT Matrix [Nc × Nr]\n+ Range Profile [Nr]  (평균 dB)"
        },
        {
            "Stage 2 — Doppler FFT (RDM 생성)",
            "Range FFT Matrix [Nc × Nr]\n(slow-time 방향으로 처리)",
            "w_s[m] = 0.5·(1 − cos(2πm/(Nc−1)))  ← Hanning\n"
            "RDM[kd][kr] = FFTshift(FFT(X[:,kr], Nd))[kd]\n"
            "v = −kd·λ / (2·Nc·TPRI)\n"
            "fd = 2·v/λ  (도플러 주파수)",
            "· Slow-time 방향(처프 축)으로 FFT\n"
            "· 각 거리 빈마다 Nc 포인트 FFT\n"
            "· FFTshift로 음/양 속도 중심 배치\n"
            "· 속도 분해능 Δv = λ/(2·Nc·TPRI)",
            "Range-Doppler Map [Nd × Nr]\n(2D 복소 스펙트럼 + dB 변환)"
        },
        {
            "Stage 3 — AoA FFT (각도 추정)",
            "RDM Cube [Nd × Nr × Nv]\n(모든 가상 채널 포함)",
            "va_sig[nv] = RDM_cube[kd][kr][nv]\n"
            "AoA[ka] = FFTshift(FFT(va_sig, Na))[ka]\n"
            "sin(θ) = ka·λ / (Na·d_elem)\n"
            "d_elem = λ/2  (ULA 가정)",
            "· CFAR 탐지된 셀에서만 AoA FFT 수행\n"
            "· 가상 배열(ULA) 신호에 공간 FFT 적용\n"
            "· 피크 탐색으로 도달각(AoA) 추정\n"
            "· 각도 분해능 Δθ ≈ λ/(Na·d_elem)",
            "DetectionPoints[]\n(range, velocity, angle, power)"
        },
        {
            "Stage 4 — CFAR 탐지",
            "RDM Power Map [Nd × Nr]\n|RDM[kd][kr]|²",
            "N_train = (2(g+t)+1)² − (2g+1)²\n"
            "α = N_train · (Pfa^(−1/N_train) − 1)\n"
            "noise = mean(training cells)\n"
            "탐지: |X[kd][kr]|² > α · noise",
            "· 2D CA-CFAR (Cell-Averaging)\n"
            "· Guard cell g개로 타겟 누출 방지\n"
            "· Summed Area Table로 O(1) 계산\n"
            "· Pfa = 오경보 확률 (e.g. 1e-4)",
            "Detection Map [Nd × Nr]\nbool + Threshold Map"
        },
        {
            "Stage 5 — DBSCAN Clustering",
            "DetectionPoints[]\n(range, velocity, angle 3D 공간)",
            "정규화 거리:\nd = √((ΔR/Rmax)² + (Δv/2Vmax)² + (Δθ/180)²)\n"
            "DBSCAN: ε = 0.1,  MinPts = 2\n"
            "군집 = {p | d(p, q) ≤ ε}",
            "· 3D 정규화 공간에서 밀도 기반 군집화\n"
            "· 잡음(noise) 포인트 자동 제거\n"
            "· 군집 무게중심이 다음 단계 입력\n"
            "· 군집 수 = 실제 탐지 타겟 수 근사",
            "Clusters[]\n(centroid: range, velocity, angle)"
        },
        {
            "Stage 6 — EKF Tracking",
            "Clusters[] + TrackState[]\n상태 벡터 x = [x, y, vx, vy]ᵀ",
            "Predict: x̂⁻ = F·x̂,  P⁻ = F·P·Fᵀ + Q\n"
            "혁신: z̃ = z − h(x̂⁻)\n"
            "칼만 이득: K = P⁻·Hᵀ·(H·P⁻·Hᵀ+R)⁻¹\n"
            "Update: x̂ = x̂⁻ + K·z̃,  P = (I−KH)·P⁻",
            "· 비선형 측정 모델 h(x) = [R, Ṙ, θ]\n"
            "· Hungarian 알고리즘으로 데이터 연계\n"
            "· hit_count ≥ 3 이면 CONFIRMED 트랙\n"
            "· miss_count ≥ 3 이면 트랙 소멸",
            "ObjectInfo[]\n(id, range, velocity, angle, P matrix)"
        },
    };

    for (int i = 0; i < 7; i++) {
        m_views[i] = new StageView();
        m_views[i]->setMeta(
            metas[i].name, metas[i].inputShape,
            metas[i].formula, metas[i].desc,
            metas[i].outputShape);
        m_stack->addWidget(m_views[i]);
    }

    layout->addWidget(m_stageList);
    layout->addWidget(m_stack, 1);

    connect(m_stageList, &QListWidget::currentRowChanged,
            this, &PipelineInspector::onStageSelected);
}

void PipelineInspector::onStageSelected(int row)
{
    m_currentStage = row;
    m_stack->setCurrentIndex(row);
    if (!m_lastResult.range_axis.empty())
        updateStage(row, m_lastResult);
}

void PipelineInspector::onSimResultReady(SimResult result)
{
    m_lastResult = result;
    updateStage(m_currentStage, result);
}

void PipelineInspector::updateStage(int stage, const SimResult &res)
{
    switch (stage) {
        case 0: showStage0(res); break;
        case 1: showStage1(res); break;
        case 2: showStage2(res); break;
        case 3: showStage3(res); break;
        case 4: showStage4(res); break;
        case 5: showStage5(res); break;
        case 6: showStage6(res); break;
    }
}

/* ═══════════════════════════════════════════════════════════════
   Stage 0 — RF Frontend
   INPUT : TX 처프 순간 주파수
   OUTPUT: Beat 신호 (Re/Im, chirp 0)
   ═══════════════════════════════════════════════════════════════ */
void PipelineInspector::showStage0(const SimResult &res)
{
    auto *v  = m_views[0];
    auto *ip = v->inputPlot();
    auto *op = v->outputPlot();

    /* INPUT: TX 처프 순간 주파수 vs 시간 */
    ip->clearGraphs();
    ip->clearPlottables();
    if (!res.t_fast.empty() && !res.tx_freq.empty()) {
        ip->addGraph();
        ip->graph(0)->setPen(QPen(COL_CYAN, 1.5));
        ip->graph(0)->setName("f_inst (MHz)");
        QVector<double> t(res.t_fast.begin(), res.t_fast.end());
        QVector<double> f(res.tx_freq.size());
        for (int i = 0; i < (int)res.tx_freq.size(); i++)
            f[i] = res.tx_freq[i] / 1e6;
        ip->graph(0)->setData(t, f);
        ip->xAxis->setLabel("Time (s)");
        ip->yAxis->setLabel("Freq (MHz)");
        ip->xAxis->setRange(t.first(), t.last());
        ip->rescaleAxes();
    }
    ip->replot();

    /* OUTPUT: Beat 신호 Re/Im (chirp 0) */
    op->clearGraphs();
    op->clearPlottables();
    if (!res.beat_one_re.empty()) {
        QVector<double> t(res.t_fast.begin(), res.t_fast.end());
        QVector<double> re(res.beat_one_re.begin(), res.beat_one_re.end());
        QVector<double> im(res.beat_one_im.begin(), res.beat_one_im.end());
        op->addGraph(); op->addGraph();
        op->graph(0)->setPen(QPen(COL_GREEN, 1.5)); op->graph(0)->setName("Re");
        op->graph(1)->setPen(QPen(COL_CYAN,  1.0)); op->graph(1)->setName("Im");
        op->graph(0)->setData(t, re);
        op->graph(1)->setData(t, im);
        op->legend->setVisible(true);
        op->legend->setBrush(QBrush(COL_BG));
        op->legend->setTextColor(COL_MID);
        op->legend->setBorderPen(QPen(COL_GRID));
        op->xAxis->setLabel("Time (s)");
        op->yAxis->setLabel("Amplitude");
        op->rescaleAxes();
    }
    op->replot();
}

/* ═══════════════════════════════════════════════════════════════
   Stage 1 — Range FFT
   INPUT : Beat Matrix 히트맵 [Nc × Ns]
   OUTPUT: Range Profile dB
   ═══════════════════════════════════════════════════════════════ */
void PipelineInspector::showStage1(const SimResult &res)
{
    auto *v = m_views[1];

    /* INPUT: Beat Matrix (Real part) 컬러맵 */
    fillColorMap(v->inputPlot(),
                 res.beat_mat_re, res.beat_rows, res.beat_cols,
                 0, res.beat_cols, 0, res.beat_rows,
                 "Sample Index", "Chirp Index",
                 QCPColorGradient::gpHot);

    /* OUTPUT: Range Profile dB */
    auto *op = v->outputPlot();
    op->clearGraphs();
    op->clearPlottables();
    if (!res.range_profile_db.empty() && !res.range_axis.empty()) {
        op->addGraph();
        op->graph(0)->setPen(QPen(COL_GREEN, 1.5));
        op->graph(0)->setName("Range Profile");
        QVector<double> r(res.range_axis.begin(), res.range_axis.end());
        QVector<double> db(res.range_profile_db.begin(), res.range_profile_db.end());
        op->graph(0)->setData(r, db);
        /* 피크 표시 */
        double maxDb = *std::max_element(db.begin(), db.end());
        for (int i = 0; i < (int)db.size(); i++) {
            if (db[i] > maxDb - 3.0) {
                auto *line = new QCPItemLine(op);
                line->start->setCoords(r[i], op->yAxis->range().lower);
                line->end->setCoords(r[i], db[i]);
                line->setPen(QPen(COL_ORANGE, 1, Qt::DashLine));
            }
        }
        op->xAxis->setLabel("Range (m)");
        op->yAxis->setLabel("dB");
        op->rescaleAxes();
    }
    op->replot();
}

/* ═══════════════════════════════════════════════════════════════
   Stage 2 — Doppler FFT
   INPUT : Range Profile (dB)
   OUTPUT: Range-Doppler Map 컬러맵
   ═══════════════════════════════════════════════════════════════ */
void PipelineInspector::showStage2(const SimResult &res)
{
    auto *v = m_views[2];

    /* INPUT: Range Profile */
    auto *ip = v->inputPlot();
    ip->clearGraphs(); ip->clearPlottables();
    if (!res.range_profile_db.empty() && !res.range_axis.empty()) {
        ip->addGraph();
        ip->graph(0)->setPen(QPen(COL_GREEN, 1.5));
        QVector<double> r(res.range_axis.begin(), res.range_axis.end());
        QVector<double> db(res.range_profile_db.begin(), res.range_profile_db.end());
        ip->graph(0)->setData(r, db);
        ip->xAxis->setLabel("Range (m)");
        ip->yAxis->setLabel("dB");
        ip->rescaleAxes();
    }
    ip->replot();

    /* OUTPUT: RDM 컬러맵 */
    if (!res.rdm_db.empty() && !res.range_axis.empty() && !res.velocity_axis.empty())
        fillColorMap(v->outputPlot(),
                     res.rdm_db, res.rdm_rows, res.rdm_cols,
                     res.range_axis.front(), res.range_axis.back(),
                     res.velocity_axis.front(), res.velocity_axis.back(),
                     "Range (m)", "Velocity (m/s)",
                     QCPColorGradient::gpJet);
    else
        v->outputPlot()->replot();
}

/* ═══════════════════════════════════════════════════════════════
   Stage 3 — AoA FFT
   INPUT : RDM 컬러맵
   OUTPUT: Angle-Range scatter (탐지 포인트)
   ═══════════════════════════════════════════════════════════════ */
void PipelineInspector::showStage3(const SimResult &res)
{
    auto *v = m_views[3];

    /* INPUT: RDM 컬러맵 */
    if (!res.rdm_db.empty() && !res.range_axis.empty() && !res.velocity_axis.empty())
        fillColorMap(v->inputPlot(),
                     res.rdm_db, res.rdm_rows, res.rdm_cols,
                     res.range_axis.front(), res.range_axis.back(),
                     res.velocity_axis.front(), res.velocity_axis.back(),
                     "Range (m)", "Velocity (m/s)",
                     QCPColorGradient::gpJet);

    /* OUTPUT: Angle-Range scatter */
    auto *op = v->outputPlot();
    op->clearGraphs(); op->clearPlottables();
    op->addGraph();
    op->graph(0)->setLineStyle(QCPGraph::lsNone);
    op->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssCircle, COL_GREEN, QColor(0,255,65,60), 8));
    op->graph(0)->setName(QString("N=%1 pts").arg(res.detection_points.size()));
    QVector<double> angles, ranges;
    for (const auto &pt : res.detection_points) {
        angles.append(pt.angle_deg);
        ranges.append(pt.range_m);
    }
    op->graph(0)->setData(angles, ranges);
    op->legend->setVisible(true);
    op->legend->setBrush(QBrush(COL_BG));
    op->legend->setTextColor(COL_MID);
    op->legend->setBorderPen(QPen(COL_GRID));
    op->xAxis->setLabel("Angle (deg)");
    op->yAxis->setLabel("Range (m)");
    if (!angles.isEmpty()) op->rescaleAxes();
    else { op->xAxis->setRange(-90, 90); op->yAxis->setRange(0, 200); }
    op->replot();
}

/* ═══════════════════════════════════════════════════════════════
   Stage 4 — CFAR
   INPUT : RDM Power 히트맵
   OUTPUT: CFAR 탐지 맵 (탐지 셀 scatter)
   ═══════════════════════════════════════════════════════════════ */
void PipelineInspector::showStage4(const SimResult &res)
{
    auto *v = m_views[4];

    /* INPUT: RDM power (log scale) */
    std::vector<double> rdmPowDb(res.rdm_pow.size());
    for (int i = 0; i < (int)res.rdm_pow.size(); i++)
        rdmPowDb[i] = (res.rdm_pow[i] > 0) ? 10.0 * std::log10(res.rdm_pow[i] + 1e-30) : -80.0;
    if (!rdmPowDb.empty() && !res.range_axis.empty() && !res.velocity_axis.empty())
        fillColorMap(v->inputPlot(),
                     rdmPowDb, res.rdm_rows, res.rdm_cols,
                     res.range_axis.front(), res.range_axis.back(),
                     res.velocity_axis.front(), res.velocity_axis.back(),
                     "Range (m)", "Velocity (m/s)",
                     QCPColorGradient::gpHot);

    /* OUTPUT: 탐지된 셀 scatter */
    auto *op = v->outputPlot();
    op->clearGraphs(); op->clearPlottables();
    op->addGraph();
    op->graph(0)->setLineStyle(QCPGraph::lsNone);
    op->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssSquare, COL_GREEN, QColor(0,255,65,80), 6));
    QVector<double> dr, dv;
    for (int rd = 0; rd < res.rdm_rows; rd++)
        for (int rr = 0; rr < res.rdm_cols; rr++) {
            int idx = rd * res.rdm_cols + rr;
            if (idx < (int)res.cfar_detections.size() && res.cfar_detections[idx]) {
                if (rr < (int)res.range_axis.size())    dr.append(res.range_axis[rr]);
                if (rd < (int)res.velocity_axis.size()) dv.append(res.velocity_axis[rd]);
            }
        }
    op->graph(0)->setData(dr, dv);
    op->graph(0)->setName(QString("Det: %1").arg(dr.size()));
    op->legend->setVisible(true);
    op->legend->setBrush(QBrush(COL_BG));
    op->legend->setTextColor(COL_MID);
    op->legend->setBorderPen(QPen(COL_GRID));
    op->xAxis->setLabel("Range (m)");
    op->yAxis->setLabel("Velocity (m/s)");
    if (!dr.isEmpty()) op->rescaleAxes();
    else if (!res.range_axis.empty() && !res.velocity_axis.empty()) {
        op->xAxis->setRange(res.range_axis.front(), res.range_axis.back());
        op->yAxis->setRange(res.velocity_axis.front(), res.velocity_axis.back());
    }
    op->replot();
}

/* ═══════════════════════════════════════════════════════════════
   Stage 5 — DBSCAN Clustering
   INPUT : 탐지 포인트 Range-Velocity scatter
   OUTPUT: 클러스터 무게중심 + 포인트 동시 표시
   ═══════════════════════════════════════════════════════════════ */
void PipelineInspector::showStage5(const SimResult &res)
{
    auto *v = m_views[5];

    /* INPUT: 탐지 포인트 scatter (Range vs Velocity) */
    auto *ip = v->inputPlot();
    ip->clearGraphs(); ip->clearPlottables();
    ip->addGraph();
    ip->graph(0)->setLineStyle(QCPGraph::lsNone);
    ip->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssPlus, COL_ORANGE, 6));
    ip->graph(0)->setName(QString("%1 pts").arg(res.detection_points.size()));
    QVector<double> pr, pv;
    for (const auto &pt : res.detection_points) {
        pr.append(pt.range_m); pv.append(pt.velocity_mps);
    }
    ip->graph(0)->setData(pr, pv);
    ip->legend->setVisible(true);
    ip->legend->setBrush(QBrush(COL_BG));
    ip->legend->setTextColor(COL_MID);
    ip->legend->setBorderPen(QPen(COL_GRID));
    ip->xAxis->setLabel("Range (m)");
    ip->yAxis->setLabel("Velocity (m/s)");
    if (!pr.isEmpty()) ip->rescaleAxes();
    else { ip->xAxis->setRange(0,200); ip->yAxis->setRange(-30,30); }
    ip->replot();

    /* OUTPUT: 포인트(회색) + 클러스터 무게중심(색상) */
    auto *op = v->outputPlot();
    op->clearGraphs(); op->clearPlottables();
    /* 포인트 배경 */
    op->addGraph();
    op->graph(0)->setLineStyle(QCPGraph::lsNone);
    op->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssPlus, QColor(0,100,30), 5));
    op->graph(0)->setData(pr, pv);
    /* 클러스터 무게중심 */
    op->addGraph();
    op->graph(1)->setLineStyle(QCPGraph::lsNone);
    op->graph(1)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssCircle, COL_GREEN, QColor(0,255,65,100), 14));
    op->graph(1)->setName(QString("Clusters: %1").arg(res.clusters.size()));
    QVector<double> cr, cv;
    for (const auto &cl : res.clusters) {
        cr.append(cl.range_m); cv.append(cl.velocity_mps);
    }
    op->graph(1)->setData(cr, cv);
    op->legend->setVisible(true);
    op->legend->setBrush(QBrush(COL_BG));
    op->legend->setTextColor(COL_MID);
    op->legend->setBorderPen(QPen(COL_GRID));
    op->xAxis->setLabel("Range (m)");
    op->yAxis->setLabel("Velocity (m/s)");
    if (!pr.isEmpty()) op->rescaleAxes();
    else { op->xAxis->setRange(0,200); op->yAxis->setRange(-30,30); }
    op->replot();
}

/* ═══════════════════════════════════════════════════════════════
   Stage 6 — EKF Tracking
   INPUT : 클러스터 무게중심 (Range-Velocity)
   OUTPUT: 트랙 상태 (공분산 타원 + 텍스트 정보)
   ═══════════════════════════════════════════════════════════════ */
void PipelineInspector::showStage6(const SimResult &res)
{
    auto *v = m_views[6];

    /* INPUT: 클러스터 scatter */
    auto *ip = v->inputPlot();
    ip->clearGraphs(); ip->clearPlottables();
    ip->addGraph();
    ip->graph(0)->setLineStyle(QCPGraph::lsNone);
    ip->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssCircle, COL_ORANGE, QColor(255,170,0,80), 12));
    ip->graph(0)->setName(QString("Clusters: %1").arg(res.clusters.size()));
    QVector<double> cr, cv;
    for (const auto &cl : res.clusters) {
        cr.append(cl.range_m); cv.append(cl.velocity_mps);
    }
    ip->graph(0)->setData(cr, cv);
    ip->legend->setVisible(true);
    ip->legend->setBrush(QBrush(COL_BG));
    ip->legend->setTextColor(COL_MID);
    ip->legend->setBorderPen(QPen(COL_GRID));
    ip->xAxis->setLabel("Range (m)");
    ip->yAxis->setLabel("Velocity (m/s)");
    if (!cr.isEmpty()) ip->rescaleAxes();
    else { ip->xAxis->setRange(0,200); ip->yAxis->setRange(-30,30); }
    ip->replot();

    /* OUTPUT: 트랙 상태 텍스트 + scatter */
    auto *op = v->outputPlot();
    op->clearGraphs(); op->clearItems(); op->clearPlottables();

    QVector<double> ox, ov_conf, ox2, ov2;
    for (const auto &obj : res.objects) {
        if (obj.confirmed) { ox.append(obj.range_m);  ov_conf.append(obj.velocity_mps); }
        else               { ox2.append(obj.range_m); ov2.append(obj.velocity_mps); }
    }
    /* CONFIRMED */
    op->addGraph();
    op->graph(0)->setLineStyle(QCPGraph::lsNone);
    op->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssStar, COL_CYAN, 16));
    op->graph(0)->setName(QString("Confirmed: %1").arg(ox.size()));
    op->graph(0)->setData(ox, ov_conf);
    /* TENTATIVE */
    op->addGraph();
    op->graph(1)->setLineStyle(QCPGraph::lsNone);
    op->graph(1)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssCircle, COL_ORANGE, 10));
    op->graph(1)->setName(QString("Tentative: %1").arg(ox2.size()));
    op->graph(1)->setData(ox2, ov2);

    /* 트랙 ID 텍스트 */
    for (const auto &obj : res.objects) {
        auto *txt = new QCPItemText(op);
        txt->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);
        txt->position->setType(QCPItemPosition::ptPlotCoords);
        txt->position->setCoords(obj.range_m, obj.velocity_mps);
        txt->setText(QString(" ID%1").arg(obj.id));
        txt->setColor(obj.confirmed ? COL_CYAN : COL_ORANGE);
        txt->setFont(QFont("Courier New", 8));
    }

    if (res.objects.empty()) {
        auto *txt = new QCPItemText(op);
        txt->setPositionAlignment(Qt::AlignCenter);
        txt->position->setType(QCPItemPosition::ptAxisRectRatio);
        txt->position->setCoords(0.5, 0.5);
        txt->setText("NO TRACKS");
        txt->setColor(COL_DIM);
        txt->setFont(QFont("Segoe UI", 14, QFont::Bold));
    }

    op->legend->setVisible(true);
    op->legend->setBrush(QBrush(COL_BG));
    op->legend->setTextColor(COL_MID);
    op->legend->setBorderPen(QPen(COL_GRID));
    op->xAxis->setLabel("Range (m)");
    op->yAxis->setLabel("Velocity (m/s)");
    if (!ox.isEmpty() || !ox2.isEmpty()) op->rescaleAxes();
    else { op->xAxis->setRange(0,200); op->yAxis->setRange(-30,30); }
    op->replot();
}
