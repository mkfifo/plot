#include <stdio.h>
#include <string.h> /* strcmp */

#include "forms.h"
#include "value.h"
#include "plot.h"
#include "funcs.h"
#include "eval.h"
#include "hash.h"
#include "env.h"

#define DEBUG 0

/* ignore unused parameter warnings */
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* (define-library (library name) body...) -core -syntax
 * define a library
 *
 * library definitions are of the form
 * (define-library <library-name>
 *      <library-declaration> ...)
 *
 * where a <library-declaration> is any of:
 *      (export <export spec> ...)
 *      (import <import set> ...)
 *      (begin <command or defintion> ...)
 *      (include <filename1> <filename2> ...)
 *      (include-ci <filename1> <filename2> ...)
 *      (include-library-declaration <filename1> <filename2> ...)
 *      (cond-expand <ce-clause1> <ce-clause2> ...)
 */
struct plot_value * plot_form_define_library(struct plot_env *env, struct plot_value *sexpr){
    /* library name */
    plot_value *name;
    /* body to iterate through */
    plot_value *body;
    /* current cursor for iteration*/
    plot_value *cur;
    /* current item we are processing in iteration */
    plot_value *item;
    /* library we generate */
    plot_value *lib;

    /* temporary returned when using other functions
     * to assist
     */
    plot_value *ret;

    /* temporary pointer to string of symbol being inspected */
    const char *str = 0;

    /* FIXME ignore unused variable warnigns */
    #pragma GCC diagnostic ignored "-Wunused-variable"

    /* two new envs, internal and external/exported */
    plot_env *in, *ex;


    /* A library has 3 'parts': */

    /* IMPORTS
     * are processed immediately and modify the internal env.
     */

    /* DEFINITIONS
     * (inside begin exprs)
     * are processed after all imports are performed.
     */
    plot_value *definitions = null;
    /* cursor into definitions */
    plot_value **defcur = &definitions;

    /* EXPORTS
     * are processed after all DEFINITIONS have been processed.
     */
    plot_value *exports = null;
    /* cursor into exports */
    plot_value **expcur = &exports;


    if( !sexpr || sexpr->type != plot_type_pair ){
        return plot_runtime_error(plot_error_bad_args, "expected at least 2 args", "plot_form_define_library");
    }

    name = car(sexpr);
    body = cdr(sexpr);

    /* no parents to either as they are both 'clean' (or emtpy) envs */
    ex = plot_alloc_env(0);
    in = plot_alloc_env(0);
    /* construct out library object */
    lib = plot_new_library(in, ex);

    switch( name->type ){
        case plot_type_pair:
            break;
        case plot_type_symbol:
            break;
        default:
            return plot_runtime_error(plot_error_bad_args, "library name was of incorrect type", "plot_form_define_library");
    }

    /* FIXME TODO check library is not already defined
     */

