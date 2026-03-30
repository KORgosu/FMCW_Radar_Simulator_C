#pragma once
#ifndef BEAT_GEN_H
#define BEAT_GEN_H

#include "radar_types.h"
#include <fftw3.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Beat cube 생성
 *   s_beat(n,t) = A * exp(j*(2π*fb_n*t + φ_n)) * exp(j*φ_spatial_m)
 *
 * out_cube: [n_chirp × n_sample × n_virtual] fftw_complex 배열 (행 우선)
 *           인덱스: [chirp*n_sample*n_virtual + sample*n_virtual + v]
 *
 * 호출자가 fftw_malloc으로 할당한 버퍼를 넘겨야 함
 */
void beat_gen(const Target *targets, int n_targets,
              const RadarParams *p, const RadarParamsDerived *d,
              fftw_complex *out_cube);

/* 진폭 계산: sqrt(rcs) / range^2 */
double target_amplitude(const Target *t);

#ifdef __cplusplus
}
#endif

#endif /* BEAT_GEN_H */
