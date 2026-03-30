# FMCW Radar Simulator — C++ 버전 기획서

> 세종대학교 국방레이더기술연구실
> 작성일: 2026-03-27
> Design by KORgosu(Su Hwan Park)
> 목적: Python 버전(`../python_version`)과 동일한 기능을 C++ + Qt6으로 구현
> 학습 목표: MATLAB → C 신호처리 코드 변환 역량 확보

---

## 1. 프로젝트 개요

### 1.1 목표
- Python(NumPy/SciPy) 기반 FMCW 신호처리 파이프라인을 **Pure C DSP 함수**로 재구현
- GUI는 **Qt6 (C++)** 으로 Python의 PyQt5 구조를 그대로 이식
- DSP 코어와 GUI 레이어를 명확히 분리하여, DSP 코어는 추후 임베디드/DSP 보드에 이식 가능한 형태로 작성

### 1.2 Python 버전과의 대응 관계

| Python | C++ |
|--------|-----|
| `numpy` 배열 (`ndarray`) | C 배열 / `std::vector<float>` |
| `numpy.fft.fft` | FFTW3 (`fftw_plan`) |
| `scipy.ndimage.uniform_filter` | 자체 구현 2D Box Filter (C) |
| `dataclass` (RadarParams, Target) | C `struct` |
| PyQt5 위젯 | Qt6 위젯 (동일 API 구조) |
| `matplotlib` 플롯 | Qt6 + **QCustomPlot 2.1.1+** |
| `@dataclass` 파생 속성 (`@property`) | C++ getter 메서드 또는 inline 함수 |

---

## 2. 기술 스택

| 분류 | 선택 | 비고 |
|------|------|------|
| 언어 | C17 (DSP 코어) + C++17 (GUI 연결, Qt) | DSP는 Pure C |
| GUI 프레임워크 | **Qt6** (Community Edition) | PyQt5 → Qt6 C++ 이식 |
| FFT 라이브러리 | **FFTW3** | numpy.fft 동등 성능 |
| 플롯 라이브러리 | **QCustomPlot 2.1.1+** | matplotlib 대체. v2.1.0부터 Qt6 공식 지원. vcpkg 설치 시 버전 고정 필요 (`"qcustomplot[qt6]"`) |
| 빌드 시스템 | **CMake 3.20+** | |
| 컴파일러 | MSVC 2022 또는 MinGW-w64 | Windows 기준 |
| 패키지 관리 | **vcpkg** | FFTW3, QCustomPlot 의존성 관리 |
| 선형대수 (EKF) | **자체 구현 4×4 행렬** | Eigen 불필요, 상태벡터 고정 크기(4)이므로 직접 구현 |

---

## 3. 아키텍처

```
┌──────────────────────────────────────────────┐
│              Qt6 GUI Layer (C++)             │
│  MainWindow / ParamPanel / SceneWidget       │
│  Tab1~Tab4 / QCustomPlot 기반 플롯 위젯       │
├──────────────────────────────────────────────┤
│           C++ Simulator Wrapper              │
│  FMCWSimulator class                        │
│  (SimResult 구조체 관리, DSP 코어 호출)       │
├──────────────────────────────────────────────┤
│         Pure C DSP Core (MATLAB→C)           │
│  chirp_gen / beat_gen / range_fft            │
│  doppler_fft / cfar / aoa_fft               │
│  clustering (DBSCAN) / ekf_tracker           │
├──────────────────────────────────────────────┤
│              FFTW3 Library                   │
└──────────────────────────────────────────────┘
```

### 데이터 흐름

```
[GUI 파라미터 변경]
        │
        ▼
FMCWSimulator::compute(targets[], params)          ← 단일 프레임 계산
        │
        ├─ 1. chirp_gen()         → tx_chirp[], tx_freq[]
        ├─ 2. beat_cube_gen()     → beat_cube[N_chirp × N_sample × N_virtual]
        ├─ 3. range_fft()         → range_profile_db[]
        ├─ 4. rdm_2dfft()         → rdm_db[N_doppler × N_range]
        ├─ 5. cfar_2d()           → cfar_detections[N_d × N_r]
        ├─ 6. aoa_fft()           → detection_points[]
        ├─ 7. dbscan_cluster()    → point_cloud[] (군집 무게중심)
        └─ 8. ekf_update()        → object_list[] (추적 결과)
                │
                ▼
        SimResult → emit SimResultReady 시그널
                │
        ┌───────┴────────┐
        ▼                ▼
   MainWindow        PipelineInspector
   (탭 플롯 갱신)     (단계별 I/O 갱신)
```

