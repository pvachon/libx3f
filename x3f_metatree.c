#include <x3f.h>
#include <x3f_priv.h>
#include <x3f_metatree.h>

#ifdef _DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#define MT_BLACK    0
#define MT_RED      1

struct x3f_metatree_node {
    void *key;
    void *data;
    int color;
    struct x3f_metatree_node *parent;
    struct x3f_metatree_node *next;
};

struct x3f_metatree {
    x3f_metatree_predicate pred;
    x3f_metatree_release_node release;

    struct x3f_metatree_node *root;
};

#define MT_OUT(x, ...) \
    X3F_PRINT(x, ##__VA_ARGS__)

#define MT_TRACE(x, ...) \
    X3F_TRACE("metatree: " x "\n", ##__VA_ARGS__)


#if 0
#define MT_GRANDPARENT(x, gp) \
    do { \
    if (((x) != NULL) && ((x)->parent != NULL)) \
        gp = x->parent->parent; \
    else gp = NULL; \
    } while (0)

#define MT_UNCLE(x, u) \
    do { \
        struct x3f_metatree_node *g; \
        MT_GRANDPARENT(x, _g); \
        if (_g == NULL) u = NULL; \
        if (n->parent == _g->left) u = _g->right; \
        else u = _g->left \
        \
    } while (0)
#endif

X3F_STATUS x3f_create_metatree(struct x3f_metatree **tree,
                               x3f_metatree_predicate pred,
                               x3f_metatree_release_node node_release)
{
    struct x3f_metatree *tree_out = NULL;

    X3F_ASSERT_ARG(tree);
    X3F_ASSERT_ARG(pred);
    X3F_ASSERT_ARG(node_release);

    tree_out = (struct x3f_metatree *)calloc(1, sizeof(struct x3f_metatree));

    if (tree_out == NULL) {
        return X3F_NO_MEMORY;
    }

    tree_out->pred = pred;
    tree_out->release = node_release;

    *tree = tree_out;

    return X3F_SUCCESS;
}

X3F_STATUS x3f_release_metatree(struct x3f_metatree *tree)
{
    struct x3f_metatree_node *n, *nn;
    X3F_ASSERT_ARG(tree);

    n = tree->root;

    while (n != NULL) {
        tree->release(n->key, n->data);
        nn = n->next;
        memset(n, 0, sizeof(struct x3f_metatree_node));
        free(n);
        n = nn;
    }

    return X3F_SUCCESS;
}

X3F_STATUS x3f_find_node(struct x3f_metatree *tree,
                         const void *key,
                         void **value)
{
    struct x3f_metatree_node *n;
    X3F_ASSERT_ARG(tree);
    X3F_ASSERT_ARG(key);
    X3F_ASSERT_ARG(value);

    *value = NULL;

    n = tree->root;

    while (n != NULL) {
        if (tree->pred(key, n->key) == 0) {
            MT_TRACE("Found value!");
            *value = n->data;
            return X3F_SUCCESS;
        }
        n = n->next;
    }

    return X3F_NOT_FOUND;
}

static struct x3f_metatree_node *x3f_metatree_new_node()
{
    return calloc(1, sizeof(struct x3f_metatree_node));
}

static void x3f_append_node(struct x3f_metatree *tree,
                            struct x3f_metatree_node *node)
{
    node->next = NULL;
    if (tree->root == NULL) {
        tree->root = node;
        node->parent = NULL;
    } else {
        struct x3f_metatree_node *n = tree->root;

        while (n->next != NULL) {
            n = n->next;
        }

        n->next = node;
        node->parent = n;
    }
}

X3F_STATUS x3f_insert_node(struct x3f_metatree *tree,
                           void *key,
                           void *value)
{
    struct x3f_metatree_node *new = NULL;

    X3F_ASSERT_ARG(tree);
    X3F_ASSERT_ARG(key);
    X3F_ASSERT_ARG(value);

    new = x3f_metatree_new_node();

    if (new == NULL) {
        return X3F_NO_MEMORY;
    }

    new->key = key;
    new->data = value;

    x3f_append_node(tree, new);

    return X3F_SUCCESS;
}

