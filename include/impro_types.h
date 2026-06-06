#ifndef impro_types_H
#define impro_types_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Img256 {
    uint16_t cols;
    uint16_t rows;
    size_t size;
    uint8_t *pixels;
} Img256;

/* Packed 4-bit grayscale image: two pixels per byte, low nibble first. */
typedef struct Img16 {
    uint16_t cols;
    uint16_t rows;
    size_t size;
    size_t byte_count;
    uint8_t *pixels;
} Img16;

/* Fixed-point frequency-domain representation of an Img256 FFT. `orig_cols`
 * and `orig_rows` capture the dimensions of the source image that produced
 * this spectrum (zero until the first forward transform). All FFT scratch
 * buffers and twiddle tables live in a separate `Fft` workspace. */
typedef struct Freq256 {
    uint16_t cols;
    uint16_t rows;
    uint16_t orig_cols;
    uint16_t orig_rows;
    size_t size;
    int32_t *real;
    int32_t *imag;
} Freq256;

/* Scratch + precomputed twiddle workspace shared by `Freq256` forward and
 * inverse transforms. Allocate once for a given `cols x rows` via
 * `impro_fft_alloc`; the scratch planes are cleared on entry to every
 * transform so the same workspace may be reused in both directions. */
typedef struct Fft {
    uint16_t cols;
    uint16_t rows;
    size_t size;
    int32_t *scratch_real;
    int32_t *scratch_imag;
    int32_t *tw_cols_r;
    int32_t *tw_cols_i;
    int32_t *tw_rows_r;
    int32_t *tw_rows_i;
} Fft;

/* Fixed-point frequency-domain representation of an Img16 FFT. */
typedef struct Freq16 {
    uint16_t cols;
    uint16_t rows;
    size_t size;
    int32_t *real;
    int32_t *imag;
    int32_t *scratch_real;
    int32_t *scratch_imag;
    int32_t *tw_cols_r;
    int32_t *tw_cols_i;
    int32_t *tw_rows_r;
    int32_t *tw_rows_i;
} Freq16;

/* Packed 24-bit RGB image: three interleaved bytes per pixel (R, G, B). The
 * `pixels` buffer length is `size * 3` bytes; `size` counts pixels, not bytes,
 * so it matches the `cols * rows` size of the grayscale planes produced by
 * `impro_rgb_split_channels`. */
typedef struct ImgRgb {
    uint16_t cols;
    uint16_t rows;
    size_t size;
    uint8_t *pixels;
} ImgRgb;

/* Packed binary image with little-endian bits inside each 32-bit word. */
typedef struct ImgBw {
    uint16_t cols;
    uint16_t rows;
    uint16_t stride;
    uint32_t total_words;
    uint32_t *pixels;
} ImgBw;

/* Per-component descriptor emitted by `impro_bw_measure`. `sum_x` and
 * `sum_y` are raw coordinate sums; the centre of mass is
 * `cx = sum_x / area`, `cy = sum_y / area`. */
typedef struct ImproFeature {
    uint16_t x_min;
    uint16_t y_min;
    uint16_t x_max;
    uint16_t y_max;
    uint32_t area;
    uint32_t perimeter;
    /* Raw coordinate sums (centroid = sum_x/area, sum_y/area). 32-bit: the
     * worst-case sum fits uint32 for the supported max image side. */
    uint32_t sum_x;
    uint32_t sum_y;
    uint16_t holes;
    uint16_t parts;
} ImproFeature;

/* Fixed-capacity result of `impro_bw_measure`. `items[0]` is reserved for
 * background and left zeroed; `items[1..=count]` are the populated
 * features in label order. */
typedef struct ImproFeatures {
    uint8_t count;
    ImproFeature items[256];
} ImproFeatures;

/* Fixed 256-bin histogram for 8-bit grayscale images. */
typedef struct Hist256 {
    uint32_t size;
    uint32_t counts[256];
} Hist256;

typedef enum {
    IMPRO_RESAMPLING_NEAREST = 0,
    IMPRO_RESAMPLING_BILINEAR = 1,
    IMPRO_RESAMPLING_BICUBIC = 2,
} impro_resampling;

#ifdef __cplusplus
}
#endif

#endif
