#include "range_fft.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void range_fft(const fftw_complex *beat_matrix,
               const RadarParams *p, const RadarParamsDerived *d,
               fftw_complex *out_rng_fft,
               double *out_profile_db)
{
    int Nc   = p->n_chirp;
    int Ns   = d->n_sample;
    int Nfft = p->n_range_fft;
    int Nhalf = Nfft / 2;

    /* Ns > Nfft 이면 Nfft 개만 복사 (zero-padding은 Ns < Nfft일 때만) */
    int copy_len = (Ns < Nfft) ? Ns : Nfft;

    /* Hanning 윈도우 (copy_len 기준) */
    double *win = (double *)malloc(copy_len * sizeof(double));
    for (int n = 0; n < copy_len; n++)
        win[n] = 0.5 * (1.0 - cos(2.0 * RADAR_PI * n / (copy_len - 1)));

    fftw_complex *buf = (fftw_complex *)fftw_malloc(Nfft * sizeof(fftw_complex));
    fftw_plan plan = fftw_plan_dft_1d(Nfft, buf, buf, FFTW_FORWARD, FFTW_ESTIMATE);

    /* 평균 프로파일 누산 */
    double *acc = (double *)calloc(Nhalf, sizeof(double));

    for (int nc = 0; nc < Nc; nc++) {
        /* 윈도우 적용 후 버퍼에 복사 (zero-padding 포함) */
        memset(buf, 0, Nfft * sizeof(fftw_complex));
        for (int ns = 0; ns < copy_len; ns++) {
            long long src = (long long)nc * Ns + ns;
            buf[ns][0] = beat_matrix[src][0] * win[ns];
            buf[ns][1] = beat_matrix[src][1] * win[ns];
        }

        fftw_execute(plan);

        /* 양의 주파수 절반만 저장 */
        for (int k = 0; k < Nhalf; k++) {
            long long dst = (long long)nc * Nhalf + k;
            out_rng_fft[dst][0] = buf[k][0];
            out_rng_fft[dst][1] = buf[k][1];
            acc[k] += sqrt(buf[k][0]*buf[k][0] + buf[k][1]*buf[k][1]);
        }
    }

    /* 평균 dB 프로파일 */
    for (int k = 0; k < Nhalf; k++) {
        double mag = acc[k] / Nc;
        out_profile_db[k] = 20.0 * log10(mag + 1e-12);
    }

    fftw_destroy_plan(plan);
    fftw_free(buf);
    free(acc);
    free(win);
}
