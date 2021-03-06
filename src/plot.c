#include <stdlib.h>
#include <stdio.h>

#include "value.h"
#include "hash.h"
#include "env.h"
#include "funcs.h"
#include "plot.h"

#include "bindings.h"

#define LENGTH(x) (sizeof x / sizeof x[0])

#define DEBUG 0

typedef struct plot {
    /* error value returned by plot_error */
    struct plot_value error;
    /* default global env */
    struct plot_env *env;

    /* known internal definitions
     * possibly exposed to userland
     * via plot-bind
     */
    struct plot_env *library_forms;

    /* libraries we have seen defined
     * alist (assosiation list)
     * key is list of symbols e.g. `(scheme base`)
     * value is a plot_library
     */
    struct plot_value *libraries;

    /**** GC arena ****/
    /* array of malloc'd values */
    struct plot_value *value_arena;
    /* number of values in above arena */
    int num_value_allocated;
    /* number of values currently in use by callers */
    int num_value_used;
    /* head of list of reclaimed plot_value(s) */
    struct plot_value *value_reclaimed;

    struct plot_env *env_arena;
    int num_env_allocated;
    int num_env_used;
    struct plot_env *env_reclaimed;

    /* optimisation to reduce waste
     */
    struct plot_value unspecified_constant;
    struct plot_value null_constant;
    struct plot_value eof_constant;
    struct plot_value true_constant;
    struct plot_value false_constant;

#if GC_STATS
    /**** garbage stats ****/
    /* number of plot_value(s) reclaimed (garbage collected) */
    int num_value_reclaimed;
    /* number of plot_value(s) recycling (handed back out after GC) */
    int num_value_recycled;

    int num_env_reclaimed;
    int num_env_recycled;

    int num_he_reclaimed;
    int num_he_recycled;
#endif
#if HASH_STATS
    /* comparisong inside hash_get */
    int num_hash_comp;
    /* calls to hash_get*/
    int num_hash_get;
    /* calls to env_get */
    int num_env_get;
    /* number of times env_get runs
     * inside it's loop
     */
    int num_env_loop;
#endif
} plot;

static plot *plot_instance;

static void plot_gc_incr(struct plot_gc *g);
static void plot_gc_value_init(void);
static void plot_gc_env_init(void);

int plot_init(void){
    size_t i=0;

    plot_instance = calloc(1, sizeof *plot_instance);
    if( ! plot_instance ){
        plot_fatal_error("alloc failed in plot_init");
        return 0; /* keep the compiler happy */
    }

    plot_instance->error.gc.refcount = -1;
    plot_instance->error.type = plot_type_error;

    plot_instance->unspecified_constant.type = plot_type_unspecified;
    plot_instance->unspecified_constant.gc.refcount = -1;

    plot_instance->null_constant.type = plot_type_null;
    plot_instance->null_constant.gc.refcount = -1;

    plot_instance->eof_constant.type = plot_type_eof;
    plot_instance->eof_constant.gc.refcount = -1;

    plot_instance->true_constant.type = plot_type_boolean;
    plot_instance->true_constant.gc.refcount = -1;
    plot_instance->true_constant.u.boolean.val = 1;

    plot_instance->false_constant.type = plot_type_boolean;
    plot_instance->false_constant.gc.refcount = -1;
    plot_instance->false_constant.u.boolean.val = 0;

    plot_gc_value_init();
    plot_gc_env_init();

    plot_instance->env = plot_alloc_env(0);
    if( ! plot_instance->env ){
        puts("call to plot_env_init failed");
        plot_cleanup();
        plot_fatal_error("call to plot_env_init failed");
        return 0; /* keep the compiler happy */
    }

    plot_instance->libraries = plot_new_null();

    plot_instance->library_forms = plot_alloc_env(0);
    if( ! plot_instance->env ){
        puts("call to plot_env_init for internal bindings failed");
        plot_cleanup();
        plot_fatal_error("call to plot_env_init for internal bindings failed");
        return 0; /* keep the compiler happy */
    }

    /* core form bindings
     * e.g.:
     *      define-library
     *      import
     *      export
     */
    for( i=0; i<LENGTH(core_forms); ++i ){
        if( ! plot_env_define( plot_instance->env, &(core_forms[i].sym), &(core_forms[i].func) ) ){
            printf("error in plot_init defining core form symbol '%s'\n", core_forms[i].sym.val);

        }
    }

    /* library form bindings
     * these are forms that are exported by libraries but are implemented in c
     * libraries must `bind` these forms and then export them
     * the bindings will then only be visible when a library is imported that exports such a binding
     *
     * this is plot specific, example (unimplemented) syntax:
     *
     *  (define-library (foo bar)
     *      (import (plot internal))
     *      (plot-bind or)
     *      (export or))
     *
     *  (import (foo bar)) ;; exports or
     *  (display (or #f #t))
     */
    for( i=0; i<LENGTH(library_forms); ++i ){
        if( ! plot_env_define( plot_instance->library_forms, &(library_forms[i].sym), &(library_forms[i].func) ) ){
            printf("error in plot_init defining internal binding symbol '%s'\n", library_forms[i].sym.val);
        }
    }

    return 1;
}

