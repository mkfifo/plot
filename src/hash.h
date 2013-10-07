#ifndef PLOT_HASH_H
#define PLOT_HASH_H

struct plot_symbol;
struct plot_value;

/* a plot_hash is a singly linked-list of plot_hash_entry (s)
 * and a count of the number of elements
 *
 * plot_hash_entry(s) are stored as a linked-list
 * in strcmp order of keys
 */
typedef struct plot_hash {
    struct plot_hash_entry *head;
    int n_elems;
} plot_hash;

/* a plot_hash_entry is an entry inside a plot_hash
 * consisting of a key and value pair
 * and a pointer to the next item in the list
 * the last element will have null as it's next value
 *
 * plot_hash_entry(s) are stored as a linked-list
 * in strcmp order of keys
 */
typedef struct plot_hash_entry {
    const struct plot_symbol * key;
    const struct plot_value *value;
    struct plot_hash_entry *next;
} plot_hash_entry;

/* create a new hash
 * allocate a new hash using calloc
 *
 * a pointer to the new hash is returned
 * or 0 if an error was encountered
 */
plot_hash * plot_hash_init(void);

/* destroy hash
 * frees all plot_hash_entry(s) and then finally the plot_hash
 */
void plot_hash_cleanup(plot_hash *hash);

/* get value stored at key within hash
 * return value for key or 0 if key was not found
 */
const struct plot_value * plot_hash_get(plot_hash *hash, const struct plot_symbol * key);

/* set value to hash under key
 * keys must be unique within the hash
 * and keys cannot be overwritten once set (no mutation)
 *
 * this hash will NOT make copies of either key or value
 * therefore both key and value must not be freed until
 *  cleanup is called on this hash
 *
 * returns 1 on success, 0 on error
 */
int plot_hash_insert(plot_hash *hash, const struct plot_symbol * key, const struct plot_value *value);

/* set value in hash under key
 * keys must be unique within the hash
 * will overwrite any existing value
 *
 * returns 1 on success, 0 on error
 */
int plot_hash_set(plot_hash *hash, const struct plot_symbol * key, const struct plot_value *value);

#endif

