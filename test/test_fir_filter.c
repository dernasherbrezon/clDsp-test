#include "../src/fir_filter.h"
#include <stdlib.h>
#include <stdio.h>
#include <check.h>

fir_filter *filter = NULL;
float complex *input = NULL;
size_t input_len = 2 * 12 * 3;

float expected_float[] = {-2.000000000F, 1.000000000F, -14.000000000F, 31.000000000F, -26.000000000F, 83.000000000F, -38.000000000F, 135.000000000F, -50.000000000F, 187.000000000F, -62.000000000F, 239.000000000F, -74.000000000F, 291.000000000F, -86.000000000F, 343.000000000F,
                          -98.000000000F, 395.000000000F, -110.000000000F, 447.000000000F, -122.000000000F, 499.000000000F, -134.000000000F, 551.000000000F, -146.000000000F, 603.000000000F, -158.000000000F, 655.000000000F, -170.000000000F, 707.000000000F, -182.000000000F, 759.000000000F,
                          -194.000000000F, 811.000000000F, -206.000000000F, 863.000000000F, -218.000000000F, 915.000000000F, -230.000000000F, 967.000000000F, -242.000000000F, 1019.000000000F, -254.000000000F, 1071.000000000F, -266.000000000F, 1123.000000000F, -278.000000000F,
                          1175.000000000F, -290.000000000F, 1227.000000000F, -302.000000000F, 1279.000000000F, -314.000000000F, 1331.000000000F, -326.000000000F, 1383.000000000F, -338.000000000F, 1435.000000000F, -350.000000000F, 1487.000000000F, -362.000000000F, 1539.000000000F,
                          -374.000000000F, 1591.000000000F, -386.000000000F, 1643.000000000F, -398.000000000F, 1695.000000000F, -410.000000000F, 1747.000000000F, -422.000000000F, 1799.000000000F};
float complex *expected = (float complex *) expected_float;
size_t expected_len = sizeof(expected_float) / sizeof(float) / 2;

static void create_input();

START_TEST(test_edge_cases)
    {
        size_t taps_len = 3;
        float complex *taps = malloc(sizeof(float complex) * taps_len);
        ck_assert(taps != NULL);
        taps[0] = 1.0f + 2.0F * I;
        taps[1] = 3.0f + 4.0F * I;
        taps[2] = 1.0f + 2.0F * I;
        int code = fir_filter_create(2, taps, taps_len, 72, &filter);
        ck_assert_int_eq(code, 0);

        create_input();

        float complex *output = NULL;
        size_t output_len = 0;
        fir_filter_process(input, 0, &output, &output_len, filter);
        ck_assert_int_eq(output_len, 0);

        for (size_t i = 0; i < input_len; i += 2) {
            fir_filter_process(input + i, 2, &output, &output_len, filter);
            ck_assert_int_eq(output_len, 1);
            ck_assert_int_eq((int32_t) crealf(expected[i / 2]) * 10000, (int32_t) crealf(output[0]) * 10000);
        }


    }
END_TEST

START_TEST (test_normal)
    {
        size_t taps_len = 3;
        float complex *taps = malloc(sizeof(float complex) * taps_len);
        ck_assert(taps != NULL);
        taps[0] = 1.0f + 2.0F * I;
        taps[1] = 3.0f + 4.0F * I;
        taps[2] = 1.0f + 2.0F * I;
        int code = fir_filter_create(2, taps, taps_len, 72, &filter);
        ck_assert_int_eq(code, 0);

        create_input();
        float complex *output = NULL;
        size_t output_len = 0;
        fir_filter_process(input, input_len, &output, &output_len, filter);

        ck_assert_int_eq(output_len, expected_len);
        for (size_t i = 0; i < expected_len; i++) {
            ck_assert_int_eq((int32_t) crealf(expected[i]) * 10000, (int32_t) crealf(output[i]) * 10000);
        }
    }
END_TEST

static void create_input() {
    input = malloc(sizeof(float complex) * input_len);
    ck_assert(input != NULL);
    float *input_as_float = (float *) input;
    for (int i = 0; i < input_len * 2; i++) {
        input_as_float[i] = (float) i;
    }
}

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

//    tcase_add_test(tc_core, test_normal);
    tcase_add_test(tc_core, test_edge_cases);

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