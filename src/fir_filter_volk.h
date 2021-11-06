#ifndef CLDSP_fir_filter_volk_H
#define CLDSP_fir_filter_volk_H

#include <stdlib.h>
#include <stdint.h>
#include <complex.h>

typedef struct fir_filter_volk_t fir_filter_volk;

int fir_filter_volk_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter_volk **filter);

int fir_filter_volk_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter_volk *filter);

void fir_filter_volk_destroy(fir_filter_volk *filter);

#endif //CLDSP_fir_filter_volk_H
