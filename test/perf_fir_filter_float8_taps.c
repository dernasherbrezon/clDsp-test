#include <time.h>
#include <stdio.h>
#include "../src/fir_filter_float8.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    size_t taps_len = 2429;
    size_t input_len = 8340;
    float complex *input = malloc(sizeof(float complex) * input_len);
    if (input == NULL) {
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < input_len; i++) {
        // don't care about the loss of data
        int8_t cur_index = (int8_t) (i * 2);
        input[i] = cur_index / 128.0F + I * ((cur_index + 1) / 128.0F);
    }

    for (size_t j = 100; j < taps_len; j += 100) {
        FILE *fp = fopen("taps.cf32", "rb");
        float complex *taps = malloc(sizeof(float complex) * taps_len);
        fread(taps, sizeof(float complex), taps_len, fp);
        fclose(fp);

        fir_filter_float8 *filter = NULL;
        int code = fir_filter_float8_create(2016000 / 48000, taps, j, input_len, &filter);
        if (code != 0) {
            return EXIT_FAILURE;
        }

        float complex *output = NULL;
        size_t output_len = 0;
        int total_executions = 5;
        clock_t begin = clock();
        for (int i = 0; i < total_executions; i++) {
            fir_filter_float8_process(input, input_len, &output, &output_len, filter);
        }
        clock_t end = clock();
        double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;

        printf("%f,%zu\n", time_spent / total_executions, j);
        fir_filter_float8_destroy(filter);
    }

    free(input);
    return EXIT_SUCCESS;
}
