#ifndef _RB_TREE_H_
#define _RB_TREE_H_

#include <stddef.h>
#include <stdlib.h>

#define RB_R  1
#define RB_B  2

#define myalloc(ptr) malloc(ptr)


typedef struct rbtree
{
   struct rbtree *left;
   struct rbtree *right;
   struct rbtree *parent;
   short color;
} rbtree_t;

typedef int (*rb_callback)(const rbtree_t* a, const rbtree_t* b);

typedef struct rbroot
{
    size_t size;
    rb_callback  cmp;
    rbtree_t *ptr;
} rbroot_t;


#endif
