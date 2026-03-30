#pragma once
#ifndef SCENE_WIDGET_H
#define SCENE_WIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <vector>
#include "../dsp/radar_types.h"

// ─────────────────────────────────────────────
//  SceneWidget — Top-down 레이더 씬
// ─────────────────────────────────────────────
class SceneWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SceneWidget(QWidget *parent = nullptr);
    void addDefaultTargets();
    const std::vector<Target>& targets() const { return m_targets; }
    int selectedIndex() const { return m_selected; }

public slots:
    void clearTargets();
    void setTargetVelocity(int idx, double v);
    void setTargetRCS(int idx, double rcs);

private slots:
    void onAnimTick();

signals:
    void targetsChanged(std::vector<Target> targets);
    void targetSelected(int idx);   // -1 = 선택 없음

protected:
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    static constexpr double MAX_RANGE = 200.0;

    std::vector<Target> m_targets;
    int m_selected  = -1;
    int m_drag_idx  = -1;
    int m_hover_idx = -1;

    QTimer        *m_animTimer;
    QElapsedTimer  m_elapsed;

    QPointF radarOrigin() const;
    double  scale() const;
    QPointF polarToPx(double r, double angle_deg) const;
    bool    pxToPolar(QPointF px, double &r, double &angle_deg) const;
    int     hitTest(QPointF px) const;
    void    drawGrid(QPainter &p);
    void    drawTargets(QPainter &p);
    void    drawRadar(QPainter &p);
};

// ─────────────────────────────────────────────
//  TargetInfoPanel — 타겟 속성 편집 패널 (씬 하단)
// ─────────────────────────────────────────────
class QLabel;
class QDoubleSpinBox;
class QPushButton;

class TargetInfoPanel : public QWidget
{
    Q_OBJECT
public:
    explicit TargetInfoPanel(SceneWidget *scene, QWidget *parent = nullptr);

private slots:
    void onTargetSelected(int idx);
    void onVelocityChanged(double v);
    void onRCSChanged(double rcs);

private:
    SceneWidget     *m_scene;
    QLabel          *m_selLabel;
    QDoubleSpinBox  *m_velSpin;
    QDoubleSpinBox  *m_rcsSpin;
    int              m_idx = -1;
};

#endif // SCENE_WIDGET_H