### 멀티프레임 구조 (EKF용)

EKF는 프레임 간 상태를 유지해야 하므로 `FMCWSimulator`는 두 가지 모드로 동작한다.

```
[단일 프레임 모드]  파라미터/타겟 변경 시 → compute() 1회 호출 → SimResult 반환
                   EKF는 매번 초기화 (트랙 이력 없음)

[멀티프레임 모드]   타겟이 시간에 따라 이동하는 시나리오 자동 생성
                   frame 0, 1, 2 ... N 순차 계산
                   EKF tracker가 프레임 간 상태(x̂, P) 유지
                   GUI에서 [▶ 재생] / [■ 정지] / [▶| 한 프레임] 버튼으로 제어
```

---

## 4. 디렉토리 구조

```
cpp_version/
├── PLAN.md                        ← 이 파일
├── CMakeLists.txt                 ← 최상위 빌드 설정
├── vcpkg.json                     ← 의존성 선언 (fftw3, qcustomplot[qt6])
│
├── src/
│   ├── dsp/                       ← Pure C DSP 코어 (MATLAB→C 이식 대상)
│   │   ├── radar_types.h          ← RadarParams, Target, SimResult, TrackState 구조체
│   │   ├── chirp_gen.c / .h       ← TX chirp 생성
│   │   ├── beat_gen.c / .h        ← Beat signal / Beat cube 생성
│   │   ├── range_fft.c / .h       ← Range-FFT, Range Profile
│   │   ├── doppler_fft.c / .h     ← 2D-FFT → Range-Doppler Map
│   │   ├── cfar.c / .h            ← 2D CA-CFAR
│   │   ├── aoa_fft.c / .h         ← 3rd FFT → AoA 추정 + Detection Points
│   │   ├── clustering.c / .h      ← DBSCAN 군집화 (정규화 포함)
│   │   ├── data_association.c / .h ← 비용 행렬 + Hungarian Algorithm
│   │   └── ekf_tracker.c / .h     ← EKF 추적기 (predict/update/초기화/소멸)
│   │
│   ├── core/                      ← C++ Wrapper
│   │   ├── simulator.h / .cpp     ← FMCWSimulator 클래스 (단일/멀티프레임 모드)
│   │   └── sim_result.h           ← SimResult (std::vector 기반)
│   │
│   └── ui/                        ← Qt6 GUI
│       ├── main_window.h / .cpp   ← MainWindow (TitleBar / Splitter / StatusBar)
│       ├── param_panel.h / .cpp   ← 슬라이더 파라미터 패널
│       ├── scene_widget.h / .cpp  ← 타겟 배치 씬 (QPainter 기반)
│       ├── plot_widget.h / .cpp   ← QCustomPlot 공통 베이스
│       ├── pipeline_inspector.h / .cpp  ← 단계별 I/O 시각화 별도 창
│       └── tabs/
│           ├── tab1_signal.h / .cpp    ← 신호 원리 탭
│           ├── tab2_rdm.h / .cpp       ← Range-Doppler 탭
│           ├── tab3_cfar.h / .cpp      ← CFAR 탐지 탭
│           └── tab4_pipeline.h / .cpp  ← 통합 파이프라인 탭
│
├── resources/
│   └── style/
│       └── dark_radar.qss         ← 동일한 다크 테마 스타일시트
│
└── main.cpp                       ← 진입점 (QApplication + MainWindow)
```

---

## 5. 기능 명세 (Python 버전 동등)

### 5.1 DSP 코어 함수 목록

| 함수 (C) | Python 대응 | 역할 |
|----------|------------|------|
| `chirp_gen()` | `_compute_waveform()` | TX chirp 순간 주파수, 복소 신호 생성 |
| `beat_cube_gen()` | `_generate_beat_cube()` | 전 타겟 beat signal 합산 + 잡음 추가 |
| `range_fft()` | `_compute_range_profile()` | Hanning 윈도우 + FFT → Range Profile |
| `rdm_2dfft()` | `_rdm()` | 2D FFT → Range-Doppler Map |
| `cfar_2d()` | `_cfar()` | 2D CA-CFAR (Box filter 기반) |
| `rdm_cube()` | `_rdm_cube()` | MIMO 채널별 RDM 계산 |
| `aoa_fft()` | `_aoa_point_cloud()` | 3rd FFT → 각도 추정 → Detection Points |
| `dbscan_cluster()` | *(신규)* | Detection Points → 군집 무게중심 Point Cloud |
| `data_association()` | *(신규)* | 비용 행렬 계산 + Hungarian Algorithm → 클러스터-트랙 매칭 |
| `ekf_update()` | *(신규)* | 매칭된 쌍 → EKF predict/update, 신규 트랙 초기화, 소멸 트랙 삭제 |

