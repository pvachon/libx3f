#ifndef __INCLUDE_X3F_METATREE_H__
#define __INCLUDE_X3F_METATREE_H__

#include <x3f.h>

struct x3f_metatree_node;

/* Functions that must be provided to operate on a metatree */
/*
 * Predicate. Returns < 0 if less than, > 0 if greater than, or 0 if equal
 */
typedef int (*x3f_metatree_predicate)(const void *left, const void *right);
typedef void (*x3f_metatree_release_node)(void *key, void *value);

struct x3f_metatree;

/*
 * Create a new metatree.
 * Requires: a reference ot a pointer to a metatree,
 *           a predicate comparison function (determines l/r),
 *           a function to be used to free a node.
 */
X3F_STATUS x3f_create_metatree(struct x3f_metatree **tree,
                               x3f_metatree_predicate pred,
                               x3f_metatree_release_node node_release);

/*
 * Frees memory used by a metatree
 */
X3F_STATUS x3f_release_metatree(struct x3f_metatree *tree);

/*
 * Find a node in the metatree.
 */
X3F_STATUS x3f_find_node(struct x3f_metatree *tree,
                         const void *key,
                         void **value);

X3F_STATUS x3f_insert_node(struct x3f_metatree *tree,
                           void *key,
                           void *value);

#endif /* __INCLUDE_X3F_METATREE_H__ */

