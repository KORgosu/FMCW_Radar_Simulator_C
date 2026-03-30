#include "clustering.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define DBSCAN_UNVISITED  -2
#define DBSCAN_NOISE      -1

static double normalized_dist(const DetectionPoint *a, const DetectionPoint *b,
                               double range_max, double vel_max)
{
    double dr = (a->range_m      - b->range_m)      / (range_max  > 1e-9 ? range_max  : 1.0);
    double dv = (a->velocity_mps - b->velocity_mps) / (2.0 * (vel_max > 1e-9 ? vel_max : 1.0));
    double dt = (a->angle_deg    - b->angle_deg)     / 180.0;
    return sqrt(dr*dr + dv*dv + dt*dt);
}

void dbscan_cluster(const DetectionPoint *points, int n_points,
                    double epsilon, int min_pts,
                    double range_max, double vel_max,
                    Cluster *out_clusters, int *out_n)
{
    if (n_points <= 0) { *out_n = 0; return; }

    int *labels = (int *)malloc(n_points * sizeof(int));
    for (int i = 0; i < n_points; i++) labels[i] = DBSCAN_UNVISITED;

    /* 이웃 인덱스 임시 버퍼 */
    int *neighbors = (int *)malloc(n_points * sizeof(int));
    /* queue: 같은 포인트가 중복 추가되는 것을 막기 위해 in_queue 플래그 사용 */
    int *queue    = (int *)malloc(n_points * sizeof(int));
    int *in_queue = (int *)calloc(n_points, sizeof(int));

    int cluster_id = 0;

    for (int i = 0; i < n_points; i++) {
        if (labels[i] != DBSCAN_UNVISITED) continue;

        /* 이웃 탐색 */
        int nn = 0;
        for (int j = 0; j < n_points; j++)
            if (normalized_dist(&points[i], &points[j], range_max, vel_max) <= epsilon)
                neighbors[nn++] = j;

        if (nn < min_pts) {
            labels[i] = DBSCAN_NOISE;
            continue;
        }

        /* 새 군집 시작 */
        labels[i] = cluster_id;
        int q_head = 0, q_tail = 0;
        for (int k = 0; k < nn; k++) {
            int nb = neighbors[k];
            if (nb != i && !in_queue[nb]) {
                queue[q_tail++] = nb;
                in_queue[nb] = 1;
            }
        }

        while (q_head < q_tail) {
            int cur = queue[q_head++];
            if (labels[cur] == DBSCAN_NOISE) labels[cur] = cluster_id;
            if (labels[cur] != DBSCAN_UNVISITED) continue;
            labels[cur] = cluster_id;

            int nn2 = 0;
            for (int j = 0; j < n_points; j++)
                if (normalized_dist(&points[cur], &points[j], range_max, vel_max) <= epsilon)
                    neighbors[nn2++] = j;

            if (nn2 >= min_pts)
                for (int k = 0; k < nn2; k++) {
                    int nb = neighbors[k];
                    if (labels[nb] == DBSCAN_UNVISITED && !in_queue[nb]) {
                        queue[q_tail++] = nb;
                        in_queue[nb] = 1;
                    }
                }
        }
        cluster_id++;
    }

    /* 군집별 무게중심 계산 */
    int max_cid = cluster_id;
    if (max_cid <= 0) { *out_n = 0; free(labels); free(neighbors); free(queue); return; }

    double *sum_r = (double *)calloc(max_cid, sizeof(double));
    double *sum_v = (double *)calloc(max_cid, sizeof(double));
    double *sum_a = (double *)calloc(max_cid, sizeof(double));
    double *sum_p = (double *)calloc(max_cid, sizeof(double));
    int    *cnt   = (int    *)calloc(max_cid, sizeof(int));

    for (int i = 0; i < n_points; i++) {
        int cid = labels[i];
        if (cid < 0) continue;
        sum_r[cid] += points[i].range_m;
        sum_v[cid] += points[i].velocity_mps;
        sum_a[cid] += points[i].angle_deg;
        sum_p[cid] += points[i].power_db;
        cnt[cid]++;
    }

    int n_clusters = 0;
    for (int cid = 0; cid < max_cid; cid++) {
        if (cnt[cid] == 0) continue;
        out_clusters[n_clusters].range_m      = sum_r[cid] / cnt[cid];
        out_clusters[n_clusters].velocity_mps = sum_v[cid] / cnt[cid];
        out_clusters[n_clusters].angle_deg    = sum_a[cid] / cnt[cid];
        out_clusters[n_clusters].power_db     = sum_p[cid] / cnt[cid];
        out_clusters[n_clusters].n_points     = cnt[cid];
        out_clusters[n_clusters].cluster_id   = cid;
        n_clusters++;
    }
    *out_n = n_clusters;

    free(sum_r); free(sum_v); free(sum_a); free(sum_p); free(cnt);
    free(labels); free(neighbors); free(queue); free(in_queue);
}