    /* go through body
     * examine each part
     *  if import -> process immediately
     *  if definition -> add to `defintions`
     *  if export -> add to `exports
     */
    for( cur = body; cur->type == plot_type_pair; cur = cdr(cur) ){
        /* `cur` records our progress through the list (`body`)
         * `item` is the current element we are inspecting inside the list (`body`)
         *
         *      here our `body` is a list
         *          (foo bar)
         *          (fooo barr)
         *          (baz barf)
         *          null
         *
         *      at each step `cur` is pointing to our position through `body` (the list)
         *          first step: cur = (foo bar) (fooo barr) (baz barf) null
         *          second step: cur = (fooo barr) (baz barf) null
         *          third step: cur = (baz barf) null
         *          fourth step: cur = null, null->type != plot_type_pair so for loop terminates
         *
         *      at each step we then set `item` to point to the actual element in the `body` (the list)
         *          first step: item = (foo bar)
         *          second step: cur = (fooo barr)
         *          third step: cur = (baz barf)
         *
         *      we then inspect the car(item) which should be a symbol
         *          first step: car(item) = foo
         *          second step: car(item) = fooo
         *          third step: car(item) = baz
         *
         */
        item = car(cur);

        if( item->type != plot_type_pair ){
            return plot_runtime_error(plot_error_bad_args, "library body was malformed, expected sub-sexpr but found expr", "plot_form_define_library");
        }

        if( car(item)->type != plot_type_symbol ){
            return plot_runtime_error(plot_error_bad_args, "library body was malformed, expected symbol", "plot_form_define_library");
        }


        str = car(item)->u.symbol.val;

        /* (export <export spec> ...)
         * add symbols to u.library.exported
         * symbols may have been defined or may later be defined
         */
        if( ! strcmp(str, "export") ){
            /* here we incr so that we can garbage collect the temporary list
             * without risking collecting object at item
             */
            plot_value_incr(item);
            *expcur = cons(item, null);
            expcur = &lcdr(*expcur);
        }

        /* (import <import set> ...)
         * should be able to re-use normal import and just specify
         * u.library.internal as the env to eval in
         */
        else if( ! strcmp(str, "import") ){
            ret = plot_form_import(in, item);
            if( !ret || ret->type == plot_type_error ){
                puts("plot_form_define_library (import)");
                display_error_expr(item);
                return ret;
            }
        }

        /* (begin <command or definition> ...)
         * normal eval with env specified as u.library.internal
         */
        else if( ! strcmp(str, "begin") ){
            /* here we incr so that we can garbage collect the temporary list
             * without risking collecting object at item
             */
            plot_value_incr(item);
            *defcur = cons(item, null);
            defcur = &lcdr(*defcur);
        }

        /* (include <filename1> <filename2> ...)
         * (include-ci <filename1> <filename2> ...)
         * (include-library-declaration <filename1> <filename2> ...)
         * various include forms
         * FIXME TODO
         */
        else if( ! strcmp(str, "include") ){
            return plot_runtime_error(plot_error_unimplemented, "define-library : include unimplemented", "plot_form_define_library");
        }

        else if( ! strcmp(str, "include-ci") ){
            return plot_runtime_error(plot_error_unimplemented, "define-library : include-ci unimplemented", "plot_form_define_library");
        }

        else if( ! strcmp(str, "include-library-declaration") ){
            return plot_runtime_error(plot_error_unimplemented, "define-library : include-library-declaration unimplemented", "plot_form_define_library");
        }

        /* (cond-expand <ce-clause1> <ce-clause2> ...)
         * FIXME TODO
         */
        else if( ! strcmp(str, "cond-expand") ){
            return plot_runtime_error(plot_error_unimplemented, "define-library : cond-expand unimplemented", "plot_form_define_library");
        }

        /* ERROR: unknown subform */
        else {
            printf("hash of offender was '%llu'\n", car(item)->u.symbol.hash );
            puts("error found when processing `define-library` : unknown subform");
            display_error_expr(item);
            return plot_runtime_error(plot_error_runtime, "library body was malformed, found unknown subform", "plot_form_define_library");
        }
    }

    if( cur->type != plot_type_null ){
        return plot_runtime_error(plot_error_runtime, "library body was malformed, expected pair or null but found neither", "plot_form_define_library");
    }

    item = 0;
    cur = 0;

    /* process DEFINITIONS
     *  NB: (*defcur)->type is safe as we initialise to null (valid object, constant)
     */
    for( *defcur = definitions; (*defcur)->type == plot_type_pair; defcur = &lcdr(*defcur) ){
        cur = car(*defcur);
        /* (begin <command or definition> ...)
         * normal eval with env specified as u.library.internal
         */
        ret = plot_eval_sexpr(in, cur);
        if( ret ){
            if( ret->type == plot_type_error ){
                puts("plot_form_define_library (definition), processing:");
                display_error_expr(cur);
                return ret;
            } else {
                /* throw away temporary, value is ignored */
                plot_value_decr(ret);
                ret = 0;
            }
        }
    }

    /* will cause garbage collection of definitions list
     * however the values in the list should still have sufficient counts
     * to prevent collection (they were incr-d when placed into list
     */
    plot_value_decr(definitions);

