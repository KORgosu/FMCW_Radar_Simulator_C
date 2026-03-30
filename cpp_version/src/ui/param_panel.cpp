#include "param_panel.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QScrollArea>
#include <QGroupBox>
#include <QLabel>
#include <cmath>

// ═══════════════════════════════════════════════════════
//  ParamRow
// ═══════════════════════════════════════════════════════

static const QString STYLE_NAME  = "color:#007a20; background:transparent; font-family:'Courier New'; font-size:9pt;";
static const QString STYLE_VALUE = "color:#00ff41; background:transparent; font-family:'Courier New'; font-size:9pt; min-width:60px;";
static const QString STYLE_SLIDER =
    "QSlider::groove:horizontal {"
    "  height:4px; background:#0d2a0d; border-radius:2px; }"
    "QSlider::handle:horizontal {"
    "  background:#00ff41; border:1px solid #00aa30;"
    "  width:10px; height:16px; margin:-6px 0; border-radius:2px; }"
    "QSlider::sub-page:horizontal {"
    "  background:#00aa30; border-radius:2px; }"
    "QSlider { max-height:16px; }";

ParamRow::ParamRow(const QString &label, const QString &unit,
                   double lo, double hi, int steps, double init,
                   bool log_scale, QWidget *parent)
    : QWidget(parent)
    , m_lo(lo), m_hi(hi), m_steps(steps), m_log(log_scale), m_unit(unit)
{
    setStyleSheet("background:transparent;");

    QVBoxLayout *vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(4, 2, 4, 2);
    vlay->setSpacing(2);

    // 상단: 이름 + 값
    QWidget *topRow = new QWidget(this);
    topRow->setStyleSheet("background:transparent;");
    QHBoxLayout *hlay = new QHBoxLayout(topRow);
    hlay->setContentsMargins(0, 0, 0, 0);
    hlay->setSpacing(4);

    QLabel *nameLabel = new QLabel(label, topRow);
    nameLabel->setStyleSheet(STYLE_NAME);
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_valLabel = new QLabel(topRow);
    m_valLabel->setStyleSheet(STYLE_VALUE);
    m_valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    hlay->addWidget(nameLabel);
    hlay->addWidget(m_valLabel);

    // 하단: 슬라이더
    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, m_steps);
    m_slider->setStyleSheet(STYLE_SLIDER);

    vlay->addWidget(topRow);
    vlay->addWidget(m_slider);

    setValue(init);

    connect(m_slider, &QSlider::valueChanged, this, &ParamRow::onSliderMoved);
}

double ParamRow::tickToVal(int tick) const
{
    double t = static_cast<double>(tick) / m_steps;
    if (m_log) {
        double lo_log = std::log10(m_lo);
        double hi_log = std::log10(m_hi);
        return std::pow(10.0, lo_log + t * (hi_log - lo_log));
    }
    return m_lo + t * (m_hi - m_lo);
}

int ParamRow::valToTick(double v) const
{
    double t;
    if (m_log) {
        double lo_log = std::log10(m_lo);
        double hi_log = std::log10(m_hi);
        t = (std::log10(v) - lo_log) / (hi_log - lo_log);
    } else {
        t = (v - m_lo) / (m_hi - m_lo);
    }
    t = std::max(0.0, std::min(1.0, t));
    return static_cast<int>(std::round(t * m_steps));
}

double ParamRow::value() const
{
    return tickToVal(m_slider->value());
}

void ParamRow::setValue(double v)
{
    QSignalBlocker blocker(m_slider);
    m_slider->setValue(valToTick(v));
    // 값 레이블 업데이트
    double val = tickToVal(m_slider->value());
    if (std::abs(val) >= 1e9)
        m_valLabel->setText(QString("%1G%2").arg(val/1e9, 0,'f',2).arg(m_unit));
    else if (std::abs(val) >= 1e6)
        m_valLabel->setText(QString("%1M%2").arg(val/1e6, 0,'f',2).arg(m_unit));
    else if (std::abs(val) >= 1e3)
        m_valLabel->setText(QString("%1k%2").arg(val/1e3, 0,'f',2).arg(m_unit));
    else if (std::abs(val) < 1e-3 && val != 0.0)
        m_valLabel->setText(QString("%1%2").arg(val, 0,'e',2).arg(m_unit));
    else
        m_valLabel->setText(QString("%1%2").arg(val, 0,'f',2).arg(m_unit));
}

