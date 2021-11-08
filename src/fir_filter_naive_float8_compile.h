#ifndef CLDSP_fir_filter_naive_float8_compile_H
#define CLDSP_fir_filter_naive_float8_compile_H

#include <stdlib.h>
#include <stdint.h>
#include <complex.h>

typedef struct fir_filter_naive_float8_compile_t fir_filter_naive_float8_compile;

int fir_filter_naive_float8_compile_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter_naive_float8_compile **filter);

int fir_filter_naive_float8_compile_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter_naive_float8_compile *filter);

void fir_filter_naive_float8_compile_destroy(fir_filter_naive_float8_compile *filter);

#endif //CLDSP_fir_filter_naive_float8_compile_H
