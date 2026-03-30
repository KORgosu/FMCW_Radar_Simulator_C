#include "beat_gen.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Box-Muller 정규 난수 */
static double randn(void)
{
    double u1 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
    double u2 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * RADAR_PI * u2);
}

double target_amplitude(const Target *t)
{
    double rcs = t->rcs_m2 > 1e-12 ? t->rcs_m2 : 1e-12;
    double r   = t->range_m > 1.0  ? t->range_m : 1.0;
    return sqrt(rcs) / (r * r);
}

void beat_gen(const Target *targets, int n_targets,
              const RadarParams *p, const RadarParamsDerived *d,
              fftw_complex *out_cube)
{
    int Nc = p->n_chirp;
    int Ns = d->n_sample;
    int Nv = d->n_virtual;
    long long total = (long long)Nc * Ns * Nv;

    /* 0으로 초기화 */
    memset(out_cube, 0, total * sizeof(fftw_complex));

    /* fast-time 벡터 */
    double dt = p->chirp_dur_s / (double)(Ns - 1);

    /* 가상 배열 위치 [Nv] */
    double *va_pos = (double *)malloc(Nv * sizeof(double));
    {
        int v = 0;
        for (int tx = 0; tx < p->n_tx; tx++)
            for (int rx = 0; rx < p->n_rx; rx++, v++)
                va_pos[v] = (tx * p->n_rx + rx) * d->d_elem;
    }

    /* 각 타겟 기여 합산 */
    for (int ti = 0; ti < n_targets; ti++) {
        const Target *tgt = &targets[ti];
        double A         = target_amplitude(tgt);
        double theta_rad = tgt->angle_deg * RADAR_PI / 180.0;
        double sin_th    = sin(theta_rad);

        for (int nc = 0; nc < Nc; nc++) {
            /* 이 처프에서의 타겟 거리 */
            double R_n  = tgt->range_m + tgt->velocity_mps * nc * d->t_pri;
            double fb_n = 2.0 * R_n * d->mu / RADAR_C;
            double phi0 = -4.0 * RADAR_PI * R_n / d->lam;

            for (int ns = 0; ns < Ns; ns++) {
                double t   = ns * dt;
                double phi = 2.0 * RADAR_PI * fb_n * t + phi0;
                double re_beat = A * cos(phi);
                double im_beat = A * sin(phi);

                for (int nv = 0; nv < Nv; nv++) {
                    double phi_sp = 2.0 * RADAR_PI / d->lam * va_pos[nv] * sin_th;
                    long long idx = ((long long)nc * Ns + ns) * Nv + nv;
                    /* exp(j*(phi+phi_sp)) = beat * exp(j*phi_sp) */
                    double cs = cos(phi_sp), ss = sin(phi_sp);
                    out_cube[idx][0] += re_beat * cs - im_beat * ss;
                    out_cube[idx][1] += re_beat * ss + im_beat * cs;
                }
            }
        }
    }

    /* 복소 가우시안 잡음 추가 */
    for (long long i = 0; i < total; i++) {
        out_cube[i][0] += p->noise_std * randn();
        out_cube[i][1] += p->noise_std * randn();
    }

    free(va_pos);
}
