#ifndef CLDSP_fir_filter_naive_float8_memmap_H
#define CLDSP_fir_filter_naive_float8_memmap_H

#include <stdlib.h>
#include <stdint.h>
#include <complex.h>

typedef struct fir_filter_naive_float8_memmap_t fir_filter_naive_float8_memmap;

int fir_filter_naive_float8_memmap_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter_naive_float8_memmap **filter);

int fir_filter_naive_float8_memmap_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter_naive_float8_memmap *filter);

void fir_filter_naive_float8_memmap_destroy(fir_filter_naive_float8_memmap *filter);

#endif //CLDSP_fir_filter_naive_float8_memmap_H
