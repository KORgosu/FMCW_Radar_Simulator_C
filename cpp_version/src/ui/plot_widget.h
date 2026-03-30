#pragma once
#ifndef PLOT_WIDGET_H
#define PLOT_WIDGET_H

#include <QWidget>
#include <qcustomplot.h>
#include "../core/sim_result.h"
#include "../dsp/radar_types.h"

// QCustomPlot 기반 공통 베이스 위젯
// 다크 레이더 테마 적용
class RadarPlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RadarPlotWidget(QWidget *parent = nullptr);

protected:
    // 플롯에 다크 테마 적용
    void applyDarkTheme(QCustomPlot *plot);
    // 축 스타일 적용
    void styleAxis(QCPAxis *axis, const QString &label = QString());
    // dB 컬러맵 색상 범위 설정
    void setColormapRange(QCPColorMap *cm, double vmin, double vmax);
};

#endif // PLOT_WIDGET_H