### 5.2 데이터 구조

```c
/* radar_types.h */

typedef struct {
    double range_m;
    double velocity_mps;
    double angle_deg;
    double rcs_m2;
} Target;

typedef struct {
    double fc_hz;
    double bandwidth_hz;
    double chirp_dur_s;
    double prf_hz;
    double fs_hz;
    int    n_chirp;
    int    n_tx;
    int    n_rx;
    double noise_std;
    double cfar_pfa;
    int    cfar_guard;
    int    cfar_train;
    int    n_range_fft;
    int    n_doppler_fft;
    int    n_angle_fft;
} RadarParams;

typedef struct {
    /* 파생 값 (compute 전 미리 계산) */
    double lam;
    double mu;
    double t_pri;
    int    n_sample;
    int    n_virtual;
    double d_elem;
    double range_resolution;
    double range_max_m;
    double velocity_max_mps;
} RadarParamsDerived;

/* EKF 단일 트랙 상태 (Cartesian 상태벡터) */
typedef struct {
    int    id;
    double x[4];      /* 상태벡터: [x_pos, y_pos, vx, vy] (m, m/s) */
    double P[4][4];   /* 오차 공분산 행렬 */
    int    miss_count; /* 연속 미탐지 프레임 수 (일정 이상이면 트랙 삭제) */
    int    hit_count;  /* 확정 트랙 판단용 */
} TrackState;

/*
 * EKF 행렬 설계 (상수 속도 모델)
 *
 *        [1  0  dt  0 ]          [dt⁴/4  0      dt³/2  0    ]
 *   F =  [0  1  0   dt]   Q = q· [0      dt⁴/4  0      dt³/2]
 *        [0  0  1   0 ]          [dt³/2  0      dt²    0    ]
 *        [0  0  0   1 ]          [0      dt³/2  0      dt²  ]
 *
 *   dt = T_frame = N_chirp × T_PRI   (프레임 주기)
 *   q  = 0.1 m²/s⁴  (가속도 불확실도 초기값, 튜닝 파라미터)
 *
 *   측정벡터 z = [R, v_radial, θ]  (극좌표)
 *   h(x) = [sqrt(x²+y²),  (x·vx+y·vy)/R,  atan2(y,x)]  → 비선형 → Jacobian H 사용
 *
 *   측정 잡음 R_meas = diag([σ_R², σ_v², σ_θ²])
 *     σ_R = range_resolution / 2
 *     σ_v = velocity_resolution / 2
 *     σ_θ = angle_resolution_rad / 2
 */
```

### 5.3 GUI 구성 (Python 버전 동등 + 신규)

| 구성 요소 | 내용 | 비고 |
|-----------|------|------|
| **TitleBar** | 타이틀, 연구실명, 제작자 표시 | Python 동등 |
| **ParamPanel** | 슬라이더 기반 파라미터 입력 (fc, B, Tc, PRF, fs, N_chirp, N_tx, N_rx, noise, CFAR 설정) | Python 동등 |
| **SceneWidget** | Top-down 뷰, 마우스 클릭/드래그로 타겟 추가·이동·삭제, RCS 프리셋 | Python 동등 |
| **Tab1 — 신호 원리** | TX Chirp 주파수 스윕 / TX vs RX 주파수 비교 / Beat Signal / Range Profile | Python 동등 |
| **Tab2 — Range-Doppler** | 2D RDM Heatmap / Range Profile / Doppler Profile | Python 동등 |
| **Tab3 — CFAR 탐지** | RDM + CFAR 임계선 / 탐지 결과 오버레이 | Python 동등 |
| **Tab4 — 통합 파이프라인** | Beat Matrix / RDM / CFAR / Point Cloud 4분할 | Python 동등 |
| **멀티프레임 컨트롤** | ▶ 재생 / ■ 정지 / ▶\| 한 프레임 버튼 + 프레임 카운터 표시 (MainWindow 하단 툴바) | **신규** |
| **Pipeline Inspector** | 별도 창, 툴바 버튼으로 열기. Stage 0~6 단계별 I/O 시각화 | **신규** |
| **StatusBar** | READY / COMPUTING / OK (탐지 N개) / ERROR 상태, 레이더 파라미터 요약 | Python 동등 |