void ParamRow::onSliderMoved(int tick)
{
    double v = tickToVal(tick);
    if (std::abs(v) >= 1e9)
        m_valLabel->setText(QString("%1G%2").arg(v/1e9, 0,'f',2).arg(m_unit));
    else if (std::abs(v) >= 1e6)
        m_valLabel->setText(QString("%1M%2").arg(v/1e6, 0,'f',2).arg(m_unit));
    else if (std::abs(v) >= 1e3)
        m_valLabel->setText(QString("%1k%2").arg(v/1e3, 0,'f',2).arg(m_unit));
    else if (std::abs(v) < 1e-3 && v != 0.0)
        m_valLabel->setText(QString("%1%2").arg(v, 0,'e',2).arg(m_unit));
    else
        m_valLabel->setText(QString("%1%2").arg(v, 0,'f',2).arg(m_unit));
    emit valueChanged(v);
}

// ═══════════════════════════════════════════════════════
//  ParamPanel
// ═══════════════════════════════════════════════════════

static const QString PANEL_STYLE =
    "QWidget#ParamPanel { background:#060d06; }"
    "QScrollArea { background:#060d06; border:none; }"
    "QGroupBox {"
    "  color:#00ff41; border:1px solid #00aa30;"
    "  border-radius:4px; margin-top:12px;"
    "  font-family:'Courier New'; font-size:9pt; font-weight:bold; }"
    "QGroupBox::title {"
    "  subcontrol-origin:margin; subcontrol-position:top left;"
    "  padding:0 4px; left:8px; color:#00ff41; }"
    "QScrollBar:vertical {"
    "  background:#060d06; width:8px; }"
    "QScrollBar::handle:vertical {"
    "  background:#00aa30; border-radius:4px; min-height:20px; }"
    "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }";

ParamPanel::ParamPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("ParamPanel");
    setFixedWidth(240);
    setStyleSheet(PANEL_STYLE);

    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(150);
    connect(m_debounceTimer, &QTimer::timeout, this, &ParamPanel::emitParams);

    buildUI();
}

static QGroupBox* makeGroup(const QString &title)
{
    QGroupBox *gb = new QGroupBox(title);
    gb->setStyleSheet(QString()); // 상속
    return gb;
}

