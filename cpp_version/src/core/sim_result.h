#pragma once
#ifndef SIM_RESULT_H
#define SIM_RESULT_H

#include <vector>
#include "../dsp/radar_types.h"

struct SimResult {
    /* ── 파형 ── */
    std::vector<double>  t_fast;          /* fast-time 축 (s) */
    std::vector<double>  tx_chirp_re;     /* TX chirp 실수부 */
    std::vector<double>  tx_chirp_im;     /* TX chirp 허수부 */
    std::vector<double>  tx_freq;         /* 순간 주파수 (Hz) */
    std::vector<double>  beat_one_re;     /* 첫 chirp beat 실수부 */
    std::vector<double>  beat_one_im;     /* 첫 chirp beat 허수부 */

    /* ── Beat Matrix [n_chirp × n_sample] ── */
    std::vector<double>  beat_mat_re;
    std::vector<double>  beat_mat_im;
    int beat_rows = 0, beat_cols = 0;

    /* ── Range ── */
    std::vector<double>  range_axis;      /* 거리 축 (m) */
    std::vector<double>  range_profile_db;

    /* ── Range-Doppler Map [n_doppler × n_range/2] ── */
    std::vector<double>  velocity_axis;
    std::vector<double>  rdm_db;
    std::vector<double>  rdm_pow;
    int rdm_rows = 0, rdm_cols = 0;

    /* ── CFAR ── */
    std::vector<double>  cfar_threshold;  /* 선형 임계값 */
    std::vector<int>     cfar_detections; /* bool 맵 */

    /* ── AoA / Point Cloud ── */
    std::vector<double>  angle_axis;
    std::vector<DetectionPoint> detection_points;

    /* ── Clustering ── */
    std::vector<Cluster> clusters;

    /* ── EKF ── */
    std::vector<ObjectInfo> objects;
};

#endif /* SIM_RESULT_H */
