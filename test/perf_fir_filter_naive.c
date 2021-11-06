#include <time.h>
#include <stdio.h>
#include "../src/fir_filter_naive.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {

    FILE *fp = fopen("taps.cf32", "rb");
    size_t taps_len = 2429;
    float complex *taps = malloc(sizeof(float complex) * taps_len);
    fread(taps, sizeof(float complex), taps_len, fp);
    fclose(fp);

//    for (int i = 0; i < taps_len; i++) {
//        printf("%.9fF, %.9fF ", crealf(taps[i]), cimagf(taps[i]));
//    }
//    printf("\n");

    size_t input_len = 8340;

    fir_filter_naive *filter = NULL;
    int code = fir_filter_naive_create(2016000 / 48000, taps, taps_len, input_len, &filter);
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
//    for (int i = 0; i < taps_len; i++) {
//        printf("%.9f, %.9f ", crealf(input[i]), cimagf(input[i]));
//    }
//    printf("\n");
    float complex *output = NULL;
    size_t output_len = 0;
    int total_executions = 1;
    clock_t begin = clock();
    for (int i = 0; i < total_executions; i++) {
        fir_filter_naive_process(input, input_len, &output, &output_len, filter);
    }
    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;

    printf("average time: %f\n", time_spent / total_executions);
    for (int i = 0; i < 20; i++) {
        printf("%.9f, %.9f ", crealf(output[i]), cimagf(output[i]));
    }
    printf("\n");
    fir_filter_naive_destroy(filter);

    // Raspberrypi 3
    // average time:

    return EXIT_SUCCESS;
}

