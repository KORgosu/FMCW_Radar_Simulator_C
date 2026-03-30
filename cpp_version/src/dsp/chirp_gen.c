#include "chirp_gen.h"
#include <math.h>

void chirp_gen(const RadarParams *p, const RadarParamsDerived *d,
               fftw_complex *out_chirp, double *out_freq, int n_sample)
{
    double dt = p->chirp_dur_s / (double)(n_sample - 1);
    for (int n = 0; n < n_sample; n++) {
        double t   = n * dt;
        double phi = RADAR_PI * d->mu * t * t;
        out_chirp[n][0] = cos(phi);   /* Re */
        out_chirp[n][1] = sin(phi);   /* Im */
        out_freq[n]     = d->mu * t;  /* 순간 주파수 (Hz) */
    }
}
