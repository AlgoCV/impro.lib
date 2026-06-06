#ifndef impro_H
#define impro_H

#include "impro_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct impro_kernel impro_kernel;
typedef struct impro_shape impro_shape;
typedef struct impro_ca impro_ca;

/* Processing functions ignore NULL pointers and incompatible dimensions. */

enum {
    IMPRO_MAX_CUSTOM_KERNEL_ROWS = 16,
    IMPRO_MAX_CUSTOM_KERNEL_COLS = 16,
    IMPRO_MAX_CUSTOM_SHAPE_ROWS = 16,
    IMPRO_MAX_CUSTOM_SHAPE_COLS = 16,
    /* Nonlinear custom kernels are smaller than the linear cap because each
     * (rows, cols, op) tuple generates its own SIMD-specialized filter body;
     * 9x9 keeps the static library size and build time bounded while still
     * covering every realistic neighborhood for max/min/median/etc. */
    IMPRO_MAX_CUSTOM_NONLINEAR_ROWS = 9,
    IMPRO_MAX_CUSTOM_NONLINEAR_COLS = 9,
};

/* Reducer selected by `impro_kernel_nonlinear_alloc`. Each value picks the
 * lane-polymorphic reducer applied to the box (all-ones) neighborhood of the
 * custom-dimension nonlinear kernel. `IMPRO_NONLINEAR_MEDIAN` requires the
 * `rows * cols` window to be odd (the comptime sorting-network and Huang
 * sliding-histogram backends both need an odd median rank); even-window
 * MEDIAN requests are rejected with a NULL return. */
typedef enum {
    IMPRO_NONLINEAR_MAX = 0,
    IMPRO_NONLINEAR_MIN = 1,
    IMPRO_NONLINEAR_AVG = 2,
    IMPRO_NONLINEAR_HARM_AVG = 3,
    IMPRO_NONLINEAR_GEOM_AVG = 4,
    IMPRO_NONLINEAR_AND = 5,
    IMPRO_NONLINEAR_OR = 6,
    IMPRO_NONLINEAR_XOR = 7,
    IMPRO_NONLINEAR_MEDIAN = 8,
} impro_nonlinear_op;

Img256 *impro_img256_alloc(uint16_t cols, uint16_t rows);
void impro_img256_free(Img256 *img);
Freq256 *impro_freq256_alloc(uint16_t cols, uint16_t rows);
void impro_freq256_free(Freq256 *freq);
Fft *impro_fft_alloc(uint16_t cols, uint16_t rows);
void impro_fft_free(Fft *work);
Img16 *impro_img16_alloc(uint16_t cols, uint16_t rows);
void impro_img16_free(Img16 *img);
Freq16 *impro_freq16_alloc(uint16_t cols, uint16_t rows);
void impro_freq16_free(Freq16 *freq);

/* Allocate and free packed binary images. */
ImgBw *impro_bw_alloc(uint16_t cols, uint16_t rows);
void impro_bw_free(ImgBw *img);

/* Allocate and free packed 24-bit RGB images. */
ImgRgb *impro_rgb_alloc(uint16_t cols, uint16_t rows);
void impro_rgb_free(ImgRgb *img);

/* Deinterleave an `ImgRgb` into three pre-allocated grayscale planes
 * (red, green, blue). All three destinations must share `src`'s dimensions
 * and must reside in distinct pixel buffers. */
void impro_rgb_split_channels(const ImgRgb *src, Img256 *dst_r, Img256 *dst_g, Img256 *dst_b);

/* Convert grayscale to binary; pixels >= threshold become one bits. */
void impro_img256_threshold(const Img256 *src, ImgBw *dst, uint8_t threshold);
void impro_img256_histogram(const Img256 *src, Hist256 *dst);
void impro_fft_forward(Fft *work, Freq256 *dst, const Img256 *src);
void impro_fft_inverse(Fft *work, Img256 *dst, const Freq256 *src);
void impro_img16_from_gray(const Img256 *src, Img16 *dst);
void impro_img16_to_gray(const Img16 *src, Img256 *dst);
void impro_img16_threshold(const Img16 *src, ImgBw *dst, uint8_t threshold);
void impro_freq16_fft(Freq16 *dst, const Img16 *src);
void impro_img16_inverse_fft(Img16 *dst, const Freq16 *src);

