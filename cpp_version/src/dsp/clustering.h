#pragma once
#ifndef CLUSTERING_H
#define CLUSTERING_H

#include "radar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DBSCAN 군집화
 *   정규화 공간: R_n = R/range_max, v_n = v/(2*v_max), θ_n = θ/180
 *   epsilon  : 정규화 공간 기준 이웃 반경  (초기값 0.1)
 *   min_pts  : 최소 이웃 수               (초기값 2)
 *
 *   out_clusters : 군집 결과 배열 (호출자가 n_points 크기로 할당)
 *   out_n        : 실제 군집 수
 */
void dbscan_cluster(const DetectionPoint *points, int n_points,
                    double epsilon, int min_pts,
                    double range_max, double vel_max,
                    Cluster *out_clusters, int *out_n);

#ifdef __cplusplus
}
#endif

#endif /* CLUSTERING_H */
