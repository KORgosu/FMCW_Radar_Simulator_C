# FMCW Radar Simulator — C++ / Qt6 Edition

> **세종대학교 국방레이더기술연구실**
> made by KORgosu (Su Hwan Park)

77 GHz mmWave FMCW 레이더의 전체 신호처리 파이프라인을 실시간으로 시뮬레이션하고 시각화하는 인터랙티브 GUI 애플리케이션입니다.

---

## 주요 기능

| 탭 | 내용 |
|----|------|
| **[1] 신호 원리** | TX 처프 신호, Beat 신호 파형, 순시 주파수, 거리 프로파일 |
| **[2] Range-Doppler** | 2D RDM(Range-Doppler Map) 컬러맵, 거리/속도 축 |
| **[3] CFAR 탐지** | 2D CA-CFAR 탐지 맵, 임계값 시각화 |
| **[4] 통합 파이프라인** | Beat Matrix, RDM, CFAR Map, Point Cloud, EKF 트랙 통합 뷰 |
| **[5] 파이프라인 인스펙터** | 각 처리 Stage별 INPUT → 수식/설명 → OUTPUT 3단 실시간 시각화 |
| **[6] 아키텍처 뷰어** | FMCW 레이더 블록 다이어그램 인터랙티브 HTML 뷰 (QWebEngineView) |

### 신호처리 파이프라인 (7 Stages)

```
[Beat 생성] → [Range FFT] → [Doppler FFT] → [AoA FFT]
     ↓               ↓              ↓              ↓
  Beat Cube     Range Profile      RDM         Det. Points
                                    ↓
                               [CFAR 2D]
                                    ↓
                            [DBSCAN 군집화]
                                    ↓
                             [EKF 추적기]
                                    ↓
                              ObjectInfo[]
```

### Stage 상세

| Stage | 처리 내용 | 입력 → 출력 |
|-------|-----------|------------|
| **Stage 0** RF Frontend | TX 처프 생성, Beat 신호 믹싱, MIMO 가상 배열 합성 | `Params+Targets` → `[Nc×Ns×Nv]` |
| **Stage 1** Range FFT | Hanning 윈도우 + FFT (fast-time 방향) | `[Nc×Ns]` → `[Nc×Nr]` |
| **Stage 2** Doppler FFT | Hanning 윈도우 + FFTshift (slow-time 방향) | `[Nc×Nr]` → `[Nd×Nr]` RDM |
| **Stage 3** AoA FFT | 가상 배열 공간 FFT → 도달각 추정 | `[Nd×Nr×Nv]` → `DetectionPoints[]` |
| **Stage 4** CFAR | 2D CA-CFAR (Summed Area Table, O(1)) | `[Nd×Nr]` power → `bool[Nd×Nr]` |
| **Stage 5** Clustering | DBSCAN (정규화 3D 거리) | `DetectionPoints[]` → `Clusters[]` |
| **Stage 6** EKF | Hungarian 매칭 + 확장 칼만 필터 | `Clusters[]` → `ObjectInfo[]` |

---

## 빌드 환경

| 항목 | 버전 |
|------|------|
| OS | Windows 10/11 (64-bit) |
| IDE | Visual Studio 2022 (v17+) |
| C++ 표준 | C++17 |
| Qt | Qt 6.11.0+ (`msvc2022_64`) |
| 패키지 관리 | vcpkg |
| 빌드 시스템 | CMake 3.20+ / Ninja |

### 의존성

- **Qt6** – Widgets, Concurrent, PrintSupport, WebEngineWidgets
- **FFTW3** – 고속 푸리에 변환 (vcpkg: `fftw3`)
- **QCustomPlot** – 2D 플롯 라이브러리 (소스 직접 포함, `thirdparty/`)

