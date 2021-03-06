#ifndef PLOT_OUTPUT_H
#define PLOT_OUTPUT_H

/* (display obj)
 * (display obj port)
 * print value to provided port or to stdout if no port is provided
 */
struct plot_value * plot_func_display(struct plot_env *env, struct plot_value *args);

/* (newline)
 * (newline port)
 * print a newline to provided port or to stdout if no port is provided
 */
struct plot_value * plot_func_newline(struct plot_env *env, struct plot_value *args);

/* (write-char char)
 * (write-char char port)
 * writes the character char (not an external representation of the char) to the given textual output port
 * returns an unspecified value
 */
struct plot_value * plot_func_output_write_char(struct plot_env *env, struct plot_value *args);

/* (write-string string)
 * (write-string string port)
 * (write-string string port start)
 * (write-string string port start end)
 * writes the characters of string from start to end in left-to-right order to the textual
 * output port provided (or current-output-port)
 * if start of end are not provided then 0 and string-length are used
 */
struct plot_value * plot_func_output_write_string(struct plot_env *env, struct plot_value *args);

#endif