---

## 6. 개발 단계

### Phase 1 — 환경 구성
- [ ] CMake + vcpkg 프로젝트 초기화
- [ ] FFTW3 연동 확인 (단순 FFT 테스트)
- [ ] Qt6 + QCustomPlot 연동 확인 (빈 윈도우)
- [ ] 다크 테마 QSS 적용 확인

### Phase 2 — DSP 코어 구현 (Pure C)
- [ ] `radar_types.h` — 구조체 정의 (RadarParams, Target, TrackState 포함)
- [ ] `chirp_gen.c` — TX chirp 생성 (복소 지수 신호)
- [ ] `beat_gen.c` — Beat cube 생성 (타겟별 beat signal 합산)
- [ ] `range_fft.c` — Hanning 윈도우 + FFTW3 Range-FFT
- [ ] `doppler_fft.c` — 2D-FFT Range-Doppler Map
- [ ] `cfar.c` — 2D CA-CFAR
- [ ] `aoa_fft.c` — 3rd FFT + Detection Points 생성
- [ ] `clustering.c` — DBSCAN (정규화 포함)
- [ ] `data_association.c` — 마할라노비스 거리 비용 행렬 + Hungarian Algorithm
- [ ] `ekf_tracker.c` — EKF (F, H Jacobian, Q, R 행렬 + 트랙 초기화/소멸)

### Phase 3 — C++ Simulator Wrapper
- [ ] `SimResult` — `std::vector` 기반 결과 컨테이너
- [ ] `FMCWSimulator` — 단일 프레임 모드 `compute()` 구현
- [ ] `FMCWSimulator` — 멀티프레임 모드 `step()` 구현 (EKF 상태 유지)
- [ ] `SimResultReady` 시그널 정의 (GUI 연결용)

### Phase 4 — Qt6 GUI 구현
- [ ] `MainWindow` — 레이아웃 구성 (TitleBar / 좌우 Splitter / StatusBar)
- [ ] `MainWindow` — 멀티프레임 재생 컨트롤 (▶ / ■ / ▶| 버튼, QTimer)
- [ ] `ParamPanel` — 슬라이더 파라미터 패널
- [ ] `SceneWidget` — QPainter 기반 Top-down 씬
- [ ] `Tab1Widget` — 신호 원리 플롯 (QCustomPlot 4분할)
- [ ] `Tab2Widget` — Range-Doppler 플롯
- [ ] `Tab3Widget` — CFAR 탐지 플롯
- [ ] `Tab4Widget` — 통합 파이프라인 플롯
- [ ] `PipelineInspectorWindow` — 단계별 I/O 시각화 별도 창

### Phase 5 — 통합 & 검증
- [ ] `SimResultReady` 시그널 → MainWindow / PipelineInspector 동시 연결
- [ ] 성능 측정 및 목표 달성 확인 (목표: 파이프라인 계산 ≤ 50ms/frame)

---

## 7. 주요 구현 고려사항

### 복소수 처리
- Python: `complex128` (numpy) → C: `fftw_complex` (= `double[2]`, `[0]=Re`, `[1]=Im`)
- 또는 C99 `<complex.h>` (`double complex`) 사용 가능

### 메모리 설계

기본 파라미터 기준 주요 버퍼 크기:

| 버퍼 | 크기 계산 | 기본값 |
|------|----------|--------|
| Beat cube | N_chirp × N_sample × N_virtual × 16B | 64×600×8×16 = **4.7 MB** |
| RDM cube | N_doppler × N_range × N_virtual × 16B | 128×256×8×16 = **4.0 MB** |
| RDA (angle) | N_doppler × N_range × N_angle × 8B | 128×256×256×8 = **67 MB** ← 주의 |

> RDA cube는 전체를 메모리에 유지하지 않고 CFAR 탐지점에서만 on-demand로 Angle FFT를 수행하여 메모리를 절약한다 (Python 버전의 `_aoa_point_cloud()` 방식 유지).

