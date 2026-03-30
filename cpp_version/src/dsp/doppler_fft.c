#include "doppler_fft.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void doppler_fft(const fftw_complex *rng_fft_mat,
                 const RadarParams *p, const RadarParamsDerived *d,
                 fftw_complex *out_rdm,
                 double *out_rdm_db,
                 double *out_rdm_pow)
{
    int Nc    = p->n_chirp;
    int Nhalf = p->n_range_fft / 2;
    int Nd    = p->n_doppler_fft;

    /* slow-time Hanning 윈도우 */
    double *win_slow = (double *)malloc(Nc * sizeof(double));
    for (int n = 0; n < Nc; n++)
        win_slow[n] = 0.5 * (1.0 - cos(2.0 * RADAR_PI * n / (Nc - 1)));

    fftw_complex *col = (fftw_complex *)fftw_malloc(Nd * sizeof(fftw_complex));
    fftw_plan plan = fftw_plan_dft_1d(Nd, col, col, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int kr = 0; kr < Nhalf; kr++) {
        /* 각 range bin 에 대해 slow-time FFT */
        memset(col, 0, Nd * sizeof(fftw_complex));
        for (int nc = 0; nc < Nc; nc++) {
            long long src = (long long)nc * Nhalf + kr;
            col[nc][0] = rng_fft_mat[src][0] * win_slow[nc];
            col[nc][1] = rng_fft_mat[src][1] * win_slow[nc];
        }

        fftw_execute(plan);

        /* fftshift 적용하여 저장: 출력 인덱스 kd → fftshift 위치 */
        for (int kd = 0; kd < Nd; kd++) {
            int kd_shift = (kd + Nd / 2) % Nd;   /* fftshift */
            long long dst = (long long)kd_shift * Nhalf + kr;
            out_rdm[dst][0] = col[kd][0];
            out_rdm[dst][1] = col[kd][1];
            double mag = sqrt(col[kd][0]*col[kd][0] + col[kd][1]*col[kd][1]);
            out_rdm_db [dst] = 20.0 * log10(mag + 1e-12);
            out_rdm_pow[dst] = mag * mag;
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(col);
    free(win_slow);
}