    /* process EXPORTS
     *  NB: (*expcur)->type is safe as we initialise to null (valid object, constant)
     *  expcur is '((export ...) ...)
     */
    for( *expcur = exports; (*expcur)->type == plot_type_pair; expcur = &lcdr(*expcur) ){
        /* section 5.6.1 p. 28
         * an export is of the form:
         *      (export <export spec> ...)
         *
         * where <export spec> can take either of the forms:
         *      <identifier>
         *      (rename <identifier1> <identifier2>)
         *
         * example taken from 5.6.2 p. 29
         *      (export make rows cols ref each (rename put! set!))
         *
         */

        /* at this point cur is (export ...) */
        cur = car(*expcur);
        /* must be a form */
        if( cur->type != plot_type_pair ){
            puts("plot_form_define_library (export processing)");
            display_error_expr(cur);
            return plot_runtime_error(plot_error_runtime, "define-library : export processing error, expected pair", "plot_form_define_library");
        }

        /* check car(cur) is really 'export' */
        /* FIXME TODO */
        //hash = car(item)->u.symbol.hash; /* FIXME checking on hash is ugly */
        //if( hash == 656723401804llu){ /* make hasher && ./hasher export */

        /* step through each later item and deal with the two cases */
        for( cur=cdr(cur); cur->type == plot_type_pair; cur=cdr(cur) ){
            if( car(cur)->type == plot_type_pair ){
                /* it is a pair and therefore must be a rename */
                /* (rename identifier identifier) */
                // 540236984962 /* make hasher && ./hasher rename */
                /* FIXME TODO */
                return plot_runtime_error(plot_error_unimplemented, "define-library : (export (rename ...)) unimplemented", "plot_form_define_library");

            } else if( car(cur)->type == plot_type_symbol ){
                /* it is an identifier so is a simple export
                 *  tmp = get internal idn
                 *  set external idn tmp
                 *  decr tmp ;; as both get and set will incr, set is fine as it holds a ref, but we need to decr for the get (as we arent holding a ref)
                 */

                /* item is now current identifier */
                item = car(cur);

                ret = plot_env_get(in, &item->u.symbol);
                /* check ret */
                if( ! ret ){
                    return plot_runtime_error(plot_error_runtime, "define-library : (export identifier...) call to plot_env_get(in, item) yielded null", "plot_form_define_library");
                }
                if( ret->type == plot_type_error ){
                    puts("define-library (export identifier ...)");
                    return ret;
                }

                /* save to external env */
                plot_env_set(ex, &item->u.symbol, ret);

                /* both get and set incr
                 * as set is holding a ref that is correct
                 * but we do not hold the ref that get expects, so must decr
                 */
                plot_value_decr(ret);
                ret = 0;

            } else {
                puts("plot_form_define_library (export processing)");
                display_error_expr(cur);
                return plot_runtime_error(plot_error_runtime, "define-library : export processing error, unexpected type", "plot_form_define_library");
            }
        }
    }

    /* will cause garbage collection of exports list
     * however the values in the list should still have sufficient counts
     * to prevent collection (they were incr-d when placed into list
     */
    plot_value_decr(exports);

    /* add our lib to list of defined-libraries
     */
    plot_add_library(name, lib);

    /* plot_add_library calls cons which will incr our lib
     * our lib is also incr-ed by creation (new library) so the refcount
     * has to be decr-ed to be correct (only reference is now in library list
     */
    plot_value_decr(lib);

    return 0;
}

/* (import (library name) ...) -core -syntax
 * import some libraries
 */
struct plot_value * plot_form_import(struct plot_env *env, struct plot_value *sexpr){

    /* 5.2 Import Declarations, page 25
     * an import declaration takes the following form:
     * (import <import-set> ...)
     *
     * <import-set> takes one of the following forms:
     *      <library-name>
     *      (only <import-set> <identifier> ...)
     *      (except <import-set> <identifier> ...)
     *      (prefix <import-set> <identifier> ...)
     *      (rename <import-set> (<identifier1> <identifier2>) ...)
     *
     */

    return plot_runtime_error(plot_error_unimplemented, "not yet implemented", "plot_form_import");
}

/* (plot-bind identifier ... ) -syntax
 * looks through plot's internal bindings and binds
 * identifiers to current scope
 *
 * eventually exported by (plot internal)
 */
struct plot_value * plot_form_plot_bind(struct plot_env *env, struct plot_value *sexpr){
    return plot_runtime_error(plot_error_unimplemented, "not yet implemented", "plot_form_plot_bind");
}

/* (begin body...) -syntax
 * evaluated each part of the body in order in the containing environment
 * as if the `begin` were not present
 *
 * returns value of final expression OR unspecified
 */
