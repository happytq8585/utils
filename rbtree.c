#include <rbtree.h>
#include <stdio.h>

static rbtree_t nil = {
    .left  = &nil,
    .right = &nil,
    .parent= &nil,
    .color = RB_B
};

static rbroot_t *create_root(rb_callback cmp) {
    rbroot_t *p = myalloc(sizeof(rbroot_t));
    if (p != NULL) {
        p->cmp = cmp;
        p->ptr = NULL;
        return p;
    }
    return NULL;
}

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

static void left_rotate(rbroot_t* root, rbtree_t* x)
{
    rbtree_t *y = x->right;
    x->right    = y->left;
    y->left->parent = x;
    y->parent   = x->parent;

    if (x->parent == &nil)
    {
        root->ptr = y;
    }
    else if (x == x->parent->left)
    {
        x->parent->left = y;
    }
    else
    {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}
static void right_rotate(rbroot_t *root, rbtree_t *x)
{
    rbtree_t *y = x->left;
    x->left     = y->right;
    y->right->parent = x;
    y->parent   = x->parent;

    if (x->parent == &nil)
    {
        root->ptr = y;
    }
    else if (x == x->parent->left)
    {
        x->parent->left = y;
    }
    else
    {
        x->parent->right = y;
    }
    y->parent = x->parent;
    x->parent = y;
}
static void rb_insert_fixup(rbroot_t* t, rbtree_t* z)
{
    while (z->parent->color == RB_R) {
        if (z->parent == z->parent->parent->left)
        {
            rbtree_t *y = z->parent->parent->right;
            if (y->color == RB_R)
            {
                z->parent->color = RB_B;
                y->color = RB_B;
                z->parent->parent->color = RB_R;
                z = z->parent->parent;
            }
            else if (z == z->parent->right)
            {
                z = z->parent;
                left_rotate(t, z);
                z->parent->color = RB_B;
                z->parent->parent->color = RB_R;
                right_rotate(t, z->parent->parent);
            }
            else
            {
                z = z->parent;
                right_rotate(t, z);
                z->parent->color = RB_B;
                z->parent->parent->color = RB_R;
                left_rotate(t, z->parent->parent);
            }
        }
    }
    t->ptr->color = RB_B;
}

static int rb_insert(rbroot_t* t, rbtree_t *z)
{
    if (t->ptr == NULL) {
        t->ptr = z;
        t->ptr->color = RB_B;
        t->size++;
        return 0;
    }

    rb_callback cmp = t->cmp;
    rbtree_t *y = &nil;
    rbtree_t *x = t->ptr;
   
    int a = 1;
    while (x != &nil) {
        y = x;
        if (cmp(z, x) < 0) {
            x = x->left;
            a = -1;
        }
        else
        {
            x = x->right;
            a = 1;
        }
    }
    z->parent = y;
    if (a < 0)
    {
        y->left = z;
    }
    else
    {
        y->right = z;
    }
    rb_insert_fixup(t, z);
    return 0;
}

typedef struct node
{
    int data;
    rbtree_t rbt;
} node_t;

static int cmp(const rbtree_t* a, const rbtree_t* b)
{
    node_t *pa = (node_t*)((char*)a - offsetof(struct node, rbt));
    node_t *pb = (node_t*)((char*)b - offsetof(struct node, rbt));
    if (pa->data < pb->data) {
        return -1;
    }
    else if (pa->data > pb->data) {
        return 1;
    }
    return 0;
}
node_t *create_data_node(int data) {
    node_t * p = myalloc(sizeof(node_t));
    if (p != NULL) {
        p->data = data;
        init_node(&p->rbt, &nil, &nil, &nil, RB_B);
    }
    return p;
}

void travel(const rbtree_t * p)
{
    while (p != NULL) {
        travel(p->left);
        node_t * ptr = (node_t*)((char*)p - offsetof(struct node, rbt));
        printf("%d\n", ptr->data);
        travel(p->right);
    }
}
int main()
{
    int a[] = {1,3, 4, 5, 2, 9, 6, 12, 7, 9, 8};
    int n = sizeof(a)/sizeof(a[0]), i;
    rbroot_t *root = create_root(cmp);
    for (i = 0; i < n; ++i) {
        node_t *p = create_data_node(a[i]);
        if (p == NULL) {
            break;
        }
        init_node(&p->rbt, &nil, &nil, &nil, RB_R);
        rb_insert(root, &p->rbt);
    }
    rbtree_t * p = root->ptr;
    travel(p);
    return 0;
}
