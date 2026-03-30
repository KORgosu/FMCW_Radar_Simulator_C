#include "scene_widget.h"

#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <cmath>

// ═══════════════════════════════════════════════════════
//  SceneWidget
// ═══════════════════════════════════════════════════════

SceneWidget::SceneWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(300, 300);
    setMouseTracking(true);
    setStyleSheet("background:#060d06;");

    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(50);   // 20 FPS 애니메이션
    connect(m_animTimer, &QTimer::timeout, this, &SceneWidget::onAnimTick);
    m_animTimer->start();
    m_elapsed.start();
}

// ── 좌표 변환 헬퍼 ────────────────────────────────────

QPointF SceneWidget::radarOrigin() const
{
    return QPointF(width() / 2.0, height() - 28.0);
}

double SceneWidget::scale() const
{
    // 최대 거리가 화면 높이의 90% 를 차지하도록
    double avail = (height() - 28.0) * 0.90;
    return avail / MAX_RANGE;
}

// 극좌표(거리, 각도) → 픽셀
// angle_deg: 정면(위쪽)=0, 오른쪽=양수
QPointF SceneWidget::polarToPx(double r, double angle_deg) const
{
    double rad = angle_deg * M_PI / 180.0;
    double sc  = scale();
    QPointF origin = radarOrigin();
    return QPointF(origin.x() + r * sc * std::sin(rad),
                   origin.y() - r * sc * std::cos(rad));
}

bool SceneWidget::pxToPolar(QPointF px, double &r, double &angle_deg) const
{
    QPointF origin = radarOrigin();
    double sc = scale();
    double dx = (px.x() - origin.x()) / sc;
    double dy = -(px.y() - origin.y()) / sc;
    r = std::sqrt(dx*dx + dy*dy);
    angle_deg = std::atan2(dx, dy) * 180.0 / M_PI;
    return (r >= 0.0 && r <= MAX_RANGE);
}

int SceneWidget::hitTest(QPointF px) const
{
    const double HIT_RADIUS = 10.0;
    for (int i = 0; i < static_cast<int>(m_targets.size()); ++i) {
        QPointF tp = polarToPx(m_targets[i].range_m, m_targets[i].angle_deg);
        double dist = std::hypot(px.x() - tp.x(), px.y() - tp.y());
        if (dist <= HIT_RADIUS)
            return i;
    }
    return -1;
}

// ── 그리기 ────────────────────────────────────────────

void SceneWidget::drawGrid(QPainter &p)
{
    QPointF origin = radarOrigin();
    double  sc     = scale();

    // 동심원 (거리 링)
    const double rings[] = {50.0, 100.0, 150.0, 200.0};
    QPen ringPen(QColor("#1a5a1a"), 1, Qt::SolidLine);
    p.setPen(ringPen);
    for (double d : rings) {
        double r = d * sc;
        p.drawEllipse(QPointF(origin.x(), origin.y()), r, r);
    }

    // 거리 레이블
    QFont labelFont("Courier New", 8);
    p.setFont(labelFont);
    p.setPen(QColor("#00aa30"));
    for (double d : rings) {
        double r = d * sc;
        p.drawText(QPointF(origin.x() + 4, origin.y() - r + 12),
                   QString("%1m").arg(static_cast<int>(d)));
    }

    // 각도선: ±30°, ±60°
    QPen anglePen(QColor("#144414"), 1, Qt::DashLine);
    p.setPen(anglePen);
    const double angles[] = {-60.0, -30.0, 30.0, 60.0};
    for (double a : angles) {
        QPointF far = polarToPx(MAX_RANGE, a);
        p.drawLine(origin, far);
    }
}

