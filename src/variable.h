#ifndef PLOT_VARIABLE_H
#define PLOT_VARIABLE_H

#include "value.h"

/* a plot_variable is a mapping of name
 * to a value
 */
typedef struct plot_variable {
    char *name;
    plot_value val;
} plot_variable;

#endif