- Beat cube, RDM cube: `fftw_malloc` 으로 heap 할당 (SIMD 정렬 보장)
- **FFTW plan 재사용**: `FMCWSimulator` 생성자에서 plan을 한 번 생성하고 멤버로 보유. 파라미터(FFT 크기)가 변경될 때만 plan을 재생성.
- 총 상주 메모리 목표: **≤ 50 MB**

### 윈도우 함수
- Hanning 윈도우: `w[n] = 0.5 * (1 - cos(2π·n / (N-1)))`
- numpy의 `np.hanning(N)` 과 동일

### CFAR Box Filter 구현
- scipy의 `uniform_filter` 대체: 2D Integral Image(Summed Area Table) 기반 고속 구현

### 스레딩
- Qt의 `QtConcurrent::run` 으로 DSP 계산을 GUI 스레드와 분리 (Python의 QTimer 디바운스 구조 대응)
- 계산 완료 후 `SimResultReady(SimResult)` 시그널을 메인 스레드로 emit → GUI 업데이트

### 성능 목표

| 항목 | 목표 |
|------|------|
| 파이프라인 전체 계산 (Stage 0~6) | ≤ 50 ms / frame |
| GUI 업데이트 (플롯 렌더링) | ≤ 16 ms (60fps 기준) |
| 총 상주 메모리 | ≤ 50 MB |
| 파라미터 변경 후 화면 반영 | ≤ 100 ms (체감 반응성) |

---

## 8. 빌드 환경 (Windows)

### 필요 설치
- Visual Studio 2022 (MSVC) 또는 MinGW-w64
- CMake 3.20+
- Qt6 Community — Qt Online Installer로 별도 설치 (vcpkg 미포함)
- vcpkg — FFTW3, QCustomPlot 의존성 관리

### vcpkg.json (의존성 선언)

```json
{
  "name": "fmcw-radar-simulator",
  "version": "1.0.0",
  "dependencies": [
    "fftw3",
    {
      "name": "qcustomplot",
      "version>=": "2.1.1",
      "features": ["qt6"]
    }
  ]
}
```

### CMakeLists.txt 핵심 설정 (예시)

```cmake
cmake_minimum_required(VERSION 3.20)
project(FMCWSimulator LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(FFTW3 CONFIG REQUIRED)
find_package(qcustomplot CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    Qt6::Widgets
    FFTW3::fftw3
    qcustomplot::qcustomplot
)
```

### 빌드 명령

```bash
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=[vcpkg경로]/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_PREFIX_PATH=[Qt6 설치경로]/lib/cmake
cmake --build build --config Release
```

---

## 9. 향후 확장 계획 (v2+)

> Phase 5 완료 후 추가 기능 검토

---

### 기능 1 — 파이프라인 단계별 I/O 시각화 윈도우 (Pipeline Inspector)

#### 개요
FMCW 신호처리 전체 흐름을 단계별로 시각화하는 별도 윈도우.
각 단계의 **입력 데이터**, **수식/알고리즘**, **출력 데이터**를 동시에 표시하여
"어떤 데이터가 어떤 연산을 거쳐 어떤 결과가 되는가"를 직관적으로 확인 가능.
교육 목적 및 디버깅 용도로 활용.

#### 전체 파이프라인 흐름 (7단계)

```
Stage 0  RF Frontend        Radar Params → Beat Matrix [N_chirp × N_sample]
  │
Stage 1  Range FFT (1D)     Beat Matrix  → Range-FFT Matrix [N_chirp × N_range]
  │
Stage 2  Doppler FFT (2D)   Range-FFT Matrix → Range-Doppler Map [N_doppler × N_range]
  │
Stage 3  Angle FFT (3D)     RDM Cube [N_d × N_r × N_virtual] → RDA Cube [N_d × N_r × N_angle]
  │
Stage 4  CFAR               RDM Power Map → Detection Points [(r, v, θ)]
  │
Stage 5  Clustering (DBSCAN) Detection Points → Point Cloud (군집 무게중심)
  │
Stage 6  EKF Tracking       Point Cloud (현재 프레임) → Object List [{ID, R, v, θ, 공분산}]
```

#### 각 단계 상세 명세

