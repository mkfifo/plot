/* don't included libcheck when I am included as part of test_main.c */
#ifndef PLOT_TEST_MAIN
#define PLOT_TEST_MAIN
#include <check.h>
#include "test.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "../src/value.h"
#include "../src/types.h"
#include "../src/hash.h"
#include "../src/env.h"
#include "../src/funcs.h"

START_TEST (test_funcs_add){
    const plot_value *res;
    plot_expr vals[2];
    plot_env *env;

    vals[0].type = plot_expr_value;
    vals[0].u.value.type = plot_type_number;
    vals[0].u.value.u.number.val = 4;

    vals[1].type = plot_expr_value;
    vals[1].u.value.type = plot_type_number;
    vals[1].u.value.u.number.val = 5;

    puts("\tsetting up env for test_funcs_add");
    env = plot_env_init(0);
    fail_if( 0 == env );

    puts("\ttesting basic addition");
    fail_if( 0 == (res = plot_func_add(env, vals, 2)) );
    fail_unless( res->type == plot_type_number );
    fail_unless( res->u.number.val == 9 );

    plot_env_cleanup(env);
}
END_TEST

START_TEST (test_funcs_env){
    plot_symbol sym;
    plot_value add;
    plot_env *env = plot_env_init(0);

    const plot_value *res;
    plot_expr vals[2];
    const plot_value *f;

    sym.val = "+";
    sym.len = 2;
    sym.size = 2;

    add.type = plot_type_function;
    add.u.function.func = plot_func_add;
    puts("\tdefining function add");
    fail_unless( 1 == plot_env_define(env, &sym, &add) );

    vals[0].type = plot_expr_value;
    vals[0].u.value.type = plot_type_number;
    vals[0].u.value.u.number.val = 4;

    vals[1].type = plot_expr_value;
    vals[1].u.value.type = plot_type_number;
    vals[1].u.value.u.number.val = 5;

    puts("\ttesting fetching function");
    fail_if( 0 == (f = plot_env_get(env, &sym)) );
    fail_unless( f->type == plot_type_function );
    fail_unless( f->u.function.func == plot_func_add );

    puts("\ttesting function call");
    fail_if( 0 == (res = f->u.function.func(env, vals, 2)) );
    fail_unless( res->type == plot_type_number );
    fail_unless( res->u.number.val == 9 );

    plot_env_cleanup(env);
}
END_TEST

START_TEST (test_error_alloc_failed){
    plot_value v;
    puts("\tTesting alloc failed error handling (error expected):");
    v.type = plot_type_error;
    v.u.error.type = plot_error_alloc_failed;
    v.u.error.msg = "testing alloc failed error";
    v.u.error.location = "test_error_alloc_failed";
    plot_handle_error(&v);
}
END_TEST

START_TEST (test_error_bad_args){
    plot_value v;
    puts("\tTesting bad args error handling (error expected):");
    v.type = plot_type_error;
    v.u.error.type = plot_error_bad_args;
    v.u.error.msg = "testing bad args error";
    v.u.error.location = "test_error_bad_args";
    plot_handle_error(&v);
}
END_TEST

START_TEST (test_error_internal){
    plot_value v;
    puts("\tTesting internal error handling (error expected):");

    v.type = plot_type_error;
    v.u.error.type = plot_error_internal;
    v.u.error.msg = "testing internal error";
    v.u.error.location = "test_error_internal";
    plot_handle_error(&v);
}
END_TEST

START_TEST (test_error_unbound_symbol){
    plot_value v;
    puts("\tTesting 'unbound symbol' error handling (error expected):");

    v.type = plot_type_error;
    v.u.error.type = plot_error_unbound_symbol;
    v.u.error.msg = "testing unbound symbol error";
    v.u.error.location = "test_error_unbound_symbol";
    plot_handle_error(&v);
}
END_TEST


START_TEST (test_display){
    plot_expr v, s;
    plot_env *env;
    env = plot_env_init(0);


    v.type = plot_expr_value;
    puts("\n\tTesting display of values");

    puts("\t\ttesting display of number (expected '3')");
    v.u.value.type = plot_type_number;
    v.u.value.u.number.val = 3;
    plot_func_display(env, &v, 1);
    puts(""); /* trailing \n */

    puts("\t\ttesting display of symbol (expected '3')");
    s.type = plot_expr_value;
    s.u.value.type = plot_type_symbol;
#define TEST_DISPLAY_STRING "testing display or function"
    s.u.value.u.symbol.val = TEST_DISPLAY_STRING;
    s.u.value.u.symbol.size = strlen(TEST_DISPLAY_STRING);
    s.u.value.u.symbol.len = strlen(TEST_DISPLAY_STRING);
    fail_unless( 1 == plot_env_define(env, &(s.u.value.u.symbol), &(v.u.value)) );
    plot_func_display(env, &s, 1);
    puts(""); /* trailing \n */

    puts("\t\ttesting display of function");
    v.u.value.type = plot_type_function;
    v.u.value.u.function.env = 0;
    v.u.value.u.function.func = 0;
    plot_func_display(env, &v, 1);

    /* last as this will cause an exit(1) */
    puts("\t\ttesting display of error (error expected)");
    v.u.value.type = plot_type_error;
    v.u.value.u.error.type = plot_error_internal;
    v.u.value.u.error.msg = "testing display of error";
    v.u.value.u.error.location = "test_display";
    plot_func_display(env, &v, 1);
}
END_TEST


TEST_CASE_NEW(funcs)
TEST_CASE_ADD(funcs, funcs_add)
TEST_CASE_ADD(funcs, funcs_env)
    /* display of error will cause exit(1) */
    tcase_add_exit_test(tc_funcs, test_display, 1);
    tcase_add_exit_test(tc_funcs, test_error_alloc_failed, 1);
    tcase_add_exit_test(tc_funcs, test_error_bad_args, 1);
    tcase_add_exit_test(tc_funcs, test_error_internal, 1);
    tcase_add_exit_test(tc_funcs, test_error_unbound_symbol, 1);
TEST_CASE_END(funcs)