struct plot_env * plot_get_global_env(void){
    if( ! plot_instance ){
        plot_fatal_error("plot_get_global_env called before plot_instance setup");
        return 0; /* keep the compiler happy */
    }

    return plot_instance->env;
}

struct plot_env * plot_get_library_forms(void){
    if( ! plot_instance ){
        plot_fatal_error("plot_get_library_forms called before plot_instance setup");
        return 0; /* keep the compiler happy */
    }

    return plot_instance->library_forms;
}

/* return alist (assoc list) for defined libraies
 */
struct plot_value * plot_get_libraries(void){
    if( ! plot_instance ){
        plot_fatal_error("plot_get_libraries called before plot_instance setup");
        return 0; /* keep the compiler happy */
    }

    return plot_instance->libraries;
}

/* set libraries list
 * method does NOT incr or decr any values
 * caller is expected to manage all such things
 * even on currently stored library value (plot_instance->libraries)
 */
void plot_set_libraries(struct plot_value * libs){
    if( ! plot_instance ){
        plot_fatal_error("plot_set_libraries called before plot_instance setup");
        return; /* keep the compiler happy */
    }

    plot_instance->libraries = libs;
}

/* add a defined library
 * method calls `cons` on supplied lib and supplied name
 * which WILL incr them
 * no other refcounts are modified
 */
void plot_add_library(struct plot_value *name, struct plot_value *lib){
    if( ! plot_instance ){
        plot_fatal_error("plot_add_library called before plot_instance setup");
        return; /* keep the compiler happy */
    }

    /* FIXME check for pre-existing library
     */

    /* libraries is an association list
     * key is `name` and value is `lib`
     */
    plot_instance->libraries = cons(cons(name, lib), plot_instance->libraries);
}

void plot_cleanup(){
#if GC_STATS

    printf("\nplot GC stats:\n");

    printf("##### plot_value stats #####\n");
    printf("\tMax in use '%d'\n", plot_instance->num_value_used);
    printf("\tStill had in the bank: '%d'\n", plot_instance->num_value_allocated - plot_instance->num_value_used );
    printf("\tnum recycled '%d', reclaimed '%d'\n", plot_instance->num_value_recycled, plot_instance->num_value_reclaimed);
    printf("\tused - reclaimed = '%d'\n", plot_instance->num_value_used - plot_instance->num_value_reclaimed);
    printf("\tnon-reclaimed (still in use at cleanup) '%d'\n", (plot_instance->num_value_used + plot_instance->num_value_recycled) - plot_instance->num_value_reclaimed);

    printf("##### plot_env stats #####\n");
    printf("\tMax in use '%d'\n", plot_instance->num_env_used);
    printf("\tStill had in the bank: '%d'\n", plot_instance->num_env_allocated - plot_instance->num_env_used );
    printf("\tnum recycled '%d', reclaimed '%d'\n", plot_instance->num_env_recycled, plot_instance->num_env_reclaimed);
    printf("\tused - reclaimed = '%d'\n", plot_instance->num_env_used - plot_instance->num_env_reclaimed);
    printf("\tnon-reclaimed (still in use at cleanup) '%d'\n", (plot_instance->num_env_used + plot_instance->num_env_recycled) - plot_instance->num_env_reclaimed);

    printf("\n");

#endif

#if HASH_STATS
    printf("###### plot hash stats #####\n");
    printf("\thash_comp '%d'\n", plot_instance->num_hash_comp);
    printf("\thash_get '%d'\n", plot_instance->num_hash_get);
    printf("\tcomp/get '%f'\n", (double) plot_instance->num_hash_comp /  (double) plot_instance->num_hash_get);
    printf("\tenv_get '%d'\n", plot_instance->num_env_get);
    printf("\tenv_loop '%d'\n", plot_instance->num_env_loop);
    printf("\n");
#endif

    /* FIXME
     * we need to make this a better mirror of init()
     * we currently rely too much on cleanup on exit
     */
    if( plot_instance ){
        plot_value_decr(plot_instance->libraries);
        plot_env_cleanup(plot_instance->library_forms);
        plot_env_cleanup(plot_instance->env);
        free(plot_instance);
    }
}