void ParamPanel::buildUI()
{
    QVBoxLayout *outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->setSpacing(0);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *container = new QWidget();
    container->setStyleSheet("background:#060d06;");
    QVBoxLayout *lay = new QVBoxLayout(container);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(8);

    // ── 파형 그룹 ──────────────────────────────
    QGroupBox *wfGroup = makeGroup("Waveform");
    QVBoxLayout *wfLay = new QVBoxLayout(wfGroup);
    wfLay->setSpacing(2);

    m_fc     = new ParamRow("fc",      "Hz",  60e9,  100e9, 200, 77e9,  true);
    m_bw     = new ParamRow("B",       "Hz",  10e6,  500e6, 200, 150e6, false);
    m_tc     = new ParamRow("Tc",      "s",   10e-6, 200e-6,200, 40e-6, false);
    m_prf    = new ParamRow("PRF",     "Hz",  100,   5000,  200, 1000,  false);
    m_fs     = new ParamRow("fs",      "Hz",  1e6,   30e6,  200, 15e6,  false);

    // N_chirp: 2의 거듭제곱 (8~256)
    m_nchirp = new ParamRow("N_chirp", "",    8,     256,   5,   64,    false);

    wfLay->addWidget(m_fc);
    wfLay->addWidget(m_bw);
    wfLay->addWidget(m_tc);
    wfLay->addWidget(m_prf);
    wfLay->addWidget(m_fs);
    wfLay->addWidget(m_nchirp);
    lay->addWidget(wfGroup);

    // ── MIMO 그룹 ─────────────────────────────
    QGroupBox *mimoGroup = makeGroup("MIMO");
    QVBoxLayout *mimoLay = new QVBoxLayout(mimoGroup);
    mimoLay->setSpacing(2);

    m_ntx = new ParamRow("N_tx", "", 1, 4, 3, 2, false);
    m_nrx = new ParamRow("N_rx", "", 1, 8, 7, 4, false);

    mimoLay->addWidget(m_ntx);
    mimoLay->addWidget(m_nrx);
    lay->addWidget(mimoGroup);

    // ── 잡음 그룹 ─────────────────────────────
    QGroupBox *noiseGroup = makeGroup("Noise");
    QVBoxLayout *noiseLay = new QVBoxLayout(noiseGroup);
    noiseLay->setSpacing(2);

    m_noise = new ParamRow("noise_std", "", 1e-6, 1e-3, 200, 2e-5, true);

    noiseLay->addWidget(m_noise);
    lay->addWidget(noiseGroup);

    // ── CFAR 그룹 ─────────────────────────────
    QGroupBox *cfarGroup = makeGroup("CFAR");
    QVBoxLayout *cfarLay = new QVBoxLayout(cfarGroup);
    cfarLay->setSpacing(2);

    m_cfar_pfa   = new ParamRow("Pfa",   "",  1e-6, 1e-2, 200, 1e-4, true);
    m_cfar_guard = new ParamRow("guard", "",  1,    8,    7,   2,    false);
    m_cfar_train = new ParamRow("train", "",  4,    32,   28,  8,    false);

    cfarLay->addWidget(m_cfar_pfa);
    cfarLay->addWidget(m_cfar_guard);
    cfarLay->addWidget(m_cfar_train);
    lay->addWidget(cfarGroup);

    lay->addStretch(1);
    scroll->setWidget(container);
    outerLay->addWidget(scroll);

    // 슬라이더 변경 시 디바운스 타이머 재시작
    auto restartTimer = [this](double) { m_debounceTimer->start(); };
    connect(m_fc,         &ParamRow::valueChanged, this, restartTimer);
    connect(m_bw,         &ParamRow::valueChanged, this, restartTimer);
    connect(m_tc,         &ParamRow::valueChanged, this, restartTimer);
    connect(m_prf,        &ParamRow::valueChanged, this, restartTimer);
    connect(m_fs,         &ParamRow::valueChanged, this, restartTimer);
    connect(m_nchirp,     &ParamRow::valueChanged, this, restartTimer);
    connect(m_ntx,        &ParamRow::valueChanged, this, restartTimer);
    connect(m_nrx,        &ParamRow::valueChanged, this, restartTimer);
    connect(m_noise,      &ParamRow::valueChanged, this, restartTimer);
    connect(m_cfar_pfa,   &ParamRow::valueChanged, this, restartTimer);
    connect(m_cfar_guard, &ParamRow::valueChanged, this, restartTimer);
    connect(m_cfar_train, &ParamRow::valueChanged, this, restartTimer);
}

RadarParams ParamPanel::currentParams() const
{
    RadarParams p = params_default();

    p.fc_hz        = m_fc->value();
    p.bandwidth_hz = m_bw->value();
    p.chirp_dur_s  = m_tc->value();
    p.prf_hz       = m_prf->value();
    p.fs_hz        = m_fs->value();

    // N_chirp: 2의 거듭제곱으로 스냅
    int raw = static_cast<int>(std::round(m_nchirp->value()));
    int snap = 1;
    while (snap * 2 <= raw) snap *= 2;
    if (std::abs(raw - snap) > std::abs(raw - snap*2)) snap *= 2;
    p.n_chirp = std::max(8, std::min(256, snap));

    p.n_tx        = static_cast<int>(std::round(m_ntx->value()));
    p.n_rx        = static_cast<int>(std::round(m_nrx->value()));
    p.noise_std   = m_noise->value();
    p.cfar_pfa    = m_cfar_pfa->value();
    p.cfar_guard  = static_cast<int>(std::round(m_cfar_guard->value()));
    p.cfar_train  = static_cast<int>(std::round(m_cfar_train->value()));

    return p;
}

void ParamPanel::emitParams()
{
    emit paramsChanged(currentParams());
}
