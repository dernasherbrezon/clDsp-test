#ifndef CLDSP_FIR_FILTER_H
#define CLDSP_FIR_FILTER_H

#include <stdlib.h>
#include <stdint.h>
#include <complex.h>

typedef struct fir_filter_memmapt fir_filter_memmap;

int fir_filter_memmap_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter_memmap **filter);

int fir_filter_memmap_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter_memmap *filter);

void fir_filter_memmap_destroy(fir_filter_memmap *filter);

#endif //CLDSP_FIR_FILTER_H
