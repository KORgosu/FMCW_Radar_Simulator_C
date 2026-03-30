cmake_minimum_required(VERSION 3.20)
project(FMCWSimulator LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Qt6 ──────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent PrintSupport WebEngineWidgets)

# ── FFTW3 ────────────────────────────────────────
find_package(FFTW3 CONFIG REQUIRED)

# ── QCustomPlot (직접 소스 포함) ──────────────────
# qcustomplot.h / qcustomplot.cpp 를 thirdparty/ 에 복사 후 빌드
set(QCP_SOURCES
    thirdparty/qcustomplot.cpp
)

# ── DSP 코어 (Pure C) ────────────────────────────
set(DSP_SOURCES
    src/dsp/radar_types.c
    src/dsp/chirp_gen.c
    src/dsp/beat_gen.c
    src/dsp/range_fft.c
    src/dsp/doppler_fft.c
    src/dsp/cfar.c
    src/dsp/aoa_fft.c
    src/dsp/clustering.c
    src/dsp/data_association.c
    src/dsp/ekf_tracker.c
)

# ── C++ Core + GUI ───────────────────────────────
set(CPP_SOURCES
    src/core/simulator.cpp
    src/ui/main_window.cpp
    src/ui/param_panel.cpp
    src/ui/scene_widget.cpp
    src/ui/plot_widget.cpp
    src/ui/pipeline_inspector.cpp
    src/ui/architecture_window.cpp
    src/ui/tabs/tab1_signal.cpp
    src/ui/tabs/tab2_rdm.cpp
    src/ui/tabs/tab3_cfar.cpp
    src/ui/tabs/tab4_pipeline.cpp
    main.cpp
)

qt_add_resources(QRC_SOURCES resources/resources.qrc)

add_executable(${PROJECT_NAME}
    ${DSP_SOURCES}
    ${QCP_SOURCES}
    ${CPP_SOURCES}
    ${QRC_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty   # qcustomplot.h 경로
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::PrintSupport
    Qt6::WebEngineWidgets
    FFTW3::fftw3
)

# ── Windows: DLL 자동 복사 + windeployqt ────────
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT}
                $<IF:$<CONFIG:Debug>,--debug,--release>
                --no-translations
                $<TARGET_FILE:${PROJECT_NAME}>
        )
    endif()
endif()
