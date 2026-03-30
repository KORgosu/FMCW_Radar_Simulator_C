#include "aoa_fft.h"
#include "range_fft.h"
#include "doppler_fft.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void rdm_cube_calc(const fftw_complex *beat_cube,
                   const RadarParams *p, const RadarParamsDerived *d,
                   fftw_complex *out_rdm_cube)
{
    int Nc    = p->n_chirp;
    int Ns    = d->n_sample;
    int Nv    = d->n_virtual;
    int Nhalf = p->n_range_fft / 2;
    int Nd    = p->n_doppler_fft;

    /* 임시 버퍼 */
    fftw_complex *beat_ch  = (fftw_complex *)fftw_malloc((long long)Nc * Ns * sizeof(fftw_complex));
    fftw_complex *rng_ch   = (fftw_complex *)fftw_malloc((long long)Nc * Nhalf * sizeof(fftw_complex));
    double       *dummy_db = (double *)malloc(Nhalf * sizeof(double));
    fftw_complex *rdm_ch   = (fftw_complex *)fftw_malloc((long long)Nd * Nhalf * sizeof(fftw_complex));
    double       *dummy_pow= (double *)malloc((long long)Nd * Nhalf * sizeof(double));
    double       *dummy_rdb= (double *)malloc((long long)Nd * Nhalf * sizeof(double));

    for (int nv = 0; nv < Nv; nv++) {
        /* beat_cube에서 가상 채널 nv 추출 */
        for (int nc = 0; nc < Nc; nc++)
            for (int ns = 0; ns < Ns; ns++) {
                long long src = ((long long)nc * Ns + ns) * Nv + nv;
                long long dst = (long long)nc * Ns + ns;
                beat_ch[dst][0] = beat_cube[src][0];
                beat_ch[dst][1] = beat_cube[src][1];
            }

        range_fft(beat_ch, p, d, rng_ch, dummy_db);
        doppler_fft(rng_ch, p, d, rdm_ch, dummy_rdb, dummy_pow);

        /* out_rdm_cube[kd][kr][nv] 에 저장 */
        for (int kd = 0; kd < Nd; kd++)
            for (int kr = 0; kr < Nhalf; kr++) {
                long long src = (long long)kd * Nhalf + kr;
                long long dst = ((long long)kd * Nhalf + kr) * Nv + nv;
                out_rdm_cube[dst][0] = rdm_ch[src][0];
                out_rdm_cube[dst][1] = rdm_ch[src][1];
            }
    }

    fftw_free(beat_ch); fftw_free(rng_ch); fftw_free(rdm_ch);
    free(dummy_db); free(dummy_pow); free(dummy_rdb);
}

void aoa_fft(const fftw_complex *rdm_cube,
             const int *detections,
             const double *rdm_db,
             const RadarParams *p, const RadarParamsDerived *d,
             const double *range_axis, const double *velocity_axis,
             const double *angle_axis,
             DetectionPoint *out_points, int *out_n)
{
    int Nd    = p->n_doppler_fft;
    int Nr    = p->n_range_fft / 2;
    int Nv    = d->n_virtual;
    int Na    = p->n_angle_fft;

    fftw_complex *va_sig  = (fftw_complex *)fftw_malloc(Na * sizeof(fftw_complex));
    fftw_plan     plan    = fftw_plan_dft_1d(Na, va_sig, va_sig, FFTW_FORWARD, FFTW_ESTIMATE);

    int count = 0;
    int max_pts = Nd * Nr;

    for (int kd = 0; kd < Nd && count < max_pts; kd++) {
        for (int kr = 0; kr < Nr && count < max_pts; kr++) {
            long long cell = (long long)kd * Nr + kr;
            if (!detections[cell]) continue;

            /* 가상 배열 신호 추출 후 zero-padding */
            memset(va_sig, 0, Na * sizeof(fftw_complex));
            for (int nv = 0; nv < Nv && nv < Na; nv++) {
                long long src = cell * Nv + nv;
                va_sig[nv][0] = rdm_cube[src][0];
                va_sig[nv][1] = rdm_cube[src][1];
            }

            fftw_execute(plan);

            /* 최대 피크 탐색 (fftshift 후 각도 축과 대응) */
            int peak = 0;
            double peak_mag = 0.0;
            for (int ka = 0; ka < Na; ka++) {
                double mag = sqrt(va_sig[ka][0]*va_sig[ka][0] + va_sig[ka][1]*va_sig[ka][1]);
                if (mag > peak_mag) { peak_mag = mag; peak = ka; }
            }
            /* fftshift: 실제 각도 인덱스 */
            int peak_shift = (peak + Na / 2) % Na;

            out_points[count].range_m      = range_axis[kr];
            out_points[count].velocity_mps = velocity_axis[kd];
            out_points[count].angle_deg    = angle_axis[peak_shift];
            out_points[count].power_db     = rdm_db[cell];
            out_points[count].range_bin    = kr;
            out_points[count].doppler_bin  = kd;
            count++;
        }
    }

    *out_n = count;
    fftw_destroy_plan(plan);
    fftw_free(va_sig);
}
