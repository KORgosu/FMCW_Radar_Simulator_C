#include "cfar.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* 2D Summed Area Table (SAT) 빌드 */
static double *sat_build(const double *data, int rows, int cols)
{
    double *sat = (double *)calloc((rows + 1) * (cols + 1), sizeof(double));
    if (!sat) return NULL;
    for (int r = 1; r <= rows; r++)
        for (int c = 1; c <= cols; c++) {
            sat[r*(cols+1)+c] = data[(r-1)*cols+(c-1)]
                + sat[(r-1)*(cols+1)+c]
                + sat[r*(cols+1)+(c-1)]
                - sat[(r-1)*(cols+1)+(c-1)];
        }
    return sat;
}

/* SAT 영역 합 (r1,c1) ~ (r2,c2) 포함, 0-indexed */
static inline double sat_sum(const double *sat, int cols,
                              int r1, int c1, int r2, int c2)
{
    if (r1 > r2 || c1 > c2) return 0.0;
    int W = cols + 1;
    return sat[(r2+1)*W+(c2+1)]
         - sat[r1    *W+(c2+1)]
         - sat[(r2+1)*W+c1    ]
         + sat[r1    *W+c1    ];
}

void cfar_2d(const double *rdm_pow,
             const RadarParams *p,
             double *out_threshold,
             int    *out_detections)
{
    int Nd = p->n_doppler_fft;
    int Nr = p->n_range_fft / 2;
    int ng = p->cfar_guard;
    int nt = p->cfar_train;

    int half_out = ng + nt;          /* 한쪽 guard+train 합 */
    int outer    = 2 * half_out + 1;
    int inner    = 2 * ng + 1;
    int N_train  = outer * outer - inner * inner;

    double alpha = N_train * (pow(p->cfar_pfa, -1.0 / N_train) - 1.0);

    double *sat = sat_build(rdm_pow, Nd, Nr);
    if (!sat) return;

    for (int rd = 0; rd < Nd; rd++) {
        for (int rr = 0; rr < Nr; rr++) {
            /* outer 영역 클램핑 */
            int r1o = rd - half_out; if (r1o < 0)  r1o = 0;
            int r2o = rd + half_out; if (r2o >= Nd) r2o = Nd - 1;
            int c1o = rr - half_out; if (c1o < 0)  c1o = 0;
            int c2o = rr + half_out; if (c2o >= Nr) c2o = Nr - 1;
            /* inner (guard) 영역 클램핑 */
            int r1i = rd - ng;       if (r1i < 0)  r1i = 0;
            int r2i = rd + ng;       if (r2i >= Nd) r2i = Nd - 1;
            int c1i = rr - ng;       if (c1i < 0)  c1i = 0;
            int c2i = rr + ng;       if (c2i >= Nr) c2i = Nr - 1;

            double sum_outer = sat_sum(sat, Nr, r1o, c1o, r2o, c2o);
            double sum_inner = sat_sum(sat, Nr, r1i, c1i, r2i, c2i);
            double noise_mean = (sum_outer - sum_inner) / N_train;
            if (noise_mean < 1e-30) noise_mean = 1e-30;

            double thr = alpha * noise_mean;
            long long idx = (long long)rd * Nr + rr;
            out_threshold[idx]  = thr;
            out_detections[idx] = (rdm_pow[idx] > thr) ? 1 : 0;
        }
    }

    free(sat);
}
