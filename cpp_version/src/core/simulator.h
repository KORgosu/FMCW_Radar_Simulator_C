#pragma once
#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QObject>
#include <vector>
#include "sim_result.h"
#include "../dsp/radar_types.h"
#include "../dsp/ekf_tracker.h"
#include <fftw3.h>

class FMCWSimulator : public QObject
{
    Q_OBJECT
public:
    explicit FMCWSimulator(QObject *parent = nullptr);
    ~FMCWSimulator();

    /* 단일 프레임 계산 (EKF 초기화 후 1회) */
    SimResult compute(const std::vector<Target> &targets,
                      const RadarParams &params);

    /* 멀티프레임: EKF 상태 유지하며 1 프레임 진행 */
    SimResult step(const std::vector<Target> &targets,
                   const RadarParams &params);

    /* EKF 트래커 리셋 */
    void resetTracker();

signals:
    void simResultReady(SimResult result);

private:
    EKFTracker  m_tracker;
    RadarParams m_last_params;
    bool        m_plan_valid = false;

    /* FFTW 플랜 캐시 */
    int          m_plan_n_sample   = 0;
    int          m_plan_n_range    = 0;
    int          m_plan_n_doppler  = 0;

    SimResult runPipeline(const std::vector<Target> &targets,
                          const RadarParams &params,
                          bool reset_ekf);
};

#endif /* SIMULATOR_H */