**Stage 0 — RF Frontend**
| 항목 | 내용 |
|------|------|
| Input | RadarParams (fc, B, Tc, PRF, fs, N_chirp), Target 목록 |
| 수식 | `s_beat(t) = A · exp(j·2π·fb·t + j·φ_doppler)` <br> `fb = 2·R·μ/c`,  `φ = -4π·R/λ` |
| Output | Beat Matrix [N_chirp × N_sample] (복소수) |
| 시각화 | ① 첫 번째 chirp beat signal (Re/Im, time domain) <br> ② Beat Matrix 전체 (컬러맵, chirp × sample 2D) |

**Stage 1 — Range FFT (1D FFT)**
| 항목 | 내용 |
|------|------|
| Input | Beat Matrix [N_chirp × N_sample] |
| 수식 | `w[n] = 0.5·(1 - cos(2π·n/(N-1)))` (Hanning) <br> `R_FFT[k] = FFT(w · s_beat)[k]`,  `R = k·c/(2·μ·N_fft/fs)` |
| Output | Range-FFT Matrix [N_chirp × N_range], Range Profile (dB) |
| 시각화 | ① Hanning 윈도우 모양 <br> ② 단일 chirp FFT 전/후 비교 (time vs frequency) <br> ③ 평균 Range Profile (dB, 거리 축) |

**Stage 2 — Doppler FFT (2D FFT)**
| 항목 | 내용 |
|------|------|
| Input | Range-FFT Matrix [N_chirp × N_range] |
| 수식 | slow-time Hanning 적용 후 `RDM = FFTshift(FFT(R_FFT, axis=chirp))` <br> `v = -f_d · λ/2`,  `f_d = k/(N_chirp · T_PRI)` |
| Output | Range-Doppler Map [N_doppler × N_range] (복소수, dB) |
| 시각화 | ① RDM 2D 히트맵 (컬러맵) <br> ② Range slice (특정 거리에서의 Doppler 스펙트럼) <br> ③ Doppler slice (특정 속도에서의 Range 프로필) |

**Stage 3 — Angle FFT (3D FFT)**
| 항목 | 내용 |
|------|------|
| Input | RDM Cube [N_d × N_r × N_virtual] (MIMO 가상 배열) |
| 수식 | `AoA_FFT = FFTshift(FFT(va_signal, n=N_angle_fft))` <br> `sin(θ) = f_s · λ/d_elem` |
| Output | Angle Spectrum, Point Cloud (탐지 위치별 각도 추정값) |
| 시각화 | ① 가상 배열 구조도 (N_tx × N_rx → N_virtual 소자 배치) <br> ② 선택된 (range bin, doppler bin)에서의 각도 스펙트럼 <br> ③ Range-Angle 2D 맵 |

**Stage 4 — CFAR 탐지**
| 항목 | 내용 |
|------|------|
| Input | RDM Power Map [N_d × N_r] |
| 수식 | `α = N_train·(Pfa^(-1/N_train) - 1)` <br> `Threshold = α · mean(training cells)` |
| Output | Detection Map (bool [N_d × N_r]), Detection Points |
| 시각화 | ① RDM + CFAR 임계선 오버레이 <br> ② Guard/Training cell 구조 다이어그램 <br> ③ 탐지 포인트 마킹 |

**Stage 5 — Clustering (DBSCAN)**
| 항목 | 내용 |
|------|------|
| Input | Detection Points [(range, velocity, angle, power)] |
| 수식 | DBSCAN: `ε`-이웃 기반 밀도 군집화 |
| 정규화 | `R_n = R / range_max`,  `v_n = v / (2·v_max)`,  `θ_n = θ / 180°` → 각 축 [0,1] 스케일 통일 후 유클리드 거리 계산 |
| 파라미터 | `ε = 0.1` (정규화 공간 기준, 초기값),  `minPts = 2` (탐지점 수가 적으므로 소값 사용) |
| Output | Clustered Point Cloud (군집 ID, 무게중심, 크기) |
| 시각화 | ① 군집 전 scatter plot (Range-Velocity 2D) <br> ② 군집 후 색상별 군집 표시 <br> ③ 군집 무게중심 + 바운딩박스 |

