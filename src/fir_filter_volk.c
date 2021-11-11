#include "fir_filter_volk.h"
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <stdio.h>
#include <math.h>
#include <volk/volk.h>

struct fir_filter_volk_t {
    uint8_t decimation;
    size_t alignment;

    float complex **taps;
    size_t aligned_taps_len;
    size_t taps_len;
    float complex *original_taps;

    float complex *working_buffer;
    size_t history_offset;
    size_t working_len_total;
    size_t max_input_buffer_length;

    float complex *output;
    float complex *volk_output;
    size_t output_len;

};

int create_aligned_taps(const float complex *original_taps, size_t taps_len, fir_filter_volk *filter) {
    size_t number_of_aligned = fmax((size_t) 1, (float) filter->alignment / sizeof(float));
    // Make a set of taps at all possible alignments
    float complex **result = malloc(number_of_aligned * sizeof(float complex *));
    if (result == NULL) {
        return -1;
    }
    for (int i = 0; i < number_of_aligned; i++) {
        size_t aligned_taps_len = taps_len + number_of_aligned - 1;
        result[i] = (float complex *) volk_malloc(aligned_taps_len * sizeof(float complex), filter->alignment);
        // some taps will be longer than original, but
        // since they contain zeros, multiplication on an input will produce 0
        // there is a tradeoff: multiply unaligned input or
        // multiply aligned input but with additional zeros
        for (size_t j = 0; j < aligned_taps_len; j++) {
            result[i][j] = 0;
        }
        for (size_t j = 0; j < taps_len; j++) {
            //reverse original taps
            result[i][i + j] = original_taps[taps_len - j - 1];
        }
    }
    filter->taps = result;
    filter->aligned_taps_len = number_of_aligned;
    return 0;
}

int fir_filter_volk_create(uint8_t decimation, float complex *taps, size_t taps_len, size_t max_input_buffer_length, fir_filter_volk **filter) {
    struct fir_filter_volk_t *result = malloc(sizeof(struct fir_filter_volk_t));
    if (result == NULL) {
        return -ENOMEM;
    }
    // init all fields with 0 so that destroy_* method would work
    *result = (struct fir_filter_volk_t) {0};

    result->decimation = decimation;
    result->taps_len = taps_len;
    result->original_taps = taps;
    result->history_offset = taps_len - 1;
    result->max_input_buffer_length = max_input_buffer_length;
    result->alignment = volk_get_alignment();
    int code = create_aligned_taps(taps, taps_len, result);
    if (code != 0) {
        fir_filter_volk_destroy(result);
        return code;
    }
    size_t max_input_with_history = max_input_buffer_length + result->history_offset;

    result->working_len_total = max_input_buffer_length + result->history_offset;
    result->working_buffer = volk_malloc(sizeof(float complex) * result->working_len_total, result->alignment);
    if (result->working_buffer == NULL) {
        fir_filter_volk_destroy(result);
        return -ENOMEM;
    }
    memset(result->working_buffer, 0, result->working_len_total * sizeof(float complex));

    // +1 for case when round-up needed.
    result->output_len = max_input_buffer_length / decimation + 1;
    result->output = malloc(sizeof(float complex) * max_input_buffer_length);
    if (result->output == NULL) {
        fir_filter_volk_destroy(result);
        return -ENOMEM;
    }

    result->volk_output = volk_malloc(1 * sizeof(float complex), result->alignment);
    if (result->volk_output == NULL) {
        fir_filter_volk_destroy(result);
        return -ENOMEM;
    }

    *filter = result;
    return 0;
}

int fir_filter_volk_process(const float complex *input, size_t input_len, float complex **output, size_t *output_len, fir_filter_volk *filter) {
    if (input_len > filter->max_input_buffer_length) {
        fprintf(stderr, "<3>requested buffer %zu is more than max: %zu\n", input_len, filter->max_input_buffer_length);
        *output = NULL;
        *output_len = 0;
        return -1;
    }
    if (input_len == 0) {
        *output = NULL;
        *output_len = 0;
        return 0;
    }

    memcpy(filter->working_buffer + filter->history_offset, input, sizeof(float complex) * input_len);
    size_t working_len = filter->history_offset + input_len;

    if (working_len < filter->taps_len) {
        filter->history_offset = working_len;
        *output = NULL;
        *output_len = 0;
        return 0;
    }

    size_t i = 0;
    size_t produced = 0;
    float complex *output_pointer = (float complex *) filter->output;
    size_t max_index = working_len - (filter->taps_len - 1);
    for (; i < max_index; i += filter->decimation, produced++) {
        volk_32fc_x2_dot_prod_32fc_u(filter->volk_output, filter->working_buffer + i, filter->original_taps,
                                      filter->taps_len);
        *output_pointer = *filter->volk_output;
        output_pointer++;
    }
    filter->history_offset = filter->taps_len - 1 - (i - input_len);
    memmove(filter->working_buffer, filter->working_buffer + (working_len - filter->history_offset), sizeof(float complex) * filter->history_offset);

    *output = filter->output;
    *output_len = produced;
    return 0;
}

void fir_filter_volk_destroy(fir_filter_volk *filter) {
    if (filter == NULL) {
        return;
    }
    if (filter->taps != NULL) {
        free(filter->taps);
    }
    if (filter->output != NULL) {
        free(filter->output);
    }
    if (filter->working_buffer != NULL) {
        free(filter->working_buffer);
    }
    if (filter->volk_output != NULL) {
        volk_free(filter->volk_output);
    }
    free(filter);
}
