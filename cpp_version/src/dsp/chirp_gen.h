#pragma once
#ifndef CHIRP_GEN_H
#define CHIRP_GEN_H

#include "radar_types.h"
#include <fftw3.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TX chirp 생성
 *   s_tx(t) = exp(j * pi * mu * t^2)
 *
 * out_chirp[n] : 복소 TX chirp (fftw_complex = double[2])
 * out_freq[n]  : 순간 주파수 (Hz), mu * t
 * n_sample     : 샘플 수 (= d->n_sample)
 */
void chirp_gen(const RadarParams *p, const RadarParamsDerived *d,
               fftw_complex *out_chirp, double *out_freq, int n_sample);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_GEN_H */
