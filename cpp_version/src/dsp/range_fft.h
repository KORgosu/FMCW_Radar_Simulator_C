#pragma once
#ifndef RANGE_FFT_H
#define RANGE_FFT_H

#include "radar_types.h"
#include <fftw3.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Range-FFT
 *   beat_matrix : [n_chirp × n_sample]  (fftw_complex, 행 우선)
 *   out_rng_fft : [n_chirp × n_range_fft/2]  Range-FFT 결과 (복소수)
 *   out_profile : [n_range_fft/2]  평균 Range Profile (dB)
 *
 * plan_cache: NULL 전달 시 매번 plan 생성 (느림).
 *             plan 재사용 시 외부에서 fftw_plan 포인터를 넘긴다.
 */
void range_fft(const fftw_complex *beat_matrix,
               const RadarParams *p, const RadarParamsDerived *d,
               fftw_complex *out_rng_fft,
               double *out_profile_db);

#ifdef __cplusplus
}
#endif

#endif /* RANGE_FFT_H */