struct plot_value * plot_form_begin(struct plot_env *env, struct plot_value *sexpr){
    plot_value *value = 0;
    plot_value *cur;

    /* step through each part of the body
     * calling eval in our env
     *  begin does not create a new environment
     */
    for( cur=sexpr; cur->type != plot_type_null; cur = cdr(cur) ){
        if( value ){
            plot_value_decr(value);
        }
        /* keep value around so we can return final value */
        value = plot_eval_expr(env, car(cur));
    }

    /* if no value is set then return unspecified */
    if( ! value ){
        value = plot_new_unspecified();
    }

    return value;
}


/* (define what value) -syntax
 */
struct plot_value * plot_form_define(struct plot_env *env, struct plot_value *sexpr){
    plot_value *value;
    plot_value *name;
    plot_value *body;
    plot_value *args;

    /* define has 2 forms:
     * (define a <value>)
     * (define (b args) <function body>)
     */

    name = car(sexpr);
    body = cdr(sexpr);

    #if DEBUG
    puts("define with name:");
    display_error_expr(name);
    puts("with body:");
    display_error_expr(body);
    #endif

    if( name->type == plot_type_pair ){
        /* function form
         * see r7rs p25, section 5.3
         * this is a simple wrapper around lambda
         *
         * (define (v f) b)
         * =>
         * (define v
         *      (lambda f b))
         */

        args = plot_new_pair( cdr(name), body );
        name = car(name);

        value = plot_new_lambda(env, args);
        plot_env_define(env, &(name->u.symbol), value);
        plot_value_decr(value);
        /* NB: we do not zero out lcar and lcdr first
         * as plot_new_lambda keeps a copy of body around
         */
        plot_value_decr(args);

    } else {
        /* value form */

        if( name->type != plot_type_symbol ){
            return plot_runtime_error(plot_error_bad_args, "first arg was not of type plot_type_symbol", "plot_form_define");
        }

        #if DEBUG_FORM || DEBUG
        puts("\tDEFINE: getting value to store");
        #endif

        if( cdr(body)->type != plot_type_null ){
            return plot_runtime_error(plot_error_bad_args, "trailing argument to define", "plot_form_define");
        }
        /* 2nd subform isnt known to be a value ! */
        value = plot_eval_expr(env, car(body));
        if( ! value ){
            return plot_runtime_error(plot_error_internal, "call to eval expr returned null", "plot_form_define");
        }

        if( value->type == plot_type_error ){
            puts("plot_form_define (define)");
            return value;
        }

        #if DEBUG_FORM || DEBUG
        puts("\tDEFINE: value fetched");
        #endif

        #if DEBUG_FORM || DEBUG
        puts("\tDEFINE: success!");
        printf("\tStoring value type '%d', under name '%s'\n'", value->type, name->u.symbol.val);
        #endif
        plot_env_define(env, &(name->u.symbol), value);
        /* decrement value as eval and define will both increment it and we are not keeping a reference around */
        plot_value_decr(value);
    }

    return plot_new_unspecified();
}

/* (lambda args body...) -syntax
 *
 * a lambda can have a few different forms
 *
 * Currently only the following form is supported (fixed number of args)
 * (lambda (arg1 arg2 ...) body ...)
 *
 * The following variable argument forms are not currently supported
 * (lambda args body ...)
 * (lambda (arg1 arg2 . rest) body ...)
 *
 * TODO:
 *  the first of the variable args forms are caught and trigger a plot_error_unimplemented
 *  however the latter form is not yet caught.
 */
struct plot_value * plot_form_lambda(struct plot_env *env, struct plot_value *sexpr){
    plot_value *args, *arg;
    plot_value *body;

    /* our sexpr will be of the form (args body... )
     */

    args = car(sexpr);
    body = cdr(sexpr);

    if( body->type == plot_type_null ){
        return plot_runtime_error(plot_error_bad_args, "no body found", "plot_form_lambda");
    }

