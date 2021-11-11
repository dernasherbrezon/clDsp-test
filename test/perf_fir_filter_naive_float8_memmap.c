#include <sys/time.h>
#include <stdio.h>
#include "../src/fir_filter_naive_float8_memmap.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {

    FILE *fp = fopen("taps.cf32", "rb");
    size_t taps_len = 2429;
    float complex *taps = malloc(sizeof(float complex) * taps_len);
    fread(taps, sizeof(float complex), taps_len, fp);
    fclose(fp);

    size_t input_len = 8340;

    fir_filter_naive_float8_memmap *filter = NULL;
    int code = fir_filter_naive_float8_memmap_create(2016000 / 48000, taps, taps_len, input_len, &filter);
    if (code != 0) {
        return EXIT_FAILURE;
    }
    float complex *input = malloc(sizeof(float complex) * input_len);
    if (input == NULL) {
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < input_len; i++) {
        // don't care about the loss of data
        int8_t cur_index = (int8_t) (i * 2);
        input[i] = cur_index / 128.0F + I * ((cur_index + 1) / 128.0F);
    }
    float complex *output = NULL;
    size_t output_len = 0;
    int total_executions = 1000;
    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (int i = 0; i < total_executions; i++) {
        fir_filter_naive_float8_memmap_process(input, input_len, &output, &output_len, filter);
    }
    gettimeofday(&end, NULL);
    double time_spent = (double) (end.tv_usec - start.tv_usec) / 1000000 + (double) (end.tv_sec - start.tv_sec);

    printf("average time: %f\n", time_spent / total_executions);
    for (int i = 0; i < 20; i++) {
        printf("%.9f, %.9f ", crealf(output[i]), cimagf(output[i]));
    }
    printf("\n");
    fir_filter_naive_float8_memmap_destroy(filter);

    // Raspberrypi 3
    // average time: 0.014420

    return EXIT_SUCCESS;
}