/* generates an appropriate error value, prints it, and then returns it
 * caller is expected to return the result
 *
 * each frame within the callstack is expected to detect this,
 * print relevant trace information, and then return the value further
 */
struct plot_value * plot_runtime_error(plot_error_type type, const char *msg, const char *location){
    plot_value *err;
    const char *error_type_str;

    if( ! plot_instance ){
        plot_fatal_error("plot_runtime_error called without plot_instance being initialised");
        return 0; /* keep the compiler happy */
    }

    err = &plot_instance->error;
    err->type = plot_type_error;
    err->u.error.type = type;
    err->u.error.msg = msg;
    err->u.error.location = location;

    switch(err->u.error.type){
        case plot_error_alloc_failed:
            error_type_str = "alloc failed";
            break;
        case plot_error_bad_args:
            error_type_str = "bad args";
            break;
        case plot_error_internal:
            error_type_str = "internal error";
            break;
        case plot_error_unbound_symbol:
            error_type_str = "unbound symbol";
            break;
        case plot_error_unimplemented:
            error_type_str = "unimplemented";
            break;
        case plot_error_undefined:
            error_type_str = "symbol is undefined";
            break;
        case plot_error_runtime:
            error_type_str = "runtime error in source program";
            break;
        default:
            error_type_str = "IMPOSSIBLE ERROR";
            break;
    }

    printf("Error encountered in '%s':\n\terror message: '%s'\n\terror type: '%s'\n\n", err->u.error.location, err->u.error.msg, error_type_str);
    return err;
}

/* a fatal error will print the supplied string and then `exit(1)`
 */
void plot_fatal_error(const char *str){
    puts("FATAL ERROR OCCURED:");
    puts(str);
    exit(1);
}

void plot_gc_incr(struct plot_gc *g){
    if( !g )
        return;

    if( g->refcount < 0 ){
        /* object is NOT under control of gc, do not touch */
        return;
    }

    if( g->refcount > 0 ){
        ++g->refcount;
    } else {
        /* this object already has a refcount of 0, this should be impossible, ERROR */
        plot_fatal_error("plot_gc_incr: This object already has a refcount of 0, ERROR has occurred, terminating");
    }
}

/* increase reference count on plot_value */
void plot_value_incr(struct plot_value *p){
    plot_gc_incr( (struct plot_gc *) p);
}

/* decrease reference count on plot_value
 * may trigger collection
 */
void plot_value_decr(struct plot_value *p){
    if( !p )
        return;

    #if DEBUG
    printf("inside plot_value_decr looking at object '%p' with refcount '%d'\n",
          (void*) p, p->gc.refcount );
    #endif

    if( p->gc.refcount < 0 ){
        #if DEBUG
        puts("\tthis object is NOT under gc control, leaving untouched");
        #endif
        /* object is NOT under control of gc, do not touch */
        return;
    }

    if( p->gc.refcount > 0 ){
        #if DEBUG
        puts("\tdecrementing refcount");
        #endif
        --p->gc.refcount;

        if( p->gc.refcount == 0 ){
            #if DEBUG
            puts("\treclaiming value");
            #endif
            #if GC_STATS
            ++plot_instance->num_value_reclaimed;
            #endif
            /* time to reclaim */
            //fprintf(stderr, "RECLAIMING\n"); // 2692537
            p->gc.next = (struct plot_gc *) plot_instance->value_reclaimed;
            plot_instance->value_reclaimed = p;
            plot_value_decons(p);
            p->type = plot_type_reclaimed; /* FIXME useful for testing */
        #if DEBUG
        } else {
            puts("\tsparing object");
        #endif
        }
    } else {
        /* this object already has a refcount of 0, ERROR */
        puts("plot_value_decr: This object already has a refcount of 0, ERROR has occurred, terminating");
        exit(1);
    }

    #if DEBUG
    puts("\tleaving plot_value_decr");
    #endif
}

/* initialise value arena
 *
 * returns on success
 * will cause a fatal error and exit program on failure
 */
