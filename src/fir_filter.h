#ifndef CLDSP_FIR_FILTER_H
#define CLDSP_FIR_FILTER_H

#include <stdlib.h>
#include <stdint.h>
#include <complex.h>

typedef struct fir_filter_t fir_filter;

int fir_filter_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter **filter);

void fir_filter_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter *filter);

void fir_filter_destroy(fir_filter *filter);

#endif //CLDSP_FIR_FILTER_H