**Stage 6 — EKF 추적**
| 항목 | 내용 |
|------|------|
| Input | Point Cloud 무게중심 (현재 프레임), 이전 프레임 TrackState[] |
| 상태벡터 | `x = [x_pos, y_pos, vx, vy]`  (Cartesian, 단위: m, m/s) |
| 수식 | Predict: `x̂⁻ = F·x̂`,  `P⁻ = F·P·Fᵀ + Q` <br> Update: `K = P⁻·Hᵀ·(H·P⁻·Hᵀ + R)⁻¹`,  `x̂ = x̂⁻ + K·(z - h(x̂⁻))` |
| F 행렬 | 상수 속도 모델, `dt = N_chirp × T_PRI` |
| Q 행렬 | 가속도 잡음 모델, `q = 0.1 m²/s⁴` (초기값, 튜닝 파라미터) |
| R 행렬 | `diag([σ_R², σ_v², σ_θ²])`,  σ = 해당 분해능의 1/2 |
| h(x) | `[sqrt(x²+y²), (x·vx+y·vy)/R, atan2(y,x)]` (비선형 → Jacobian H 선형화) |
| 트랙 관리 | miss_count ≥ 3이면 트랙 삭제,  hit_count ≥ 3이면 확정 트랙으로 격상 |
| Output | Object List [{ID, R, v, θ, 공분산 타원}] |
| 시각화 | ① 예측 위치 vs 측정 위치 비교 <br> ② 불확실도 공분산 타원 (2-sigma) <br> ③ 프레임 간 트랙 궤적 (멀티프레임 모드) |

**Stage 6 — Data Association (EKF 전처리)**

EKF update 전에 반드시 "어느 클러스터가 어느 트랙에 매칭되는가"를 결정해야 한다.

```
Point Cloud (M개 클러스터)
        │
        ▼
  [비용 행렬 계산]  C[i][j] = 트랙 i의 예측 위치와 클러스터 j의 마할라노비스 거리
        │           d²(i,j) = (z_j - h(x̂⁻_i))ᵀ · S⁻¹ · (z_j - h(x̂⁻_i))
        │           S = H·P⁻·Hᵀ + R  (혁신 공분산)
        ▼
  [Hungarian Algorithm]  최소 비용 전역 매칭
        │
        ├─ 매칭된 쌍  → EKF update()
        ├─ 미매칭 트랙 → miss_count++,  predict()만 수행
        └─ 미매칭 클러스터 → 신규 트랙 초기화
```

**트랙 초기화 전략**

새 클러스터가 기존 트랙과 매칭되지 않으면 신규 TrackState를 생성한다.

```c
/* 최초 탐지점 (R, v, θ) 로부터 Cartesian 상태벡터 초기화 */
x̂₀[0] = R * sin(θ_rad);          /* x_pos */
x̂₀[1] = R * cos(θ_rad);          /* y_pos */
x̂₀[2] = v * sin(θ_rad);          /* vx (시선 방향 속도로 근사) */
x̂₀[3] = v * cos(θ_rad);          /* vy */

/* 초기 오차 공분산: 측정 불확실도로 보수적으로 설정 */
P₀ = diag([σ_R²,  σ_R²,  σ_v²,  σ_v²])
```

- 비용 임계값 `d²_max = 9.21` (χ² 분포, 자유도 3, 99% 신뢰구간) 초과 시 매칭 거부

#### UI 구성 (별도 창)

```
┌─────────────────────────────────────────────────────────────┐
│  PIPELINE INSPECTOR                              [X] 닫기   │
├──────────────┬──────────────────────────────────────────────┤
│  [Stage 선택]│                                              │
│  ○ Stage 0  │   [INPUT 시각화]    →  [수식/처리]  →  [OUTPUT 시각화] │
│  ● Stage 1  │                                              │
│  ○ Stage 2  │   파라미터 요약 표시                          │
│  ○ Stage 3  │   데이터 크기: [N × M] / dtype 표시          │
│  ○ Stage 4  │                                              │
│  ○ Stage 5  │                                              │
│  ○ Stage 6  │                                              │
└──────────────┴──────────────────────────────────────────────┘
```

- 좌측: 단계 선택 리스트 (클릭 시 해당 단계 I/O 표시)
- 우측 상단: Input 플롯
- 우측 중앙: 적용 수식 텍스트 + 처리 파라미터 + 데이터 크기 표시
- 우측 하단: Output 플롯
- 메인 윈도우와 연동 (파라미터/타겟 변경 시 자동 갱신)

#### Inspector ↔ MainWindow 데이터 공유 구조

```
MainWindow
  └─ FMCWSimulator::compute() 완료
       └─ emit SimResultReady(const SimResult& result)
                │
        ┌───────┴───────────────────────────────┐
        ▼                                       ▼
  MainWindow::onSimResultReady()      PipelineInspector::onSimResultReady()
  (탭 플롯 갱신)                       (현재 선택된 Stage I/O 갱신)
```

