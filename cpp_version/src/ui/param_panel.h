#pragma once
#ifndef PARAM_PANEL_H
#define PARAM_PANEL_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QTimer>
#include "../dsp/radar_types.h"

// ─────────────────────────────────────────────
//  ParamRow — 슬라이더 한 행: [이름] [────●────] [값 단위]
// ─────────────────────────────────────────────
class ParamRow : public QWidget
{
    Q_OBJECT
public:
    ParamRow(const QString &label, const QString &unit,
             double lo, double hi, int steps, double init,
             bool log_scale = false, QWidget *parent = nullptr);

    double value() const;
    void   setValue(double v);

signals:
    void valueChanged(double v);

private slots:
    void onSliderMoved(int tick);

private:
    double tickToVal(int tick) const;
    int    valToTick(double v) const;

    QSlider *m_slider;
    QLabel  *m_valLabel;
    double   m_lo, m_hi;
    int      m_steps;
    bool     m_log;
    QString  m_fmt;
    QString  m_unit;
};

// ─────────────────────────────────────────────
//  ParamPanel — 레이더 파라미터 패널 (좌측 고정)
// ─────────────────────────────────────────────
class ParamPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ParamPanel(QWidget *parent = nullptr);
    RadarParams currentParams() const;

signals:
    void paramsChanged(RadarParams p);

private:
    void buildUI();
    void emitParams();

    // 파형 파라미터
    ParamRow *m_fc;
    ParamRow *m_bw;
    ParamRow *m_tc;
    ParamRow *m_prf;
    ParamRow *m_fs;
    ParamRow *m_nchirp;

    // MIMO 파라미터
    ParamRow *m_ntx;
    ParamRow *m_nrx;

    // 잡음 파라미터
    ParamRow *m_noise;

    // CFAR 파라미터
    ParamRow *m_cfar_pfa;
    ParamRow *m_cfar_guard;
    ParamRow *m_cfar_train;

    // 디바운스 타이머 (150ms)
    QTimer *m_debounceTimer;
};

#endif // PARAM_PANEL_H
