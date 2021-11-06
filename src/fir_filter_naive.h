#ifndef CLDSP_fir_filter_naive_H
#define CLDSP_fir_filter_naive_H

#include <stdlib.h>
#include <stdint.h>
#include <complex.h>

typedef struct fir_filter_naive_t fir_filter_naive;

int fir_filter_naive_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter_naive **filter);

int fir_filter_naive_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter_naive *filter);

void fir_filter_naive_destroy(fir_filter_naive *filter);

#endif //CLDSP_fir_filter_naive_H