void SceneWidget::drawTargets(QPainter &p)
{
    QFont font("Courier New", 8);
    p.setFont(font);

    for (int i = 0; i < static_cast<int>(m_targets.size()); ++i) {
        const Target &t = m_targets[i];
        QPointF tp = polarToPx(t.range_m, t.angle_deg);
        bool selected = (i == m_selected);

        QColor color = selected ? QColor("#66ffaa") : QColor("#00ff41");
        p.setPen(QPen(color, selected ? 2 : 1));
        p.setBrush(Qt::NoBrush);

        // 원
        const double R = 8.0;
        p.drawEllipse(tp, R, R);

        // 십자
        p.drawLine(QPointF(tp.x() - R*1.6, tp.y()), QPointF(tp.x() + R*1.6, tp.y()));
        p.drawLine(QPointF(tp.x(), tp.y() - R*1.6), QPointF(tp.x(), tp.y() + R*1.6));

        // 속도 방향 화살표
        if (std::abs(t.velocity_mps) > 1e-2) {
            double rad   = t.angle_deg * M_PI / 180.0;
            /* + velocity → 레이더 방향(안쪽), - velocity → 바깥쪽 */
            double sign  = (t.velocity_mps > 0) ? -1.0 : 1.0;
            double arrowLen = std::min(std::abs(t.velocity_mps) * 1.5 + 10.0, 30.0);
            QPointF arrowTip(tp.x() + sign * arrowLen * std::sin(rad),
                             tp.y() - sign * arrowLen * std::cos(rad));
            QPen arrowPen(t.velocity_mps > 0 ? QColor("#ff4444") : QColor("#4488ff"), 2);
            p.setPen(arrowPen);
            p.drawLine(tp, arrowTip);
            /* 화살촉 */
            double headLen = 6.0;
            double headAng = 0.4;
            double ax = arrowTip.x() - tp.x(), ay = arrowTip.y() - tp.y();
            double alen = std::hypot(ax, ay);
            if (alen > 1e-3) {
                ax /= alen; ay /= alen;
                p.drawLine(arrowTip,
                    QPointF(arrowTip.x() - headLen*(ax*std::cos(headAng) - ay*std::sin(headAng)),
                            arrowTip.y() - headLen*(ay*std::cos(headAng) + ax*std::sin(headAng))));
                p.drawLine(arrowTip,
                    QPointF(arrowTip.x() - headLen*(ax*std::cos(headAng) + ay*std::sin(headAng)),
                            arrowTip.y() - headLen*(ay*std::cos(headAng) - ax*std::sin(headAng))));
            }
        }

        // 텍스트 레이블
        QString velStr = t.velocity_mps >= 0
                         ? QString("+%1").arg(t.velocity_mps, 0, 'f', 1)
                         : QString("%1").arg(t.velocity_mps, 0, 'f', 1);
        QString info = QString("R=%1m V=%2").arg(t.range_m, 0, 'f', 0).arg(velStr);
        p.setPen(color);
        p.drawText(QPointF(tp.x() + R + 4, tp.y() - 4), info);
    }
}

void SceneWidget::drawRadar(QPainter &p)
{
    QPointF origin = radarOrigin();

    // 레이더 아이콘: 위쪽 삼각형
    QPainterPath triangle;
    triangle.moveTo(origin.x(),        origin.y() - 14);
    triangle.lineTo(origin.x() - 10,  origin.y() +  4);
    triangle.lineTo(origin.x() + 10,  origin.y() +  4);
    triangle.closeSubpath();

    p.setPen(QPen(QColor("#00ff41"), 1));
    p.setBrush(QBrush(QColor("#00ff41")));
    p.drawPath(triangle);
}

void SceneWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 배경
    p.fillRect(rect(), QColor("#060d06"));

    drawGrid(p);
    drawTargets(p);
    drawRadar(p);
}

// ── 마우스 이벤트 ─────────────────────────────────────

void SceneWidget::mousePressEvent(QMouseEvent *e)
{
    QPointF pos = e->position();

    if (e->button() == Qt::LeftButton) {
        int idx = hitTest(pos);
        if (idx >= 0) {
            m_drag_idx = idx;
            m_selected = idx;
            emit targetSelected(m_selected);
        } else {
            double r, ang;
            if (pxToPolar(pos, r, ang) && r > 0.5) {
                Target t;
                t.range_m      = r;
                t.velocity_mps = 0.0;
                t.angle_deg    = ang;
                t.rcs_m2       = 1.0;
                m_targets.push_back(t);
                m_selected = static_cast<int>(m_targets.size()) - 1;
                emit targetsChanged(m_targets);
                emit targetSelected(m_selected);
            }
        }
    } else if (e->button() == Qt::RightButton) {
        int idx = hitTest(pos);
        if (idx >= 0) {
            m_targets.erase(m_targets.begin() + idx);
            if (m_selected >= static_cast<int>(m_targets.size()))
                m_selected = static_cast<int>(m_targets.size()) - 1;
            emit targetsChanged(m_targets);
            emit targetSelected(m_selected);
        }
    }
    update();
}