    switch( args->type ){
        case plot_type_null:
            /* FIXME
             * no arguments, currently a bug in parsing of null
             * (lambda () ...)
             * will consider the () to be a null rather than an empty pair
             * see TODO and bugs/empty-list.scm
             */
            break;

        case plot_type_pair:
            /* list of arguments */
            /* check all subforms are symbols */
            for( arg=args; arg->type != plot_type_null; arg = cdr(arg) ){
                /* expected list, got improper pair */
                if( arg->type != plot_type_pair ){
                    return plot_runtime_error(plot_error_runtime, "invalid arg list, improper pairs not allowed", "plot_form_lambda");
                }

                if( car(arg)->type != plot_type_symbol ){
                    return plot_runtime_error(plot_error_runtime, "invalid parameter type, not of type plot_type_symbol", "plot_form_lambda");
                }
            }
            break;

        default:
            /* catch unimplemented list args form (lambda args body ...) */
            /* r7rs page 13 section 4.1.4 says:
             * ((lambda x x) 3 4 5 6) => (3 4 5 6)
             */
            return plot_runtime_error(plot_error_unimplemented, "lambda auto-list arguments not yet supported", "plot_form_lambda");
            break;
    }

    return plot_new_lambda(env, sexpr);
}

/* (if cond if-expr else-expr) -syntax
 * (if cond if-expr)
 *
 * evaluates cond and then picks the branch to evaluated based on truthy-ness (see `plot_truthy`) of cond
 *  if cond is truthy then evaluate if-expr and return result
 *  if cond is not truthy then evaluate else-expr and return result,
 *      if no else-expr is provided then the result is unspecified
 *
 * (if #f 'hello)        ;; => unspecified
 * (if #t 'hello)        ;; => 'hello
 * (if #f 'hello 'world) ;; => 'world
 *
 *  the branch not taken is not evaluated
 *
 * (if #f (function-that-doesnt-exist) 0) ;; => 0
 *  if if-expr branch was taken this would trigger a runtime error
 *
 */
struct plot_value * plot_form_if(struct plot_env *env, struct plot_value *sexpr){
    plot_value *value;
    plot_value *cond;
    plot_value *if_expr;
    plot_value *else_expr = 0;

    /* scheme if's can have 2 forms
     * (if cond if-expr) ; 'guard'
     * (if cond if-expr else-expr) ; 'branching'
     *
     * when we receive it will be either of
     *  (cond if-expr)
     *  (cond if-expr else-expr)
     *
     */

    cond = car(sexpr);
    if_expr = car(cdr(sexpr));
    else_expr = cdr(cdr(sexpr));

    if( else_expr->type == plot_type_pair ) {
        else_expr = car(else_expr);
    } else {
        /* null */
        else_expr = 0;
    }

    /* must remember to decr in all branches that
     * do not return value
     */
    value = plot_eval_expr(env, cond);

    if( ! value ){
        #if DEBUG_FORM || DEBUG
        puts("\teval of if condition returned NULL");
        #endif
        return 0; /* FIXME ERROR */
    }

    if( value->type == plot_type_error ){
        puts("plot_form_if (if cond)");
        return value;
    }

    if( plot_truthy(value) ){
        plot_value_decr(value);

        value = plot_eval_expr(env, if_expr);

        if( ! value ){
            #if DEBUG_FORM || DEBUG
            puts("\teval of if true branch returned NULL");
            #endif
            return 0; /* FIXME ERROR */
        }

        if( value->type == plot_type_error ){
            puts("plot_form_if (if if-expr)");
            return value;
        }

    } else if( else_expr ){ /* (if cond if-expr else-expr) */
        plot_value_decr(value);

        value = plot_eval_expr(env, else_expr);

        if( ! value ){
            #if DEBUG_FORM || DEBUG
            puts("\teval of if false branch returned NULL");
            #endif
            return 0; /* FIXME ERROR */
        }

        if( value->type == plot_type_error ){
            puts("plot_form_if (if else-expr)");
            return value;
        }

    } else {
        plot_value_decr(value);

        /* (display (if #f "hello")) ;; => unspecified
         * in csi this is 'unspecified'
         * in racket (lang scheme) there is no output
         *
         * r5rs says: (4.1.5 page 10)
         *  "if the <test> yields a false value and no <alternate>
         *  is spcified, then the result of the expression is
         *  unspecified"
         *
         * r6rs says: (11.4.3 page 33)
         *  "if the <test> yields #f and no <alternate> is
         *  specified, then the result of the expression is
         *  unspecified"
         *
         * r7rs says: (4.1.5 page 13)
         *  "if the <test> yields a false value and no <alternative>
         *  if specified, then the result of teh expression is
         *  unspecified"
         *
         * plot takes this far too literally and has a literal value
         * specifically for 'unspecified'
         */
        return plot_new_unspecified(); /* success */
    }

