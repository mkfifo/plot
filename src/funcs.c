#include <stdlib.h>
#include <stdio.h>

#include "value.h"
#include "types.h"
#include "hash.h"
#include "env.h"
#include "funcs.h"
#include "eval.h"

#define DEBUG 0

/* takes a list of expressions
 * evals each and then adds the expression's value if it is a number
 * if any of the expressions evaluate to something other than an error
 * throw plot_error_bad_args
 */
const plot_value * plot_func_add(plot_env *env, const plot_expr *args, int argc){
    plot_value *res;
    const plot_value *tmp;
    const plot_expr *arg;
    int sum=0, i;

    #if DEBUG
    puts("inside plot_func_add");
    #endif

    if( !env ){
        #if DEBUG
        puts("env is null");
        #endif
        return 0; /* ERROR */
    }

    for( arg=args, i=0; i<argc; ++i, arg+=1 ){
        tmp = plot_eval_expr(env, arg);
        if( ! tmp ){
            #if DEBUG
            puts("value returned by plot_eval is null");
            #endif
            return 0; /* ERROR */
        }

        if( ! (tmp->type == plot_type_number) ){
            #if DEBUG
            puts("value returned by plot_eval is not a number");
            #endif
            return 0; /* ERROR */
        }

        sum += tmp->u.number.val;
    }

    res = calloc(1, sizeof *res);
    if( !res ){
        #if DEBUG
        puts("res failed to calloc");
        #endif
        return 0; /* ERROR */
    }

    res->type = plot_type_number;
    res->u.number.val = sum;

    #if DEBUG
    puts("returning sum of 2 numbers");
    #endif
    return res;
}

/* print error information and then exit
 */
void plot_handle_error(const plot_value *error, const char *place){
    char *type = "unknown";
    if( ! error->type == plot_type_error ){
        printf("Error encountered in '%s', invalid error value supplied\n", place);
        exit(1);
    }

    switch(error->u.error.type){
        case plot_error_alloc_failed:
            type = "alloc failed";
            break;
        case plot_error_bad_args:
            type = "bad args";
            break;
        case plot_error_internal:
            type = "internal error";
            break;
        case plot_error_unbound_symbol:
            type = "unbound symbol";
            break;
        default:
            type = "IMPOSSIBLE ERROR";
            break;
    }

    printf("Error encountered in '%s', error message: '%s', error type: '%s'\n", place, error->u.error.msg, type);
    exit(1);
}

/* internal routine for displaying a value
 */
static void plot_func_display_value(plot_env *env, const plot_value *val){
    plot_value err;

    if( ! val )
        return;

    switch(val->type){
        case plot_type_number:
            printf("%d", val->u.number.val);
            break;
        case plot_type_symbol:
            puts("Unable to print a symbol at this point in time");
            break;
        case plot_type_function:
            puts("Unable to print a function at this point in time");
            break;
        case plot_type_error:
            plot_handle_error(val, "plot_func_display_value");
            break;
        default:
            err.type = plot_type_error;
            err.u.error.type = plot_error_internal;
            err.u.error.msg = "impossible type for value";
            plot_handle_error(&err, "plot_func_display_value");
            break;
    }
}

/* print value to stdout
 */
const plot_value * plot_func_display(plot_env *env, const plot_expr *args, int argc){
    const plot_expr *arg;
    const plot_value *val;
    int i;

    /* FIXME TODO should only care about first arg */
    for(arg=args, i=0; i<argc; ++i, arg=(args+1)){
        val = plot_eval_expr(env, arg);
        if( ! val ){
            /* TODO FIXME handle error cases
             */
            puts( "OOPS" );
        }

        plot_func_display_value(env, val);
    }

}

/* print a newline to stdout
 */
const plot_value * plot_func_newline(plot_env *env, const plot_expr *args, int argc){
    /* FIXME currently ignores arguments, only there to match plot_func interface
     */
    puts("");
}