void plot_gc_value_init(void){
    if( ! plot_instance ){
        puts("plot_gc_value_init called without plot_instance being initialised");
        exit(1);
    }

    plot_instance->num_value_used = 0;
    #if GC_STATS
    plot_instance->num_value_reclaimed = 0;
    #endif
    /* FIXME
     * before gc (fibo 31) would require 6731342 plot_values
     * after tracking waste, this included 2692537 wasted values (mostly from if test position)
     * this means the minimum size of num_values_allocated must be 6731342 - 2692537 = 4038805
     *
     * now after func-call-temp-reclaiming this value can be reduced to 2692574
     *
     * and now, after env-cleanup (fibo 31) only needs 52
     */
    plot_instance->num_value_allocated = 2000;
    plot_instance->value_arena = calloc( plot_instance->num_value_allocated, sizeof (struct plot_value) );
    if( ! plot_instance->value_arena ){
        plot_fatal_error("plot_gc_value_init ERROR: failed to calloc value arena");
        return; /* keep the compiler happy */
    }
}

/* increase reference count on plot_env */
void plot_env_incr(struct plot_env *e){
    plot_gc_incr( (struct plot_gc *) e);
}

/* decrease reference count on plot_env
 * may trigger collection of the env,
 * it's parents and it's stored values
 */
void plot_env_decr(struct plot_env *e){
    if( !e )
        return;

    #if DEBUG
    printf("inside plot_env_decr considering object '%p' with refcount '%d'\n",
          (void*) e, e->gc.refcount);
    #endif

    if( e->gc.refcount < 0 ){
        #if DEBUG
        puts("\tthis object is not under GC control, leaving untouched");
        #endif
        /* object is NOT under control of gc, do not touch */
        return;
    }

    if( e->gc.refcount > 0 ){
        #if DEBUG
        puts("\tdecrementing refcount");
        #endif

        --e->gc.refcount;
        if( e->gc.refcount == 0 ){
            #if DEBUG
            puts("\treclaiming object");
            #endif
            #if GC_STATS
            ++plot_instance->num_env_reclaimed;
            #endif
            /* time to reclaim */
            //fprintf(stderr, "RECLAIMING\n"); // 2692537
            e->gc.next = (struct plot_gc *) plot_instance->env_reclaimed;
            plot_instance->env_reclaimed = e;

            /* env cleanup will decr parent and trigger decr on all stored values */
            plot_env_cleanup(e);
        #if DEBUG
        } else {
            puts("\tsparing object");
        #endif
        }
    } else {
        /* this object already has a refcount of 0, ERROR */
        plot_fatal_error("plot_env_decr: This object already has a refcount of 0, ERROR has occurred, terminating");
        return; /* keep the compiler happy */
    }

    #if DEBUG
    puts("\tleaving plot_env_decr");
    #endif
}

/* initialise env arena
 *
 * returns on success
 * will cause a fatal error and exit on failure
 */
void plot_gc_env_init(void){
    if( ! plot_instance ){
        plot_fatal_error("plot_gc_env_init called without plot_instance being initialised");
        return; /* keep the compiler happy */
    }

    plot_instance->num_env_used = 0;
    #if GC_STATS
    plot_instance->num_env_reclaimed = 0;
    #endif
    plot_instance->num_env_allocated = 2000;
    plot_instance->env_arena = calloc( plot_instance->num_env_allocated, sizeof (struct plot_env) );
    if( ! plot_instance->env_arena ){
        plot_fatal_error("plot_gc_env_init ERROR: failed to calloc env arena");
        return; /* keep the compiler happy */
    }
}

/* allocate new ref counted value */
struct plot_value * plot_alloc_value(void){
    struct plot_value *p;
    if( ! plot_instance ){
        plot_fatal_error("plot_alloc_value called without plot_instance being initialised");
        return 0; /* keep the compiler happy */
    }

    if( plot_instance->value_reclaimed ){
        #if GC_STATS
        ++plot_instance->num_value_recycled;
        #endif
        //fprintf(stderr, "reusing\n");
        p = plot_instance->value_reclaimed;
        /* gc is the first element of plot_value so this is safe */
        plot_instance->value_reclaimed = (plot_value *) p->gc.next;
        p->gc.refcount = 1;
        p->gc.next = 0;
        return p;
    }
    if( plot_instance->num_value_used >= plot_instance->num_value_allocated ){
        /* FIXME realloc */
        printf("THE BANK IS EMPTY; allocated all '%d' plot_value(s)\n", plot_instance->num_value_used);
        plot_fatal_error("The bank is empty");
        return 0; /* keep the compiler happy */
    } else {
        /* hand out resources */
        p = &(plot_instance->value_arena[ plot_instance->num_value_used ++]);
        p->gc.refcount = 1;
        p->gc.next = 0;
        return p;
    }
}

/* allocate new NON-ref counted value
 * this is needed for constants until
 * a better solution comes along
 *
 * this has been mostly-deprecated in favour of plot_value_constantify
 */
struct plot_value * plot_alloc_constant(void){
    struct plot_value *p;
    p = plot_alloc_value();
    p->gc.refcount = -1; /* FIXME hack */
    return p;
}