    /* returns value set in either branch of if evaluation */
    return value;
}

/* (cond (<test> <expression>)...) -syntax
 *
 * (cond ((> 3 2) 'greater)
 *       ((< 3 2) => 'less)
 *       (else 'equal))
 *
 * a cond is a set of <test> <expression> pairs which are tried in order:
 *   if a <test> results in #t (or a truthy value) then expression is eval-ed
 *     and the result of the cond expression is the result of this expression.
 *
 *   else is always true.
 *
 *   if all <test>s are tried and found to be false then the value of the cond
 *     is unspecified.
 *
 *   possible forms:
 *      ((< 3 2) 'greater)
 *      (else 'equal)
 *      ((> 3 2) => 'less)
 *
 */
struct plot_value * plot_form_cond(struct plot_env *env, struct plot_value *sexpr){
    /* current cursor into args */
    plot_value *cur;
    /* result from an eval call */
    plot_value *res;
    /* temporary value used in testing and eval */
    plot_value *tmp;

    if( sexpr->type != plot_type_pair ){
        return plot_runtime_error(plot_error_bad_args, "expected at least 1 cond clause", "plot_form_cond");
    }

    for( cur = sexpr; cur->type == plot_type_pair; cur = cdr(cur) ){
        if( cur->type != plot_type_pair ){
            return plot_runtime_error(plot_error_bad_args, "expected cond clause", "plot_form_cond");
        }

        if( car(cur)->type != plot_type_pair ){
            return plot_runtime_error(plot_error_bad_args, "expected cond clause", "plot_form_cond");
        }

        /* assign test position to tmp */
        tmp = car(car(cur));

        /* if tmp is 'else' then it is considered true */
        if( tmp->type == plot_type_symbol && !strcmp("else", tmp->u.symbol.val) ){
            /* NB: plot_truthy no longer handles decr for us
             * make sure to decr for both branches below
             */
            res = plot_new_boolean(1);
        } else {
            /* otherwise we eval */
            /* NB: plot_truthy no longer handles decr for us
             * make sure to decr for both branches below
             */
            res = plot_eval_expr(env, tmp);
        }

        /* test for truthyness
         * plot_truthy no longer handles our decr
         */
        if( plot_truthy(res) ){
            if( cdr(car(cur))->type != plot_type_pair ){
                return plot_runtime_error(plot_error_bad_args, "expected cond expression, none found", "plot_form_cond");
            }

            /* assign expression to tmp */
            tmp = car(cdr(car(cur)));

            /* if this is `=>` then the next element is the expression */
            if( tmp->type == plot_type_symbol && !strcmp("=>", tmp->u.symbol.val) ){
                if( cdr(cdr(car(cur)))->type != plot_type_pair ){
                    return plot_runtime_error(plot_error_bad_args, "expected cond expression, none found in => cond clause", "plot_form_cond");
                }
                tmp = car(cdr(cdr(car(cur))));
            }

            /* decr existing res */
            plot_value_decr(res);

            res = plot_eval_expr(env, tmp);
            return res;
        } else {
            /* decr res */
            plot_value_decr(res);
        }
    }

    return plot_new_unspecified();
}


/* (set! variable value) -syntax
 */
struct plot_value * plot_form_set(struct plot_env *env, struct plot_value *sexpr){
    plot_value *value;
    plot_value *name, *expr;

    /* (set! variable expr)
     * car = symbol set!
     * cdr =
     *  car = symbol <variable>
     *  cdr =
     *      car = <expr>
     *      cdr = null
     */

    name = car(sexpr);
    expr = car(cdr(sexpr));

    if( name->type != plot_type_symbol ||
        cdr(cdr(sexpr))->type != plot_type_null ){
        return plot_runtime_error(plot_error_bad_args, "malformed set! form", "plot_form_set");
    }

    value = plot_eval_expr(env, expr);
    if( ! value ){
        #if DEBUG_FORM || DEBUG
        puts("\teval of set value returned NULL");
        #endif
        return 0; /* FIXME ERROR */
    }
    if( value->type == plot_type_error ){
        puts("plot_form_set (set!");
        return value;
    }