void impro_img256_add_saturate(Img256 *dst, const Img256 *src);
void impro_img256_add_wrap(Img256 *dst, const Img256 *src);
void impro_img256_diff_saturate(Img256 *dst, const Img256 *src);
void impro_img256_diff_wrap(Img256 *dst, const Img256 *src);
void impro_img256_avg(Img256 *dst, const Img256 *src);
void impro_img256_min(Img256 *dst, const Img256 *src);
void impro_img256_max(Img256 *dst, const Img256 *src);
void impro_img256_harm_avg(Img256 *dst, const Img256 *src);
void impro_img256_geom_avg(Img256 *dst, const Img256 *src);
void impro_img256_and(Img256 *dst, const Img256 *src);
void impro_img256_or(Img256 *dst, const Img256 *src);
void impro_img256_xor(Img256 *dst, const Img256 *src);
void impro_img256_not(Img256 *dst);

void impro_img16_add_saturate(Img16 *dst, const Img16 *src);
void impro_img16_add_wrap(Img16 *dst, const Img16 *src);
void impro_img16_diff_saturate(Img16 *dst, const Img16 *src);
void impro_img16_diff_wrap(Img16 *dst, const Img16 *src);
void impro_img16_avg(Img16 *dst, const Img16 *src);
void impro_img16_min(Img16 *dst, const Img16 *src);
void impro_img16_max(Img16 *dst, const Img16 *src);
void impro_img16_harm_avg(Img16 *dst, const Img16 *src);
void impro_img16_geom_avg(Img16 *dst, const Img16 *src);
void impro_img16_and(Img16 *dst, const Img16 *src);
void impro_img16_or(Img16 *dst, const Img16 *src);
void impro_img16_xor(Img16 *dst, const Img16 *src);
void impro_img16_not(Img16 *dst);

/* Histogram-driven intensity remaps. All three are LUT-backed and branch-free
 * in the pixel loop; `hist_scale` stretches [lo,hi] onto 0..255 (no-op when
 * lo>=hi), `hist_mask` keeps [lo,hi] and replaces everything else,
 * `hist_equalize` performs classic CDF histogram equalisation. */
void impro_img256_hist_scale(Img256 *dst, uint8_t lo, uint8_t hi);
void impro_img256_hist_mask(Img256 *dst, uint8_t lo, uint8_t hi, uint8_t replacement);
void impro_img256_hist_equalize(Img256 *dst);

/* Fixed 256-bin grayscale histogram operations. */
Hist256 *impro_hist_alloc(void);
void impro_hist_free(Hist256 *hist);
void impro_hist_clear(Hist256 *hist);
uint32_t impro_hist_size(const Hist256 *hist);
uint32_t impro_hist_bin(const Hist256 *hist, uint8_t bin);
void impro_hist_copy_counts(const Hist256 *hist, uint32_t *dst);
void impro_hist_add_saturate(Hist256 *dst, const Hist256 *src);
void impro_hist_diff_saturate(Hist256 *dst, const Hist256 *src);
void impro_hist_and(Hist256 *dst, const Hist256 *src);
void impro_hist_or(Hist256 *dst, const Hist256 *src);
void impro_hist_xor(Hist256 *dst, const Hist256 *src);
void impro_hist_not(Hist256 *dst);
impro_ca *impro_ca_alloc(uint16_t width, const int16_t *weights, uint32_t denominator);
void impro_ca_free(impro_ca *ca);
void impro_hist_convolve(const impro_ca *ca, const Hist256 *src, Hist256 *dst);

/* Packed binary image operations. */
/* Expand a packed binary image into 8-bit grayscale: one bits -> 255, zero bits -> 0. */
void impro_bw_to_gray(const ImgBw *src, Img256 *dst);
void impro_bw_skeletonize(const ImgBw *src, ImgBw *dst);
void impro_bw_and(ImgBw *dst, const ImgBw *src);
void impro_bw_or(ImgBw *dst, const ImgBw *src);
void impro_bw_xor(ImgBw *dst, const ImgBw *src);
void impro_bw_not(ImgBw *dst);
/* Label foreground components into dst; square_connection != 0 uses 8-neighbor connectivity, otherwise 4-neighbor. */
void impro_bw_segment(const ImgBw *src, Img256 *dst, uint8_t square_connection);
void impro_bw_segment4(const ImgBw *src, Img256 *dst);
void impro_bw_segment8(const ImgBw *src, Img256 *dst);

