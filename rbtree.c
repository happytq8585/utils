#include <rbtree.h>

static rbtree_t nil = {
    .left  = &nil,
    .right = &nil,
    .parent= &nil,
    .color = RB_B
};

static rbtree_t *create_node() {
    return myalloc(sizeof(rbtree_t));
}

static int init_node(rbtree_t* ptr, rbtree_t* l, rbtree_t* r, rbtree_t* p, short c)
{
    if (c != RB_B && c != RB_R) {
        return -1;
    }
    if (ptr == NULL) {
        return -1;
    }
    ptr->left    = l;
    ptr->right   = r;
    ptr->parent  = p;
    ptr->color   = c;
    return 0;
}

static int rb_insert(rbtree_t *t, rbtree_t *z)
{
    rbtree_t *y = &nil;
    rbtree_t *x = t;
    
    while (x != &nil) {
        y = x;
        if (cmp(z, x) < 0) {
            x = x->left;
        }
        else
        {
            x = x->right;
        }
    }
    z->parent = y;
    if (y == &nil) {
        t/
    }
}

