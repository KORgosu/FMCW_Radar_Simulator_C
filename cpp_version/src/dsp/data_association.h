#pragma once
#ifndef DATA_ASSOCIATION_H
#define DATA_ASSOCIATION_H

#include "radar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DA_MAX_SIZE  32    /* 행렬 최대 크기 */
#define DA_GATE_CHI2 9.21  /* χ²(3 DoF, 99%) 매칭 거부 임계값 */

/*
 * Data Association — Hungarian Algorithm
 *
 * tracks      : 현재 활성 TrackState 배열
 * n_tracks    : 트랙 수
 * clusters    : 현재 프레임 Cluster 배열
 * n_clusters  : 군집 수
 *
 * out_match_track[i]   : 군집 i에 매칭된 트랙 인덱스 (-1 = 미매칭, 신규 트랙)
 * out_unmatched_tracks : 매칭되지 않은 트랙 인덱스 목록
 * out_n_unmatched      : 미매칭 트랙 수
 */
void data_association(const TrackState *tracks, int n_tracks,
                      const Cluster *clusters, int n_clusters,
                      const RadarParamsDerived *d,
                      int *out_match_track,
                      int *out_unmatched_tracks, int *out_n_unmatched);

/* 마할라노비스 거리² 계산 (3x3 혁신 공분산 S의 역행렬 기반) */
double mahal_dist2(const TrackState *track, const Cluster *cluster,
                   const RadarParamsDerived *d);

#ifdef __cplusplus
}
#endif

#endif /* DATA_ASSOCIATION_H */
