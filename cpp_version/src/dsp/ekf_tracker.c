#include "ekf_tracker.h"
#include "data_association.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* ── 4×4 행렬 연산 헬퍼 ─────────────────────────────────────── */
static void mat4_mul(const double A[4][4], const double B[4][4], double C[4][4])
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            C[i][j] = 0;
            for (int k = 0; k < 4; k++) C[i][j] += A[i][k] * B[k][j];
        }
}
static void mat4_add(const double A[4][4], const double B[4][4], double C[4][4])
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) C[i][j] = A[i][j] + B[i][j];
}
static void mat4_transpose(const double A[4][4], double At[4][4])
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) At[j][i] = A[i][j];
}

/* 3×3 역행렬 (측정 공분산 S) */
static int mat3_inv(const double M[3][3], double Minv[3][3])
{
    double det = M[0][0]*(M[1][1]*M[2][2]-M[1][2]*M[2][1])
               - M[0][1]*(M[1][0]*M[2][2]-M[1][2]*M[2][0])
               + M[0][2]*(M[1][0]*M[2][1]-M[1][1]*M[2][0]);
    if (fabs(det) < 1e-30) return 0;
    double inv_det = 1.0 / det;
    Minv[0][0] = (M[1][1]*M[2][2]-M[1][2]*M[2][1]) * inv_det;
    Minv[0][1] = (M[0][2]*M[2][1]-M[0][1]*M[2][2]) * inv_det;
    Minv[0][2] = (M[0][1]*M[1][2]-M[0][2]*M[1][1]) * inv_det;
    Minv[1][0] = (M[1][2]*M[2][0]-M[1][0]*M[2][2]) * inv_det;
    Minv[1][1] = (M[0][0]*M[2][2]-M[0][2]*M[2][0]) * inv_det;
    Minv[1][2] = (M[0][2]*M[1][0]-M[0][0]*M[1][2]) * inv_det;
    Minv[2][0] = (M[1][0]*M[2][1]-M[1][1]*M[2][0]) * inv_det;
    Minv[2][1] = (M[0][1]*M[2][0]-M[0][0]*M[2][1]) * inv_det;
    Minv[2][2] = (M[0][0]*M[1][1]-M[0][1]*M[1][0]) * inv_det;
    return 1;
}

/* ── Predict ────────────────────────────────────────────────── */
void ekf_predict(TrackState *tr, double dt, double q)
{
    /* F: 상수 속도 모델 */
    double F[4][4] = {
        {1, 0, dt, 0 },
        {0, 1, 0,  dt},
        {0, 0, 1,  0 },
        {0, 0, 0,  1 }
    };
    /* Q: 가속도 잡음 모델 */
    double dt2 = dt * dt, dt3 = dt2 * dt, dt4 = dt3 * dt;
    double Q[4][4] = {
        {dt4/4, 0,     dt3/2, 0    },
        {0,     dt4/4, 0,     dt3/2},
        {dt3/2, 0,     dt2,   0    },
        {0,     dt3/2, 0,     dt2  }
    };
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) Q[i][j] *= q;

    /* x = F * x */
    double xn[4] = {0};
    for (int i = 0; i < 4; i++)
        for (int k = 0; k < 4; k++) xn[i] += F[i][k] * tr->x[k];
    memcpy(tr->x, xn, sizeof(xn));

    /* P = F * P * F^T + Q */
    double Ft[4][4], FP[4][4], FPFt[4][4], Pnew[4][4];
    mat4_transpose(F, Ft);
    mat4_mul(F, tr->P, FP);
    mat4_mul(FP, Ft, FPFt);
    mat4_add(FPFt, Q, Pnew);
    memcpy(tr->P, Pnew, sizeof(Pnew));
}

