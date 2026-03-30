#pragma once
#ifndef AOA_FFT_H
#define AOA_FFT_H

#include "radar_types.h"
#include <fftw3.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AoA FFT — 탐지된 (doppler_bin, range_bin) 위치에서 3rd FFT
 *
 * rdm_cube    : [n_doppler × n_range/2 × n_virtual]  RDM 큐브 (복소수)
 * detections  : [n_doppler × n_range/2]  CFAR 탐지 여부
 * out_points  : 탐지 결과 배열 (호출자가 충분한 크기로 할당)
 * out_n       : 실제 탐지된 포인트 수
 */
void aoa_fft(const fftw_complex *rdm_cube,
             const int *detections,
             const double *rdm_db,
             const RadarParams *p, const RadarParamsDerived *d,
             const double *range_axis, const double *velocity_axis,
             const double *angle_axis,
             DetectionPoint *out_points, int *out_n);

/*
 * RDM 큐브 계산 — beat_cube → [n_doppler × n_range/2 × n_virtual]
 *
 * beat_cube   : [n_chirp × n_sample × n_virtual]
 * out_rdm_cube: [n_doppler × n_range/2 × n_virtual]
 */
void rdm_cube_calc(const fftw_complex *beat_cube,
                   const RadarParams *p, const RadarParamsDerived *d,
                   fftw_complex *out_rdm_cube);

#ifdef __cplusplus
}
#endif

#endif /* AOA_FFT_H */
