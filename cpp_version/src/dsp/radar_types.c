#include "radar_types.h"
#include <stdlib.h>
#include <math.h>

RadarParams params_default(void)
{
    RadarParams p;
    p.fc_hz        = 77e9;
    p.bandwidth_hz = 150e6;
    p.chirp_dur_s  = 40e-6;
    p.prf_hz       = 1000.0;
    p.fs_hz        = 15e6;
    p.n_chirp      = 64;
    p.n_tx         = 2;
    p.n_rx         = 4;
    p.noise_std    = 2e-5;
    p.cfar_pfa     = 1e-4;
    p.cfar_guard   = 2;
    p.cfar_train   = 8;
    p.n_range_fft  = 512;
    p.n_doppler_fft= 128;
    p.n_angle_fft  = 256;
    return p;
}

RadarParamsDerived params_derive(const RadarParams *p)
{
    RadarParamsDerived d;
    d.lam              = RADAR_C / p->fc_hz;
    d.mu               = p->bandwidth_hz / p->chirp_dur_s;
    d.t_pri            = 1.0 / p->prf_hz;
    d.n_sample         = (int)(p->fs_hz * p->chirp_dur_s);
    d.n_virtual        = p->n_tx * p->n_rx;
    d.d_elem           = d.lam / 2.0;
    d.range_resolution = RADAR_C / (2.0 * p->bandwidth_hz);
    d.range_max_m      = p->fs_hz * RADAR_C / (4.0 * d.mu);
    d.velocity_max_mps = d.lam * p->prf_hz / 4.0;
    d.velocity_resolution = d.lam / (2.0 * p->n_chirp * d.t_pri);
    return d;
}

double *range_axis_alloc(const RadarParams *p, const RadarParamsDerived *d)
{
    int n = p->n_range_fft / 2;
    double *axis = (double *)malloc(n * sizeof(double));
    if (!axis) return NULL;
    for (int k = 0; k < n; k++) {
        double freq = (double)k * p->fs_hz / p->n_range_fft;
        axis[k] = freq * RADAR_C / (2.0 * d->mu);
    }
    return axis;
}

double *velocity_axis_alloc(const RadarParams *p, const RadarParamsDerived *d)
{
    int n = p->n_doppler_fft;
    double *axis = (double *)malloc(n * sizeof(double));
    if (!axis) return NULL;
    for (int k = 0; k < n; k++) {
        /* fftshift: k -> k - N/2 */
        int ks = k - n / 2;
        double fd = (double)ks / ((double)n * d->t_pri);
        axis[k] = -fd * d->lam / 2.0;   /* 접근 = 양수 */
    }
    return axis;
}

double *angle_axis_alloc(const RadarParams *p, const RadarParamsDerived *d)
{
    int n = p->n_angle_fft;
    double *axis = (double *)malloc(n * sizeof(double));
    if (!axis) return NULL;
    for (int k = 0; k < n; k++) {
        int ks = k - n / 2;
        double fs = (double)ks / (double)n;
        double sin_t = fs * d->lam / d->d_elem;
        if (sin_t >  1.0) sin_t =  1.0;
        if (sin_t < -1.0) sin_t = -1.0;
        axis[k] = asin(sin_t) * 180.0 / RADAR_PI;
    }
    return axis;
}