/* ── Update ─────────────────────────────────────────────────── */
void ekf_update(TrackState *tr, const Cluster *cl, const RadarParamsDerived *d)
{
    double x = tr->x[0], y = tr->x[1], vx = tr->x[2], vy = tr->x[3];
    double R = sqrt(x*x + y*y);
    if (R < 1e-3) R = 1e-3;

    /* 예측 측정값 h(x) */
    double h[3];
    h[0] = R;
    h[1] = (x*vx + y*vy) / R;
    h[2] = atan2(y, x) * 180.0 / RADAR_PI;

    /* 측정값 z */
    double z[3] = { cl->range_m, cl->velocity_mps, cl->angle_deg };

    /* H Jacobian [3×4] */
    double H[3][4] = {
        { x/R,           y/R,          0, 0 },
        { (vx*R - x*(x*vx+y*vy)/R) / (R*R),
          (vy*R - y*(x*vx+y*vy)/R) / (R*R), x/R, y/R },
        { -y/(R*R) * (180.0/RADAR_PI),
           x/(R*R) * (180.0/RADAR_PI), 0, 0 }
    };

    /* 측정 잡음 R_meas = diag(σ²) */
    double sr = d->range_resolution / 2.0;
    double sv = d->velocity_resolution / 2.0;
    double sa = 3.0;
    double Rm[3][3] = {
        {sr*sr, 0,     0    },
        {0,     sv*sv, 0    },
        {0,     0,     sa*sa}
    };

    /* S = H * P * H^T + R_meas  [3×3] */
    double HP[3][4], Ht[4][3];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 4; j++) {
            HP[i][j] = 0;
            for (int k = 0; k < 4; k++) HP[i][j] += H[i][k] * tr->P[k][j];
        }
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 4; j++) Ht[j][i] = H[i][j];

    double S[3][3];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            S[i][j] = Rm[i][j];
            for (int k = 0; k < 4; k++) S[i][j] += HP[i][k] * Ht[k][j];
        }

    double Sinv[3][3];
    if (!mat3_inv(S, Sinv)) return;

    /* K = P * H^T * S^-1  [4×3] */
    double PHt[4][3], K[4][3];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++) {
            PHt[i][j] = 0;
            for (int k = 0; k < 4; k++) PHt[i][j] += tr->P[i][k] * Ht[k][j];
        }
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++) {
            K[i][j] = 0;
            for (int k = 0; k < 3; k++) K[i][j] += PHt[i][k] * Sinv[k][j];
        }

    /* 잔차 dz */
    double dz[3] = { z[0]-h[0], z[1]-h[1], z[2]-h[2] };

    /* x = x + K * dz */
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++) tr->x[i] += K[i][j] * dz[j];

    /* P = (I - K*H) * P */
    double KH[4][4] = {{0}};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 3; k++) KH[i][j] += K[i][k] * H[k][j];

    double Pnew[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            Pnew[i][j] = (i==j ? 1.0 : 0.0) * tr->P[i][j] - KH[i][j] * tr->P[i][j];
    /* 올바른 (I-KH)P 계산 */
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            Pnew[i][j] = 0;
            for (int k = 0; k < 4; k++) {
                double IKH_ij = (i==k ? 1.0 : 0.0) - KH[i][k];
                Pnew[i][j] += IKH_ij * tr->P[k][j];
            }
        }
    memcpy(tr->P, Pnew, sizeof(Pnew));
}

/* ── 트랙 초기화 ─────────────────────────────────────────────── */
void ekf_init_track(TrackState *tr, int id, const Cluster *cl)
{
    double theta_rad = cl->angle_deg * RADAR_PI / 180.0;
    tr->id = id;
    tr->x[0] = cl->range_m      * sin(theta_rad);   /* x_pos */
    tr->x[1] = cl->range_m      * cos(theta_rad);   /* y_pos */
    tr->x[2] = cl->velocity_mps * sin(theta_rad);   /* vx    */
    tr->x[3] = cl->velocity_mps * cos(theta_rad);   /* vy    */

    double sr = 5.0, sv = 2.0;
    memset(tr->P, 0, sizeof(tr->P));
    tr->P[0][0] = sr*sr; tr->P[1][1] = sr*sr;
    tr->P[2][2] = sv*sv; tr->P[3][3] = sv*sv;

    tr->miss_count = 0;
    tr->hit_count  = 1;
    tr->active     = 1;
}

