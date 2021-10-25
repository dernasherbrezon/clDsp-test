#include <time.h>
#include <stdio.h>
#include "../src/fir_filter.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {

    FILE *fp = fopen("taps.cf32", "rb");
    size_t taps_len = 2429;
    float complex *taps = malloc(sizeof(float complex) * taps_len);
    fread(taps, sizeof(float complex), taps_len, fp);
    fclose(fp);

    size_t input_len = 200000 / 2;

    fir_filter *filter = NULL;
    int code = fir_filter_create(2016000 / 48000, taps, taps_len, input_len, &filter);
    if (code != 0) {
        return EXIT_FAILURE;
    }
    float complex *input = malloc(sizeof(float complex) * input_len);
    if (input == NULL) {
        return EXIT_FAILURE;
    }
    for (size_t i = 0; i < input_len; i++) {
        // don't care about the loss of data
        input[i] = i * 2 / 128.0F + I * ((i * 2 + 1) / 128.0F);
    }
    double totalTime = 0.0;
    int total = 10;
    for (int j = 0; j < total; j++) {
        int total_executions = 10;
        clock_t begin = clock();
        for (int i = 0; i < total_executions; i++) {
            float complex *output = NULL;
            size_t output_len = 0;
            fir_filter_process(input, input_len, &output, &output_len, filter);
//            for (int i = 0; i < 20; i++) {
//                printf("%.9f, %.9f ", crealf(output[i]), cimagf(output[i]));
//            }
//            printf("\n");
        }
        clock_t end = clock();
        double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
        totalTime += time_spent;
    }

    printf("average time: %f\n", totalTime / total);
    fir_filter_destroy(filter);

    // Raspberrypi 3
    // average time: 0.099887

    return EXIT_SUCCESS;
}

