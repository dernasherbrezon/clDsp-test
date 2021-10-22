#include "../src/fir_filter.h"
#include <stdlib.h>
#include <check.h>

fir_filter *filter = NULL;

START_TEST (test_normal)
    {
        size_t taps_len = 3;
        float complex *taps = malloc(sizeof(float complex) * taps_len);
        taps[0] = 1.0f + 2.0F * I;
        taps[1] = 3.0f + 4.0F * I;
        taps[2] = 1.0f + 2.0F * I;
        int code = fir_filter_create(2, taps, taps_len, 10, &filter);

        const float complex input[] = {1.0F, 2.0F, 3.0F, 4.0F, 1.0F, 2.0F};
        size_t input_len = 3;
        float complex *output = NULL;
        size_t output_len = 0;
        fir_filter_process(input, input_len, &output, &output_len, filter);

    }
END_TEST

void teardown() {
    if (filter != NULL) {
        fir_filter_destroy(filter);
        filter = NULL;
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