/* ── Tracker 초기화 ─────────────────────────────────────────── */
void ekf_init(EKFTracker *tracker)
{
    memset(tracker, 0, sizeof(EKFTracker));
}

/* ── 한 프레임 처리 ──────────────────────────────────────────── */
void ekf_step(EKFTracker *tracker,
              const Cluster *clusters, int n_clusters,
              const RadarParams *p, const RadarParamsDerived *d,
              ObjectInfo *out_objects, int *out_n)
{
    double dt = (double)p->n_chirp * d->t_pri;  /* 프레임 주기 */

    /* 1. Predict */
    for (int j = 0; j < tracker->n_tracks; j++)
        if (tracker->tracks[j].active)
            ekf_predict(&tracker->tracks[j], dt, EKF_Q_ACCEL);

    /* 2. Data Association */
    int *match = (int *)malloc((n_clusters > 0 ? n_clusters : 1) * sizeof(int));
    for (int i = 0; i < n_clusters; i++) match[i] = -1;
    int unmatched[RADAR_MAX_TRACKS];
    int n_unmatched = 0;
    data_association(tracker->tracks, tracker->n_tracks,
                     clusters, n_clusters, d,
                     match, unmatched, &n_unmatched);

    /* 3a. Update 매칭 트랙 */
    for (int i = 0; i < n_clusters; i++) {
        int j = match[i];
        if (j < 0) continue;
        ekf_update(&tracker->tracks[j], &clusters[i], d);
        tracker->tracks[j].hit_count++;
        tracker->tracks[j].miss_count = 0;
    }

    /* 3b. 미매칭 트랙 miss_count 증가 */
    for (int k = 0; k < n_unmatched; k++) {
        int j = unmatched[k];
        tracker->tracks[j].miss_count++;
        if (tracker->tracks[j].miss_count >= EKF_MISS_MAX)
            tracker->tracks[j].active = 0;
    }

    /* 3c. 신규 트랙 초기화 */
    for (int i = 0; i < n_clusters; i++) {
        if (match[i] != -1) continue;
        if (tracker->n_tracks >= RADAR_MAX_TRACKS) break;
        ekf_init_track(&tracker->tracks[tracker->n_tracks],
                       tracker->next_id++, &clusters[i]);
        tracker->n_tracks++;
    }

    /* 4. 소멸 트랙 압축 */
    int new_n = 0;
    for (int j = 0; j < tracker->n_tracks; j++)
        if (tracker->tracks[j].active)
            tracker->tracks[new_n++] = tracker->tracks[j];
    tracker->n_tracks = new_n;

    /* 5. ObjectInfo 출력 */
    int cnt = 0;
    for (int j = 0; j < tracker->n_tracks; j++) {
        TrackState *tr = &tracker->tracks[j];
        double x = tr->x[0], y = tr->x[1];
        double vx = tr->x[2], vy = tr->x[3];
        double R = sqrt(x*x + y*y);
        if (R < 1e-3) R = 1e-3;
        out_objects[cnt].id           = tr->id;
        out_objects[cnt].range_m      = R;
        out_objects[cnt].velocity_mps = (x*vx + y*vy) / R;
        out_objects[cnt].angle_deg    = atan2(x, y) * 180.0 / RADAR_PI;
        out_objects[cnt].cov_xx       = tr->P[0][0];
        out_objects[cnt].cov_yy       = tr->P[1][1];
        out_objects[cnt].cov_xy       = tr->P[0][1];
        out_objects[cnt].confirmed    = (tr->hit_count >= EKF_HIT_CONFIRM) ? 1 : 0;
        cnt++;
    }
    *out_n = cnt;
    free(match);
}
