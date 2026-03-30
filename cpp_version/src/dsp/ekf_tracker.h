#pragma once
#ifndef EKF_TRACKER_H
#define EKF_TRACKER_H

#include "radar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EKF_MISS_MAX    3   /* 이 횟수 이상 미탐지 시 트랙 소멸 */
#define EKF_HIT_CONFIRM 3   /* 이 횟수 이상 탐지 시 확정 트랙 */
#define EKF_Q_ACCEL     0.1 /* 가속도 잡음 분산 q (m²/s⁴) */

typedef struct {
    TrackState tracks[RADAR_MAX_TRACKS];
    int        n_tracks;
    int        next_id;
} EKFTracker;

/* 트래커 초기화 */
void ekf_init(EKFTracker *tracker);

/*
 * 한 프레임 처리
 *   1. Predict 모든 트랙
 *   2. Data Association
 *   3. Update 매칭 트랙 / miss_count 증가 / 신규 트랙 초기화
 *   4. 소멸 트랙 제거
 *   5. out_objects 에 결과 기록
 */
void ekf_step(EKFTracker *tracker,
              const Cluster *clusters, int n_clusters,
              const RadarParams *p, const RadarParamsDerived *d,
              ObjectInfo *out_objects, int *out_n);

/* 단일 트랙 predict */
void ekf_predict(TrackState *tr, double dt, double q);

/* 단일 트랙 update (측정: z = [R, v_radial, theta_deg]) */
void ekf_update(TrackState *tr, const Cluster *cl, const RadarParamsDerived *d);

/* 새 트랙 초기화 */
void ekf_init_track(TrackState *tr, int id, const Cluster *cl);

#ifdef __cplusplus
}
#endif

#endif /* EKF_TRACKER_H */
