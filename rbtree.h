#ifndef _RB_TREE_H_
#define _RB_TREE_H_

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


#endif