void SceneWidget::mouseMoveEvent(QMouseEvent *e)
{
    QPointF pos = e->position();

    if (m_drag_idx >= 0 && (e->buttons() & Qt::LeftButton)) {
        double r, ang;
        if (pxToPolar(pos, r, ang) && r > 0.5) {
            m_targets[m_drag_idx].range_m    = r;
            m_targets[m_drag_idx].angle_deg  = ang;
            update();
        }
    } else {
        int hover = hitTest(pos);
        if (hover != m_hover_idx) {
            m_hover_idx = hover;
            update();
        }
    }
}

void SceneWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_drag_idx >= 0) {
        emit targetsChanged(m_targets);
        m_drag_idx = -1;
    }
}

void SceneWidget::addDefaultTargets()
{
    m_targets.clear();

    Target t1;
    t1.range_m      = 50.0;
    t1.velocity_mps =  10.0;
    t1.angle_deg    =   0.0;
    t1.rcs_m2       =  10.0;
    m_targets.push_back(t1);

    Target t2;
    t2.range_m      = 100.0;
    t2.velocity_mps =  -5.0;
    t2.angle_deg    =  20.0;
    t2.rcs_m2       =   1.0;
    m_targets.push_back(t2);

    update();
    emit targetsChanged(m_targets);
}

void SceneWidget::onAnimTick()
{
    /* 드래그 중에는 사용자가 직접 제어하므로 위치 갱신 안 함 */
    if (m_drag_idx >= 0) { m_elapsed.restart(); return; }

    double dt = m_elapsed.restart() / 1000.0;  /* 초 단위 경과 시간 */
    dt = std::min(dt, 0.2);                     /* 최대 0.2초 (포커스 잃었을 때 튀는 것 방지) */

    bool anyMoved = false;
    for (auto &t : m_targets) {
        if (std::abs(t.velocity_mps) < 1e-3) continue;
        /* + velocity = 접근(거리 감소), - velocity = 이탈(거리 증가) */
        t.range_m -= t.velocity_mps * dt;
        anyMoved = true;
    }

    if (!anyMoved) return;

    /* 범위 벗어난 표적 제거 (0m 이하 또는 200m 초과) */
    bool removed = false;
    for (int i = (int)m_targets.size() - 1; i >= 0; --i) {
        if (m_targets[i].range_m <= 0.0 || m_targets[i].range_m >= MAX_RANGE) {
            m_targets.erase(m_targets.begin() + i);
            if (m_selected == i)          m_selected = -1;
            else if (m_selected > i)      m_selected--;
            if (m_drag_idx == i)          m_drag_idx = -1;
            else if (m_drag_idx > i)      m_drag_idx--;
            removed = true;
        }
    }

    update();
    emit targetsChanged(m_targets);
    if (removed) emit targetSelected(m_selected);
}

void SceneWidget::clearTargets()
{
    m_targets.clear();
    m_selected = -1;
    m_drag_idx = -1;
    emit targetsChanged(m_targets);
    emit targetSelected(-1);
    update();
}

void SceneWidget::setTargetVelocity(int idx, double v)
{
    if (idx < 0 || idx >= static_cast<int>(m_targets.size())) return;
    m_targets[idx].velocity_mps = v;
    emit targetsChanged(m_targets);
    update();
}

void SceneWidget::setTargetRCS(int idx, double rcs)
{
    if (idx < 0 || idx >= static_cast<int>(m_targets.size())) return;
    m_targets[idx].rcs_m2 = rcs;
    emit targetsChanged(m_targets);
}

// ═══════════════════════════════════════════════════════
//  TargetInfoPanel
// ═══════════════════════════════════════════════════════

static const QString kSpinStyle =
    "QDoubleSpinBox { background:#0d2a0d; color:#00ff41; border:1px solid #1a5a1a;"
    " font-family:'Courier New'; font-size:9pt; padding:1px 4px; }"
    "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width:14px; }";

static const QString kBtnStyle =
    "QPushButton { background:#0d2a0d; color:#00ff41; border:1px solid #1a5a1a;"
    " font-family:'Courier New'; font-size:9pt; padding:2px 8px; }"
    "QPushButton:hover { background:#1a5a1a; }";

