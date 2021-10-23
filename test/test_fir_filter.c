#include "../src/fir_filter.h"
#include <stdlib.h>
#include <stdio.h>
#include <check.h>

fir_filter *filter = NULL;
float complex *input = NULL;

START_TEST (test_normal)
    {
        size_t taps_len = 3;
        float complex *taps = malloc(sizeof(float complex) * taps_len);
        taps[0] = 1.0f + 2.0F * I;
        taps[1] = 3.0f + 4.0F * I;
        taps[2] = 1.0f + 2.0F * I;
        int code = fir_filter_create(2, taps, taps_len, 72, &filter);

        size_t input_len = 2 * 12 * 3;
        input = malloc(sizeof(float complex) * input_len);
        float *input_as_float = (float *) input;
        for (int i = 0; i < input_len * 2; i++) {
            input_as_float[i] = (float) i;
        }
        float complex *output = NULL;
        size_t output_len = 0;
        fir_filter_process(input, input_len, &output, &output_len, filter);
        for (int i = 0; i < output_len; i++) {
            printf("%.9fF, %.9fF, ", crealf(output[i]), cimagf(output[i]));
        }
        printf("\n");
    }
END_TEST

void teardown() {
    if (filter != NULL) {
        fir_filter_destroy(filter);
        filter = NULL;
    }
    if (input != NULL) {
        free(input);
        input = NULL;
    }
}

void setup() {
    //do nothing
}

Suite *common_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("fir_filter");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_normal);

    tcase_add_checked_fixture(tc_core, setup, teardown);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = common_suite();
    sr = srunner_create(s);

    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}