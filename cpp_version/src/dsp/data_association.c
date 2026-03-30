#include "data_association.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

/* ── 마할라노비스 거리² ─────────────────────────────────────── */
double mahal_dist2(const TrackState *tr, const Cluster *cl,
                   const RadarParamsDerived *d)
{
    double x = tr->x[0], y = tr->x[1];
    double R = sqrt(x*x + y*y);
    if (R < 1e-3) R = 1e-3;

    /* 예측 측정값 h(x) */
    double h0 = R;
    double h1 = (x * tr->x[2] + y * tr->x[3]) / R;
    double h2 = atan2(y, x) * 180.0 / RADAR_PI;

    /* 잔차 */
    double dz0 = cl->range_m      - h0;
    double dz1 = cl->velocity_mps - h1;
    double dz2 = cl->angle_deg    - h2;

    /* 측정 잡음 근사 (간소화: 대각 S^-1) */
    double sr = d->range_resolution / 2.0;
    double sv = d->velocity_resolution / 2.0;
    double sa = 3.0;   /* 각도 불확실도 ~3도 */
    double inv_sr2 = 1.0 / (sr*sr + 1e-9);
    double inv_sv2 = 1.0 / (sv*sv + 1e-9);
    double inv_sa2 = 1.0 / (sa*sa);

    return dz0*dz0*inv_sr2 + dz1*dz1*inv_sv2 + dz2*dz2*inv_sa2;
}

/* ── Hungarian Algorithm (최소 비용 매칭) ──────────────────── */
/* n×m 비용 행렬 → 행(군집) 별 최적 열(트랙) 할당 (−1=미할당) */
static void hungarian(const double *cost, int n, int m, int *assign)
{
    /* n=군집, m=트랙 */
    int sz = n > m ? n : m;   /* 정방 행렬로 확장 */
    double *C = (double *)malloc(sz * sz * sizeof(double));
    for (int i = 0; i < sz * sz; i++) C[i] = 1e18;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            C[i * sz + j] = cost[i * m + j];

    int *u     = (int *)calloc(sz + 1, sizeof(int));
    int *v     = (int *)calloc(sz + 1, sizeof(int));
    int *p     = (int *)calloc(sz + 1, sizeof(int));
    int *way   = (int *)calloc(sz + 1, sizeof(int));

    for (int i = 1; i <= sz; i++) {
        p[0] = i;
        int j0 = 0;
        double *minv = (double *)malloc((sz + 1) * sizeof(double));
        int    *used = (int    *)calloc(sz + 1, sizeof(int));
        if (!minv || !used) { free(minv); free(used); goto hungarian_done; }
        for (int j = 0; j <= sz; j++) minv[j] = 1e18;

        do {
            used[j0] = 1;
            int i0 = p[j0], j1 = -1;
            double delta = 1e18;
            for (int j = 1; j <= sz; j++) {
                if (!used[j]) {
                    double val = C[(i0-1)*sz + (j-1)] - u[i0] - v[j];
                    if (val < minv[j]) { minv[j] = val; way[j] = j0; }
                    if (minv[j] < delta) { delta = minv[j]; j1 = j; }
                }
            }
            for (int j = 0; j <= sz; j++) {
                if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                else          minv[j] -= delta;
            }
            if (j1 == -1) { free(minv); free(used); goto hungarian_done; }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            p[j0] = p[way[j0]];
            j0 = way[j0];
        } while (j0);

        free(minv); free(used);
    }
    hungarian_done:

    for (int i = 0; i < n; i++) assign[i] = -1;
    for (int j = 1; j <= sz && j <= m; j++)
        if (p[j] >= 1 && p[j] <= n)
            assign[p[j] - 1] = j - 1;

    free(u); free(v); free(p); free(way); free(C);
}

void data_association(const TrackState *tracks, int n_tracks,
                      const Cluster *clusters, int n_clusters,
                      const RadarParamsDerived *d,
                      int *out_match_track,
                      int *out_unmatched_tracks, int *out_n_unmatched)
{
    for (int i = 0; i < n_clusters; i++) out_match_track[i] = -1;
    *out_n_unmatched = 0;

    if (n_clusters == 0 || n_tracks == 0) {
        for (int j = 0; j < n_tracks; j++)
            out_unmatched_tracks[(*out_n_unmatched)++] = j;
        return;
    }

    /* 비용 행렬 [n_clusters × n_tracks] */
    double *cost = (double *)malloc(n_clusters * n_tracks * sizeof(double));
    for (int i = 0; i < n_clusters; i++)
        for (int j = 0; j < n_tracks; j++) {
            double d2 = mahal_dist2(&tracks[j], &clusters[i], d);
            cost[i * n_tracks + j] = (d2 <= DA_GATE_CHI2) ? d2 : 1e15;
        }

    int *assign = (int *)malloc(n_clusters * sizeof(int));
    if (!assign) { free(cost); return; }
    hungarian(cost, n_clusters, n_tracks, assign);

    /* 게이팅: 비용이 임계값 초과면 매칭 거부 */
    int *track_matched = (int *)calloc(n_tracks, sizeof(int));
    if (!track_matched) { free(cost); free(assign); return; }
    for (int i = 0; i < n_clusters; i++) {
        int j = assign[i];
        if (j >= 0 && j < n_tracks &&
            mahal_dist2(&tracks[j], &clusters[i], d) <= DA_GATE_CHI2) {
            out_match_track[i]  = j;
            track_matched[j]    = 1;
        }
    }
    for (int j = 0; j < n_tracks; j++)
        if (!track_matched[j])
            out_unmatched_tracks[(*out_n_unmatched)++] = j;

    free(cost); free(assign); free(track_matched);
}