/* allocate new env
 * parent is the enclosing environment, or 0
 */
struct plot_env * plot_alloc_env(struct plot_env *parent){
    struct plot_env *e;
    if( ! plot_instance ){
        plot_fatal_error("plot_alloc_env called without plot_instance being initialised");
        return 0; /* keep the compiler happy */
    }

    if( plot_instance->env_reclaimed ){
        #if GC_STATS
        ++plot_instance->num_env_recycled;
        #endif
        //fprintf(stderr, "reusing\n");
        e = plot_instance->env_reclaimed;
        /* gc is the first element of plot_env so this is safe */
        plot_instance->env_reclaimed = (plot_env *) e->gc.next;
        e->gc.refcount = 1;
        e->gc.next = 0;
        //printf("\tRECLAIMED: trying to set parent for '%p', parent is '%p'\n", (void*)e, (void*)parent);
        if( ! plot_env_init(e, parent) ){
            plot_fatal_error("plot_value_new: call to plot_env_init failed");
            return 0; /* keep the compiler happy */
        }
        return e;
    }
    if( plot_instance->num_env_used >= plot_instance->num_env_allocated ){
        /* FIXME realloc */
        printf("THE BANK IS EMPTY; allocated all '%d' plot_env(s)\n", plot_instance->num_env_used);
        plot_fatal_error("plot_alloc_env: the bank is empty");
        return 0; /* keep the compiler happy */
    } else {
        /* hand out resources */
        e = &(plot_instance->env_arena[ plot_instance->num_env_used ++]);
        e->gc.refcount = 1;
        e->gc.next = 0;
        //printf("\tNEW: trying to set parent for '%p', parent is '%p'\n", (void*)e, (void*)parent);
        if( ! plot_env_init(e, parent) ){
            plot_fatal_error("plot_value_new: call to plot_env_init failed");
            return 0; /* keep the compiler happy */
        }
        return e;
    }

}

/* allocate new string */
char * plot_alloc_string(int len){
    char * c;
    c = calloc(len, sizeof *c);
    if( ! c ){
        /* TODO FIXME use plot error handling */
        plot_fatal_error("plot_alloc_string: calloc failed, dying");
        return 0; /* keep the compiler happy */
    }
    return c;
}

struct plot_value * plot_get_unspecified_constant(void){
    if( ! plot_instance){
        plot_fatal_error("plot_get_unspecified_constant called without plot_instance initialisation");
        return 0; /* keep the compiler happy */
    }
    return &plot_instance->unspecified_constant;
}

struct plot_value * plot_get_null_constant(void){
    if( ! plot_instance){
        plot_fatal_error("plot_get_null_constant called without plot_instance initialisation");
        return 0; /* keep the compiler happy */
    }
    return &plot_instance->null_constant;
}

struct plot_value * plot_get_eof_constant(void){
    if( ! plot_instance){
        plot_fatal_error("plot_get_eof_constant called without plot_instance initialisation");
        return 0; /* keep the compiler happy */
    }
    return &plot_instance->eof_constant;
}

struct plot_value * plot_get_true_constant(void){
    if( ! plot_instance){
        plot_fatal_error("plot_get_true_constant called without plot_instance initialisation");
        return 0; /* keep the compiler happy */
    }
    return &plot_instance->true_constant;
}

struct plot_value * plot_get_false_constant(void){
    if( ! plot_instance){
        plot_fatal_error("plot_get_false_constant called without plot_instance initialisation");
        return 0; /* keep the compiler happy */
    }
    return &plot_instance->false_constant;
}

#if HASH_STATS
void plot_stats_hash_get(void){
    if( ! plot_instance ){
        plot_fatal_error("plot_stats_hash_get called without plot_instance initialisation");
        return; /* keep the compiler happy */
    }
    ++plot_instance->num_hash_get;
}

void plot_stats_hash_comp(void){
    if( ! plot_instance ){
        plot_fatal_error("plot_stats_hash_comp called without plot_instance initialisation");
        return; /* keep the compiler happy */
    }
    ++plot_instance->num_hash_comp;
}

void plot_stats_env_get(void){
    if( ! plot_instance ){
        plot_fatal_error("plot_stats_env_get called without plot_instance initialisation");
        return; /* keep the compiler happy */
    }
    ++plot_instance->num_env_get;
}

void plot_stats_env_loop(void){
    if( ! plot_instance ){
        plot_fatal_error("plot_stats_env_loop called without plot_instance initialisation");
        return; /* keep the compiler happy */
    }
    ++plot_instance->num_env_loop;
}

#endif

