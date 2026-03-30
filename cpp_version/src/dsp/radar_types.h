#pragma once
#ifndef RADAR_TYPES_H
#define RADAR_TYPES_H

#include <stdint.h>
#include <fftw3.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================
 *  기본 상수
 * ========================================================= */
#define RADAR_C       3e8          /* 광속 (m/s) */
#define RADAR_PI      3.14159265358979323846
#define RADAR_MAX_TARGETS   16
#define RADAR_MAX_TRACKS    32

/* =========================================================
 *  Target — 시뮬레이션 입력 타겟
 * ========================================================= */
typedef struct {
    double range_m;        /* 거리 (m) */
    double velocity_mps;   /* 시선 방향 속도 (m/s, 양수=접근) */
    double angle_deg;      /* 정면 기준 각도 (도) */
    double rcs_m2;         /* 레이더 반사 단면적 (m²) */
} Target;

/* =========================================================
 *  RadarParams — 레이더 파형/처리 파라미터
 * ========================================================= */
typedef struct {
    /* 파형 */
    double fc_hz;          /* 반송파 주파수 (Hz)       default: 77e9  */
    double bandwidth_hz;   /* 처프 대역폭 (Hz)         default: 150e6 */
    double chirp_dur_s;    /* 처프 지속 시간 (s)        default: 40e-6 */
    double prf_hz;         /* 펄스 반복 주파수 (Hz)     default: 1000  */
    double fs_hz;          /* ADC 샘플링 레이트 (Hz)   default: 15e6  */
    int    n_chirp;        /* 처프 수                  default: 64    */
    /* MIMO */
    int    n_tx;           /* TX 안테나 수             default: 2     */
    int    n_rx;           /* RX 안테나 수             default: 4     */
    /* 잡음 */
    double noise_std;      /* 복소 잡음 표준편차        default: 2e-5  */
    /* CFAR */
    double cfar_pfa;       /* 오경보 확률              default: 1e-4  */
    int    cfar_guard;     /* guard cell (한쪽)        default: 2     */
    int    cfar_train;     /* training cell (한쪽)     default: 8     */
    /* FFT 포인트 */
    int    n_range_fft;    /* Range FFT 포인트         default: 512   */
    int    n_doppler_fft;  /* Doppler FFT 포인트       default: 128   */
    int    n_angle_fft;    /* Angle FFT 포인트         default: 256   */
} RadarParams;

/* =========================================================
 *  RadarParamsDerived — 파생 값 (params_derive()로 계산)
 * ========================================================= */
typedef struct {
    double lam;               /* 파장 (m)                     */
    double mu;                /* 처프 레이트 (Hz/s)            */
    double t_pri;             /* 처프 반복 주기 (s)            */
    int    n_sample;          /* 처프 당 ADC 샘플 수           */
    int    n_virtual;         /* 가상 배열 소자 수 = n_tx*n_rx */
    double d_elem;            /* 가상 배열 소자 간격 (m)       */
    double range_resolution;  /* 거리 분해능 (m)               */
    double range_max_m;       /* 최대 탐지 거리 (m)            */
    double velocity_max_mps;  /* 최대 탐지 속도 (m/s)          */
    double velocity_resolution; /* 속도 분해능 (m/s)           */
} RadarParamsDerived;

/* =========================================================
 *  DetectionPoint — CFAR 탐지 후 AoA 추정 결과
 * ========================================================= */
typedef struct {
    double range_m;
    double velocity_mps;
    double angle_deg;
    double power_db;
    int    range_bin;
    int    doppler_bin;
} DetectionPoint;

/* =========================================================
 *  Cluster — DBSCAN 군집화 결과
 * ========================================================= */
typedef struct {
    double range_m;        /* 군집 무게중심 거리 */
    double velocity_mps;   /* 군집 무게중심 속도 */
    double angle_deg;      /* 군집 무게중심 각도 */
    double power_db;       /* 군집 평균 파워 */
    int    n_points;       /* 군집 내 포인트 수 */
    int    cluster_id;
} Cluster;

/* =========================================================
 *  TrackState — EKF 단일 트랙 상태
 * ========================================================= */
typedef struct {
    int    id;
    double x[4];         /* [x_pos, y_pos, vx, vy] (m, m/s) */
    double P[4][4];      /* 오차 공분산 행렬 */
    int    miss_count;   /* 연속 미탐지 프레임 수 */
    int    hit_count;    /* 누적 탐지 수 (확정 판단용) */
    int    active;       /* 1=활성, 0=소멸 */
} TrackState;

/* =========================================================
 *  ObjectInfo — EKF 출력 (상위 레이어 전달용)
 * ========================================================= */
typedef struct {
    int    id;
    double range_m;
    double velocity_mps;
    double angle_deg;
    double cov_xx;       /* 공분산 타원용 (x-x) */
    double cov_yy;       /* 공분산 타원용 (y-y) */
    double cov_xy;       /* 공분산 타원용 (x-y) */
    int    confirmed;    /* hit_count >= 3 이면 1 */
} ObjectInfo;

/* =========================================================
 *  파라미터 초기화 / 파생값 계산
 * ========================================================= */
RadarParams       params_default(void);
RadarParamsDerived params_derive(const RadarParams *p);

/* 축 배열 생성 (호출자가 free 해야 함) */
double *range_axis_alloc(const RadarParams *p, const RadarParamsDerived *d);
double *velocity_axis_alloc(const RadarParams *p, const RadarParamsDerived *d);
double *angle_axis_alloc(const RadarParams *p, const RadarParamsDerived *d);

#ifdef __cplusplus
}
#endif

#endif /* RADAR_TYPES_H */
