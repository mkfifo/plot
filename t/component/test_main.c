#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <check.h>

#include "test.h"

/* test_*.c want to include check themselves in order to allow better
 * editor feedback
 * INCLUDED_CHECK prevents them including it
 */
#define PLOT_TEST_MAIN
#include "test_read.c"
#include "test_hash.c"
#include "test_env.c"
#include "test_garbage.c"
#include "test_hashing.c"

int main(void){
    int number_failed = 0;
    Suite *s;
    SRunner *sr;

    puts("\nBegining testing");

    MAIN_ADD_SUITE(read)

    MAIN_ADD_SUITE(hash)

    MAIN_ADD_SUITE(env)

    MAIN_ADD_SUITE(garbage)

    MAIN_ADD_SUITE(hashing)

    puts("\nFinished testing.");
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