TargetInfoPanel::TargetInfoPanel(SceneWidget *scene, QWidget *parent)
    : QWidget(parent), m_scene(scene)
{
    setFixedHeight(76);
    setStyleSheet("background:#060d06;");

    auto *vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(8, 4, 8, 4);
    vlay->setSpacing(4);

    // ── 1행: 힌트 + Clear All 버튼
    auto *row1 = new QHBoxLayout();
    auto *hint = new QLabel(
        "<span style='color:#007a20; font-family:Courier New; font-size:8pt;'>"
        "LClick: add &nbsp;|&nbsp; Drag: move &nbsp;|&nbsp; RClick: delete"
        "</span>");
    hint->setTextFormat(Qt::RichText);

    auto *btnClear = new QPushButton("✕ Clear All");
    btnClear->setStyleSheet(kBtnStyle);
    btnClear->setFixedHeight(22);

    row1->addWidget(hint);
    row1->addStretch();
    row1->addWidget(btnClear);

    // ── 2행: 선택 타겟 속성
    auto *row2 = new QHBoxLayout();
    row2->setSpacing(8);

    m_selLabel = new QLabel("선택: 없음");
    m_selLabel->setStyleSheet("color:#007a20; font-family:'Courier New'; font-size:8pt;");
    m_selLabel->setMinimumWidth(70);

    auto *velLbl = new QLabel("Velocity");
    velLbl->setStyleSheet("color:#00aa30; font-family:'Courier New'; font-size:8pt;");

    m_velSpin = new QDoubleSpinBox();
    m_velSpin->setRange(-100.0, 100.0);
    m_velSpin->setSingleStep(1.0);
    m_velSpin->setSuffix(" m/s");
    m_velSpin->setDecimals(1);
    m_velSpin->setFixedWidth(100);
    m_velSpin->setStyleSheet(kSpinStyle);
    m_velSpin->setEnabled(false);

    auto *rcsLbl = new QLabel("RCS");
    rcsLbl->setStyleSheet("color:#00aa30; font-family:'Courier New'; font-size:8pt;");

    m_rcsSpin = new QDoubleSpinBox();
    m_rcsSpin->setRange(0.01, 1000.0);
    m_rcsSpin->setSingleStep(0.5);
    m_rcsSpin->setSuffix(" m²");
    m_rcsSpin->setDecimals(2);
    m_rcsSpin->setFixedWidth(100);
    m_rcsSpin->setStyleSheet(kSpinStyle);
    m_rcsSpin->setEnabled(false);

    row2->addWidget(m_selLabel);
    row2->addWidget(velLbl);
    row2->addWidget(m_velSpin);
    row2->addWidget(rcsLbl);
    row2->addWidget(m_rcsSpin);
    row2->addStretch();

    vlay->addLayout(row1);
    vlay->addLayout(row2);

    // 시그널 연결
    connect(scene, &SceneWidget::targetSelected, this, &TargetInfoPanel::onTargetSelected);
    connect(btnClear, &QPushButton::clicked, scene, &SceneWidget::clearTargets);
    connect(m_velSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TargetInfoPanel::onVelocityChanged);
    connect(m_rcsSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TargetInfoPanel::onRCSChanged);
}

void TargetInfoPanel::onTargetSelected(int idx)
{
    m_idx = idx;
    if (idx < 0 || idx >= static_cast<int>(m_scene->targets().size())) {
        m_selLabel->setText("선택: 없음");
        m_velSpin->setEnabled(false);
        m_rcsSpin->setEnabled(false);
        return;
    }
    const Target &t = m_scene->targets()[idx];
    m_selLabel->setText(QString("Target #%1").arg(idx + 1));
    m_velSpin->blockSignals(true);
    m_rcsSpin->blockSignals(true);
    m_velSpin->setValue(t.velocity_mps);
    m_rcsSpin->setValue(t.rcs_m2);
    m_velSpin->blockSignals(false);
    m_rcsSpin->blockSignals(false);
    m_velSpin->setEnabled(true);
    m_rcsSpin->setEnabled(true);
}

void TargetInfoPanel::onVelocityChanged(double v)
{
    m_scene->setTargetVelocity(m_idx, v);
}

void TargetInfoPanel::onRCSChanged(double rcs)
{
    m_scene->setTargetRCS(m_idx, rcs);
}
