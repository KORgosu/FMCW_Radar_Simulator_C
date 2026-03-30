#pragma once
#ifndef DOPPLER_FFT_H
#define DOPPLER_FFT_H

#include "radar_types.h"
#include <fftw3.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 2D Doppler FFT → Range-Doppler Map
 *   rng_fft_mat : [n_chirp × n_range_fft/2]  Range-FFT 결과
 *   out_rdm     : [n_doppler_fft × n_range_fft/2]  RDM (복소수, fftshift 적용)
 *   out_rdm_db  : [n_doppler_fft × n_range_fft/2]  RDM (dB)
 *   out_rdm_pow : [n_doppler_fft × n_range_fft/2]  RDM (선형 파워, CFAR용)
 */
void doppler_fft(const fftw_complex *rng_fft_mat,
                 const RadarParams *p, const RadarParamsDerived *d,
                 fftw_complex *out_rdm,
                 double *out_rdm_db,
                 double *out_rdm_pow);

#ifdef __cplusplus
}
#endif

#endif /* DOPPLER_FFT_H */