- `SimResult`는 값(value) 복사 전달 → Inspector가 독립적인 스냅샷 보유
- Inspector 창은 `show()` / `hide()` 방식 (메인 창 종료 시 함께 닫힘)
- 메인 윈도우 툴바에 `[Pipeline Inspector]` 버튼 배치

---

## 10. 개발 환경 설치 가이드 (Windows 11)

### 설치 순서

```
① Visual Studio 2022  →  C++ 컴파일러 확보
② CMake               →  빌드 시스템
③ Qt6                 →  GUI 프레임워크
④ vcpkg               →  FFTW3, QCustomPlot 패키지 관리
⑤ cmake 빌드          →  실행 파일 생성
```

---

### Step 1 — Visual Studio 2022

1. https://visualstudio.microsoft.com/downloads/ 에서 **Community 2022** (무료) 다운로드
2. 설치 시 워크로드 선택 화면에서 반드시 체크:
   - ✅ **C++를 사용한 데스크톱 개발**
3. 나머지 기본값으로 설치 (약 8~10 GB, 20~30분 소요)

> **주의**: Visual Studio Code(에디터)와 다른 프로그램임. 반드시 **Visual Studio 2022** 설치.

---

### Step 2 — CMake

1. https://cmake.org/download/ 에서 **Windows x64 Installer** (`.msi`) 다운로드
2. 설치 중 **"Add CMake to the system PATH for all users"** 선택 ← 필수
3. 설치 완료 후 확인:

```bash
cmake --version
# cmake version 3.xx.x
```

---

### Step 3 — Qt6

1. https://www.qt.io/download-qt-installer 에서 **Qt Online Installer** 다운로드
2. Qt 계정 없으면 무료 회원가입 필요
3. 설치 시 아래 항목 선택:

```
Qt
└── Qt 6.7.x (최신 버전)
    ✅ MSVC 2019 64-bit       ← Visual Studio용 (필수)
```

> Qt Concurrent는 별도 항목이 아님. `MSVC 2019 64-bit` 선택 시 자동 포함.

4. 설치 경로 기억 (예: `C:\Qt\6.7.3\msvc2019_64`)

---

### Step 4 — vcpkg + 의존성 설치

**4-1. vcpkg 설치** (PowerShell 또는 Git Bash):

```bash
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

**4-2. 시스템 PATH 등록**:

Windows 검색 → "시스템 환경 변수 편집" → 환경 변수 → Path → 새로 만들기 → `C:\vcpkg` 추가

**4-3. 의존성 설치**:

```bash
cd C:\vcpkg
vcpkg install fftw3:x64-windows
vcpkg integrate install
```

> QCustomPlot은 vcpkg 대신 직접 파일로 포함 (아래 Step 4-4 참고)

**4-4. QCustomPlot 직접 추가** (vcpkg 미사용):

1. https://www.qcustomplot.com/index.php/download 에서 **QCustomPlot source** 다운로드
2. 압축 해제 후 `qcustomplot.h`, `qcustomplot.cpp` 두 파일을 프로젝트의 `thirdparty/` 폴더에 복사:

```
cpp_version/
└── thirdparty/
    ├── qcustomplot.h
    └── qcustomplot.cpp
```

> CMakeLists.txt가 이미 `thirdparty/` 경로를 참조하도록 설정되어 있음.

---

### Step 5 — 프로젝트 빌드

`cpp_version/` 디렉토리에서 아래 명령 실행:

**명령 프롬프트 (cmd):**

```bat
cmake -B build ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.7.3/msvc2019_64/lib/cmake

cmake --build build --config Release
```

**PowerShell:**

```powershell
cmake -B build `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.7.3/msvc2019_64/lib/cmake

cmake --build build --config Release
```

빌드 성공 시 실행 파일 위치:

```
build/Release/FMCWSimulator.exe
```

---

### 버전 호환성 정리

| 도구 | 권장 버전 | 비고 |
|------|----------|------|
| Visual Studio | **2022** (최신) | 2019도 가능 |
| CMake | **3.20+** | |
| Qt | **6.6 이상** | 6.7.x 권장 |
| QCustomPlot | **2.1.1+** | Qt6 지원 버전 |
| FFTW3 | **3.3.x** | vcpkg 자동 설치 |
