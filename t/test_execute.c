#include <stdlib.h> /* EXIT_SUCCESS and EXIT_FAILURE */
#include <stdio.h>
#include <check.h>

#include "../src/execute.h"

START_TEST (test_linkage){
    fail_unless( execute_test() == 1 );
}
END_TEST

Suite *
execute_suite(void){
    Suite *s = suite_create("execute");

    TCase *tc_execute = tcase_create("newline");

    tcase_add_test(tc_execute, test_linkage);

    suite_add_tcase(s, tc_execute);

    return s;

}

int main(void){
    int number_failed = 0;
    Suite *s = execute_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