/* Extracts per-component features from a segmented `seg` (the output of
 * `impro_bw_segment`). `square_connection` selects the adjacency used by
 * the segmentation; the dual connectivity flood is used to count holes.
 *
 * Internally allocates O(cols * rows) scratch via malloc(); on allocation
 * failure `out->count` is set to 0 and the function returns 0. Otherwise
 * returns 1 on success. */
uint8_t impro_bw_measure(const Img256 *seg, uint8_t square_connection, ImproFeatures *out);
void impro_bw_fill_holes(const ImgBw *src, ImgBw *dst, uint8_t square_connection);
void impro_bw_fill_holes4(const ImgBw *src, ImgBw *dst);
void impro_bw_fill_holes8(const ImgBw *src, ImgBw *dst);
void impro_bw_erode(ImgBw *dst, const ImgBw *src, const impro_shape *shape);
void impro_bw_dilate(ImgBw *dst, const ImgBw *src, const impro_shape *shape);

void impro_shape_free(impro_shape *shape);
/* Allocate a custom packed ShapeBw(rows, cols); mask is row-major u32 words. */
impro_shape *impro_shape_alloc(uint8_t rows, uint8_t cols, const uint32_t *mask);

void impro_filter(const impro_kernel *kernel, const Img256 *src, Img256 *dst);
void impro_filter16(const impro_kernel *kernel, const Img16 *src, Img16 *dst);

void impro_img256_resample(const Img256 *src, Img256 *dst, uint8_t strategy);
void impro_img256_resample_kernel(const impro_kernel *kernel, const Img256 *src, Img256 *dst);
void impro_img256_crop(const Img256 *src, Img256 *dst, int32_t off_x, int32_t off_y, uint8_t fill);

void impro_img16_resample(const Img16 *src, Img16 *dst, uint8_t strategy);
void impro_img16_resample_kernel(const impro_kernel *kernel, const Img16 *src, Img16 *dst);
void impro_img16_crop(const Img16 *src, Img16 *dst, int32_t off_x, int32_t off_y, uint8_t fill);

void impro_kernel_free(impro_kernel *kernel);

/* Allocate custom kernel variants backed by unified ZeroKernel/UnitKernel descriptors. */
impro_kernel *impro_kernel_zero_alloc(uint8_t rows, uint8_t cols, const int8_t *weights);
impro_kernel *impro_kernel_unit_alloc(uint8_t rows, uint8_t cols, const uint8_t *weights, uint32_t denominator);

/* Allocate a custom-dimension nonlinear box kernel applying `op` to every
 * `rows x cols` neighborhood (all taps active). Returns NULL on zero/oversized
 * dimensions, on an unknown `op`, or on a `MEDIAN` request with even window. */
impro_kernel *impro_kernel_nonlinear_alloc(uint8_t rows, uint8_t cols, impro_nonlinear_op op);

/* Allocate a custom-dimension nonlinear kernel from a row-major 0/1 shape mask
 * (length `rows * cols`, each byte must be 0 or 1, at least one active tap).
 * Only `IMPRO_NONLINEAR_MAX`/`MIN`/`AND`/`OR`/`XOR` are supported here because
 * their reducers are invariant under per-tap identity masking; the AVG-family
 * reducers depend on the matrix length and MEDIAN short-circuits on zeros, so
 * `AVG`/`HARM_AVG`/`GEOM_AVG`/`MEDIAN` return NULL — use the full-box
 * `impro_kernel_nonlinear_alloc` for those. Returns NULL on zero/oversized
 * dimensions, a NULL or all-zero shape, weights outside {0, 1}, or an
 * unsupported op. */
impro_kernel *impro_kernel_nonlinear_custom_alloc(uint8_t rows, uint8_t cols, const uint8_t *shape, impro_nonlinear_op op);

#ifdef __cplusplus
}
#endif

#endif
