#pragma once
#ifndef CFAR_H
#define CFAR_H

#include "radar_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 2D CA-CFAR (Summed Area Table 기반)
 *   rdm_pow         : [n_doppler × n_range]  선형 파워
 *   out_threshold   : [n_doppler × n_range]  셀별 임계값 (선형)
 *   out_detections  : [n_doppler × n_range]  탐지 여부 (1=탐지, 0=비탐지)
 */
void cfar_2d(const double *rdm_pow,
             const RadarParams *p,
             double *out_threshold,
             int    *out_detections);

#ifdef __cplusplus
}
#endif

#endif /* CFAR_H */
