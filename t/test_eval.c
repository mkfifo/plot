/* don't included libcheck when I am included as part of test_main.c */
#ifndef PLOT_TEST_MAIN
#define PLOT_TEST_MAIN
#include <check.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "../src/value.h"
#include "../src/hash.h"
#include "../src/env.h"
#include "../src/eval.h"
#include "../src/parse.h"
#include "../src/funcs.h"

START_TEST (test_eval_add){
    const plot_value *val;
    plot_env *env = plot_env_init(0);
    plot_expr expr;

    plot_symbol sym;
    plot_value add;

    sym.val = "+";
    sym.len = 2;
    sym.size = 2;

    add.type = plot_type_function;
    add.u.function.func = plot_func_add;
    add.u.function.env = 0;

    /* FIXME TODO need to setup env to know about addition */
    puts("\tdefining function add");
    fail_unless( 1 == plot_env_define(env, &sym, &add) );

#define PLOT_EVAL_SIMPLE "(+ 5 4)"
    char *ch = PLOT_EVAL_SIMPLE;
    int i=0;

    puts("\tparsing expression for eval test");
    fail_if( 0 == plot_parse_expr(&expr, ch, &i) );

    puts("\teval of '" PLOT_EVAL_SIMPLE "'");
    fail_if( 0 == (val = plot_eval(env, &expr)) );
    fail_unless( i == strlen(ch) );
    fail_unless( val->type == plot_type_number );
    fail_unless( val->u.number.val == 9 );

    printf("Expected number '9', got number '%d'\n", val->u.number.val);

    plot_env_cleanup(env);
}
END_TEST

Suite *
eval_suite(void){
    Suite *s = suite_create("suite_eval");

    TCase *tc_eval = tcase_create("test_eval");

    tcase_add_test(tc_eval, test_eval_add);

    suite_add_tcase(s, tc_eval);

    return s;
}