    if( ! plot_env_set(env, &(name->u.symbol), value) ){
        puts("\tset! call to plot_env_set failed");
        return 0; /* FIXME ERROR */
    }

    return plot_new_unspecified();
}

/* (quote expr) -syntax
 */
struct plot_value * plot_form_quote(struct plot_env *env, struct plot_value *sexpr){
    /* current position in input */
    plot_value *in;
    /* output */
    plot_value *out, **cur;


    #if DEBUG_FORM || DEBUG
    puts("quote of sexpr:");
    display_error_expr(sexpr);
    #endif

    out = 0;
    cur = &out;

    switch( car(sexpr)->type ){
        case plot_type_pair:
            /* return map quote cdr(sexpr);
             * '(a b c) => (list 'a 'b 'c)
             */
            for( in=car(sexpr); in->type == plot_type_pair; in = cdr(in) ){
                *cur = plot_new_pair( 0, plot_new_null() );
                lcar(*cur) = plot_form_quote( env,  plot_new_pair( car(in), plot_new_null() ) );
                cur = &lcdr(*cur);
            }
            return out;
            break;
        default:
            return car(sexpr);
            break;
    }

    plot_fatal_error("Impossible expression type given to plot_form_quote");
    return 0;
}

/* (delay expr) -syntax
 */
struct plot_value * plot_form_delay(struct plot_env *env, struct plot_value *sexpr){
    plot_value *val;
    /* delay will have
     * car = symbol delay
     * cdr =
     *  car = expr
     *  cdr = null
     */

    if( cdr(sexpr)->type != plot_type_null ){
        return plot_runtime_error(plot_error_bad_args, "malformed delay expression", "plot_form_delay");
    }

    val = plot_new_promise(env, car(sexpr));
    return val;
}

/* (and obj1 obj2 ...) -synax
 * logical and of all arguments
 *
 * returns #f if any expressions evaluated to #f
 *
 * otherwise the value of the last truthy expression is returned.
 *
 * only evaluates arguments until the first falsey expression is encountered.
 */
struct plot_value * plot_form_and(struct plot_env *env, struct plot_value *args){
    /* current iterator through args list */
    plot_value *cur;
    /* temporary value for result of evaluating arguments */
    plot_value *value = 0;

    if( args->type != plot_type_pair || cdr(args)->type != plot_type_pair ){
        return plot_new_boolean(true);
    }

    for( cur = args; cur->type == plot_type_pair; cur = cdr(cur) ){
        /* `and` must return the value of the last truthy expression,
         * so the iterator pattern is a little odd
         * we only decr the value before overwriting it
         */
        if( value )
            plot_value_decr(value);

        value = plot_eval_expr(env, car(cur));

        if( ! plot_truthy(value) ){
            /* we must decr here as we are about to lose the local variable value */
            plot_value_decr(value);
            return plot_new_boolean(false);
        }
    }

    /* do not need to incr value as eval creates it with count of 1
     * would usually decr after testing, but since we are returning we are keeping a copy around
     * - value is created with count 1 (+1)
     * - returning it so need to increase count (+1)
     * - losing local copy (value) so need to decrease count (-1)
     * net: +1, no change needed
     */
    return value;
}

/* (or obj1 obj2 ...) -syntax
 * logical or of all arguments
 *
 * if all expressions evaluate to #f or if there are no expressions then #f is returned
 *
 * otherwise the value of the first truthy expression is returned.
 *
 * only evaluates arguments until the first truthy expression is encountered.
 */
struct plot_value * plot_form_or(struct plot_env *env, struct plot_value *args){
    /* current iterator through args list */
    plot_value *cur;
    /* temporary value for result of evaluating arguments */
    plot_value *value;

    for( cur = args; cur->type == plot_type_pair; cur = cdr(cur) ){
        value = plot_eval_expr(env, car(cur));
        if( plot_truthy(value) ){
            /* do not need to incr value as eval creates it with count of 1
             * would usually decr after testing, but since we are returning we are keeping a copy around
             * - value is created with count 1 (+1)
             * - returning it so need to increase count (+1)
             * - losing local copy (value) so need to decrease count (-1)
             * net: +1, no change needed
             */
            return value;
        }
        plot_value_decr(value);
    }

    return plot_new_boolean(false);
}