> **Qt WebEngineWidgets 사용 시 추가 설치 필요 (Qt Maintenance Tool)**
>
> `Qt WebEngineWidgets`는 아래 모듈에 의존합니다. Qt Maintenance Tool →
> `Qt 6.11.0` → `Additional Libraries`에서 모두 체크 후 설치하세요.
>
> | 모듈 | 용도 |
> |------|------|
> | **Qt WebChannel** | WebEngine ↔ C++ 브릿지 (필수 의존성) |
> | **Qt Positioning** | WebEngineCore 필수 의존성 |
> | **Qt WebSockets** | WebEngine 권장 의존성 |
> | **Qt WebView** | WebEngine 권장 의존성 |

---

## 빌드 방법

### 1. Qt6 설치

[https://www.qt.io/download](https://www.qt.io/download) 에서 Qt 6.11.0+ **MSVC 2022 64-bit** 컴포넌트를 설치합니다.

설치 시 다음 항목을 반드시 포함하세요:
- `Qt 6.x.x` → `MSVC 2022 64-bit`
- `Qt 6.x.x` → `Additional Libraries` → **Qt WebChannel**
- `Qt 6.x.x` → `Additional Libraries` → **Qt Positioning**
- `Qt 6.x.x` → `Additional Libraries` → **Qt WebSockets**

### 2. vcpkg로 FFTW3 설치

```bash
cd C:/vcpkg
vcpkg install fftw3:x64-windows
```

### 3. CMakePresets.json Qt 경로 확인

`CMakePresets.json`에서 Qt 설치 경로를 확인합니다:

```json
"CMAKE_PREFIX_PATH": "C:/Qt/6.x.x/msvc2022_64"
```

### 4. Visual Studio에서 열기

1. Visual Studio → **폴더 열기(Open Folder)** → `cpp_version` 선택
2. CMake 자동 구성 완료 확인
3. **빌드 → 모두 빌드** (`Ctrl+Shift+B`)
4. **▶ FMCWSimulator** 실행

---

## 프로젝트 구조

```
cpp_version/
├── CMakeLists.txt              # 빌드 정의
├── CMakePresets.json           # VS CMake 프리셋 (Qt 경로 등)
├── vcpkg.json                  # 패키지 의존성
├── main.cpp                    # 진입점
├── resources/
│   ├── resources.qrc
│   ├── images/
│   │   └── sejong_logo.png     # 세종대학교 로고 (타이틀바 표시)
│   └── style/
│       └── dark_radar.qss      # 다크 레이더 테마
├── src/
│   ├── core/
│   │   ├── simulator.cpp/.h    # 시뮬레이션 오케스트레이터
│   │   └── sim_result.h        # 파이프라인 출력 구조체
│   ├── dsp/                    # Pure C DSP 코어
│   │   ├── radar_types.c/.h    # 파라미터 구조체, 파생값 계산
│   │   ├── chirp_gen.c/.h      # TX 처프 생성
│   │   ├── beat_gen.c/.h       # Beat 신호 생성 (MIMO)
│   │   ├── range_fft.c/.h      # Range FFT
│   │   ├── doppler_fft.c/.h    # Doppler FFT → RDM
│   │   ├── aoa_fft.c/.h        # AoA FFT (각도 추정)
│   │   ├── cfar.c/.h           # 2D CA-CFAR
│   │   ├── clustering.c/.h     # DBSCAN 군집화
│   │   ├── data_association.c/.h # Hungarian 매칭
│   │   └── ekf_tracker.c/.h    # Extended Kalman Filter
│   └── ui/                     # Qt6 GUI
│       ├── main_window.cpp/.h  # 메인 윈도우
│       ├── param_panel.cpp/.h  # 레이더 파라미터 패널
│       ├── scene_widget.cpp/.h # 타겟 배치 씬 위젯 (실시간 이동 애니메이션)
│       ├── plot_widget.cpp/.h  # 공용 플롯 베이스
│       ├── pipeline_inspector.cpp/.h  # 파이프라인 인스펙터 (Tab 5)
│       └── tabs/
│           ├── tab1_signal.cpp/.h     # [1] 신호 원리
│           ├── tab2_rdm.cpp/.h        # [2] Range-Doppler
│           ├── tab3_cfar.cpp/.h       # [3] CFAR 탐지
│           └── tab4_pipeline.cpp/.h   # [4] 통합 파이프라인
└── thirdparty/
    ├── qcustomplot.cpp
    └── qcustomplot.h
```

---

## 조작 방법

### Scene 위젯 (타겟 배치)

| 조작 | 동작 |
|------|------|
| **좌클릭 (빈 공간)** | 타겟 추가 |
| **좌클릭 + 드래그** | 타겟 이동 |
| **우클릭** | 타겟 삭제 |
| **타겟 클릭 후 하단 패널** | Velocity / RCS 수치 조정 |
| **✕ Clear All** | 모든 타겟 삭제 |

### 실시간 표적 이동 애니메이션

Scene 위젯은 **20 FPS** 타이머로 실제 경과 시간 기반의 표적 이동을 시뮬레이션합니다.

| 속도 | 동작 | 화살표 색 |
|------|------|-----------|
| **+ (양수)** | 레이더 방향으로 접근 (거리 감소) | 🔴 빨강 |
| **- (음수)** | 레이더에서 멀어짐 (거리 증가) | 🔵 파랑 |
| **0** | 정지 (이동 없음) | 없음 |

**자동 소멸 조건:**
- `range ≤ 0 m` — 레이더 위치 도달 시 소멸
- `range ≥ 200 m` — 최대 탐지거리 이탈 시 소멸
- 드래그 중에는 자동 이동 일시 정지

### 멀티프레임 컨트롤

| 버튼 | 동작 |
|------|------|
| **▶ 재생** | 5 FPS로 연속 프레임 시뮬레이션 (EKF 추적 확인) |
| **■ 정지** | 재생 중지 |
| **▶\| 한 프레임** | 단계별 프레임 진행 |

---

## 레이더 파라미터 기본값

| 파라미터 | 값 | 설명 |
|----------|----|------|
| 반송파 주파수 fc | 77 GHz | mmWave 대역 |
| 대역폭 B | 150 MHz | 거리 분해능 결정 |
| 처프 지속시간 Tc | 40 µs | |
| PRF | 1000 Hz | 처프 반복 주파수 |
| ADC 샘플링 fs | 15 MHz | |
| 처프 수 Nc | 64 | 도플러 FFT 크기 |
| TX 안테나 | 2 | MIMO |
| RX 안테나 | 4 | MIMO |
| 거리 분해능 ΔR | ~1.0 m | c/(2B) |
| 최대 탐지 거리 | ~75 m | |
| 최대 탐지 속도 | ±~20 m/s | |

---

## 아키텍처

```
┌─────────────────────────────────────────────────────┐
│  Qt GUI Thread                                      │
│  MainWindow → ParamPanel / SceneWidget              │
│       ↓ (debounce 80ms)                             │
│  QtConcurrent::run()  ──→  FMCWSimulator::compute() │
│       ↓ (QFutureWatcher)                            │
│  applyResult() → Tab1~5 updatePlots()               │
└─────────────────────────────────────────────────────┘
         ↓
┌─────────────────────────────────────────────────────┐
│  DSP Core (Pure C)                                  │
│  chirp_gen → beat_gen → range_fft → doppler_fft     │
│           → aoa_fft → cfar_2d → dbscan → ekf_step  │
└─────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────┐
│  Scene Animation (독립 타이머, 20 FPS)               │
│  QElapsedTimer 기반 dt 계산                          │
│  → 표적 위치 갱신 → targetsChanged 신호 → 재계산    │
└─────────────────────────────────────────────────────┘
```

- GUI 스레드와 DSP 연산을 `QtConcurrent`로 분리하여 UI 블로킹 없음
- DSP 코어는 Pure C로 작성되어 이식성 확보
- 파라미터 변경 시 80ms 디바운스 후 자동 재계산
- Scene 애니메이션은 DSP 계산과 독립적으로 동작

---

## 라이선스

본 프로젝트는 학술/교육 목적으로 작성되었습니다.
QCustomPlot은 [GPLv3](https://www.qcustomplot.com/) 라이선스를 따릅니다.
