#include "simulator.h"
#include "../dsp/chirp_gen.h"
#include "../dsp/beat_gen.h"
#include "../dsp/range_fft.h"
#include "../dsp/doppler_fft.h"
#include "../dsp/cfar.h"
#include "../dsp/aoa_fft.h"
#include "../dsp/clustering.h"
#include "../dsp/data_association.h"
#include "../dsp/ekf_tracker.h"
#include <cstring>
#include <cstdlib>
#include <ctime>

FMCWSimulator::FMCWSimulator(QObject *parent)
    : QObject(parent)
{
    std::srand((unsigned)std::time(nullptr));
    ekf_init(&m_tracker);
}

FMCWSimulator::~FMCWSimulator() {}

void FMCWSimulator::resetTracker()
{
    ekf_init(&m_tracker);
}

SimResult FMCWSimulator::compute(const std::vector<Target> &targets,
                                  const RadarParams &params)
{
    return runPipeline(targets, params, true);
}

SimResult FMCWSimulator::step(const std::vector<Target> &targets,
                               const RadarParams &params)
{
    return runPipeline(targets, params, false);
}

SimResult FMCWSimulator::runPipeline(const std::vector<Target> &targets,
                                      const RadarParams &params,
                                      bool reset_ekf)
{
    if (reset_ekf) ekf_init(&m_tracker);

    SimResult res;
    RadarParamsDerived d = params_derive(&params);

    int Ns    = d.n_sample;
    int Nc    = params.n_chirp;
    int Nv    = d.n_virtual;
    int Nhalf = params.n_range_fft / 2;
    int Nd    = params.n_doppler_fft;
    int Na    = params.n_angle_fft;

    /* ── 축 생성 ── */
    double *r_axis = range_axis_alloc(&params, &d);
    double *v_axis = velocity_axis_alloc(&params, &d);
    double *a_axis = angle_axis_alloc(&params, &d);

    res.range_axis.assign(r_axis, r_axis + Nhalf);
    res.velocity_axis.assign(v_axis, v_axis + Nd);
    res.angle_axis.assign(a_axis, a_axis + Na);
    free(r_axis); free(v_axis); free(a_axis);

    /* ── Stage 0: TX chirp ── */
    res.t_fast.resize(Ns);
    res.tx_chirp_re.resize(Ns);
    res.tx_chirp_im.resize(Ns);
    res.tx_freq.resize(Ns);

    fftw_complex *tx_chirp = (fftw_complex *)fftw_malloc(Ns * sizeof(fftw_complex));
    chirp_gen(&params, &d, tx_chirp, res.tx_freq.data(), Ns);
    double dt = params.chirp_dur_s / (Ns - 1);
    for (int n = 0; n < Ns; n++) {
        res.t_fast[n]      = n * dt;
        res.tx_chirp_re[n] = tx_chirp[n][0];
        res.tx_chirp_im[n] = tx_chirp[n][1];
    }
    fftw_free(tx_chirp);

    /* ── Stage 0: Beat cube ── */
    long long cube_sz = (long long)Nc * Ns * Nv;
    fftw_complex *beat_cube = (fftw_complex *)fftw_malloc(cube_sz * sizeof(fftw_complex));

    if (!targets.empty())
        beat_gen(targets.data(), (int)targets.size(), &params, &d, beat_cube);
    else
        std::memset(beat_cube, 0, cube_sz * sizeof(fftw_complex));

    /* beat_matrix (channel 0) → beat_one, beat_mat */
    res.beat_rows = Nc; res.beat_cols = Ns;
    res.beat_mat_re.resize((long long)Nc * Ns);
    res.beat_mat_im.resize((long long)Nc * Ns);
    for (int nc = 0; nc < Nc; nc++)
        for (int ns = 0; ns < Ns; ns++) {
            long long src = ((long long)nc * Ns + ns) * Nv;
            res.beat_mat_re[nc * Ns + ns] = beat_cube[src][0];
            res.beat_mat_im[nc * Ns + ns] = beat_cube[src][1];
        }
    res.beat_one_re.assign(res.beat_mat_re.begin(), res.beat_mat_re.begin() + Ns);
    res.beat_one_im.assign(res.beat_mat_im.begin(), res.beat_mat_im.begin() + Ns);

    /* beat_matrix as fftw_complex (channel 0) */
    fftw_complex *beat_mat = (fftw_complex *)fftw_malloc((long long)Nc * Ns * sizeof(fftw_complex));
    for (int nc = 0; nc < Nc; nc++)
        for (int ns = 0; ns < Ns; ns++) {
            long long src = ((long long)nc * Ns + ns) * Nv;
            long long dst = (long long)nc * Ns + ns;
            beat_mat[dst][0] = beat_cube[src][0];
            beat_mat[dst][1] = beat_cube[src][1];
        }

    /* ── Stage 1: Range-FFT ── */
    fftw_complex *rng_fft = (fftw_complex *)fftw_malloc((long long)Nc * Nhalf * sizeof(fftw_complex));
    res.range_profile_db.resize(Nhalf);
    range_fft(beat_mat, &params, &d, rng_fft, res.range_profile_db.data());

    /* ── Stage 2: Doppler-FFT ── */
    fftw_complex *rdm = (fftw_complex *)fftw_malloc((long long)Nd * Nhalf * sizeof(fftw_complex));
    res.rdm_db.resize((long long)Nd * Nhalf);
    res.rdm_pow.resize((long long)Nd * Nhalf);
    res.rdm_rows = Nd; res.rdm_cols = Nhalf;
    doppler_fft(rng_fft, &params, &d, rdm,
                res.rdm_db.data(), res.rdm_pow.data());
    fftw_free(rdm);

    /* ── Stage 3: CFAR ── */
    res.cfar_threshold.resize((long long)Nd * Nhalf);
    res.cfar_detections.resize((long long)Nd * Nhalf);
    cfar_2d(res.rdm_pow.data(), &params,
            res.cfar_threshold.data(), res.cfar_detections.data());

    /* ── Stage 3: RDM Cube ── */
    fftw_complex *rdm_cube = (fftw_complex *)fftw_malloc((long long)Nd * Nhalf * Nv * sizeof(fftw_complex));
    rdm_cube_calc(beat_cube, &params, &d, rdm_cube);

    /* ── Stage 3: AoA FFT ── */
    int max_pts = Nd * Nhalf;
    res.detection_points.resize(max_pts);
    int n_pts = 0;
    aoa_fft(rdm_cube, res.cfar_detections.data(), res.rdm_db.data(),
            &params, &d,
            res.range_axis.data(), res.velocity_axis.data(), res.angle_axis.data(),
            res.detection_points.data(), &n_pts);
    res.detection_points.resize(n_pts);

    /* ── Stage 5: Clustering ── */
    res.clusters.resize(n_pts > 0 ? n_pts : 1);
    int n_clusters = 0;
    if (n_pts > 0)
        dbscan_cluster(res.detection_points.data(), n_pts,
                       0.1, 2,
                       d.range_max_m, d.velocity_max_mps,
                       res.clusters.data(), &n_clusters);
    res.clusters.resize(n_clusters);

    /* ── Stage 6: EKF ── */
    /* 헝가리안 O(n³) 방지: 클러스터 수를 RADAR_MAX_TRACKS로 제한 */
    int n_clusters_ekf = n_clusters < RADAR_MAX_TRACKS ? n_clusters : RADAR_MAX_TRACKS;
    res.objects.resize(RADAR_MAX_TRACKS);
    int n_obj = 0;
    ekf_step(&m_tracker, res.clusters.data(), n_clusters_ekf,
             &params, &d, res.objects.data(), &n_obj);
    res.objects.resize(n_obj);

    fftw_free(beat_cube);
    fftw_free(beat_mat);
    fftw_free(rng_fft);
    fftw_free(rdm_cube);

    return res;
}
