/*
    Generic AVL tree implementation in C
    Copyright (C) 2017 Yagmur Oymak

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "bstree.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define MAX_IMBALANCE 1

/* Structs for internal usage
 */

struct bstree_node {
    void *object;
    struct bstree_node *left;
    struct bstree_node *right;
    int count;
    int height;
};

struct bstree_ops {
    int (*compare_object)(const void *lhs, const void *rhs);
    /* If the user supplies a function to free the objects, then we know that
     * we take the ownership of the resources, and we should eventually
     * free every pointer we are given if we are still holding it in the end.
     * If the freeing function is NULL, we just keep the pointers to the
     * objects and never free them, that means the user manages the lifetime.
     */
    void (*free_object)(void *object);
};

/* Internal helper functions
 */

static int int_max_(int a, int b)
{
    return a > b ? a : b;
}

static int height_(struct bstree_node *root)
{
    return root ? root->height : -1;
}

/* Make a node that is a valid tree consisting of one node, only the root.
 */
static struct bstree_node *mknode_(void *object)
{
    struct bstree_node *root = malloc(sizeof *root);
    root->object = object;
    root->left = NULL;
    root->right = NULL;
    root->count = 1;
    root->height = 0;
    return root;
}

static struct bstree_node *rotate_with_left_(struct bstree_node *root)
{
    struct bstree_node *newroot = root->left;
    root->left = newroot->right;
    newroot->right = root;
    root->height = int_max_(height_(root->left), height_(root->right)) + 1;
    newroot->height = int_max_(height_(newroot->left),
            height_(newroot->right)) + 1;
    return newroot;
}

static struct bstree_node *rotate_with_right_(struct bstree_node *root)
{
    struct bstree_node *newroot = root->right;
    root->right = newroot->left;
    newroot->left = root;
    root->height = int_max_(height_(root->left), height_(root->right)) + 1;
    newroot->height = int_max_(height_(newroot->left),
            height_(newroot->right)) + 1;
    return newroot;
}

static struct bstree_node *double_with_left_(struct bstree_node *root)
{
    root->left = rotate_with_right_(root->left);
    root = rotate_with_left_(root);
    return root;
}

static struct bstree_node *double_with_right_(struct bstree_node *root)
{
    root->right = rotate_with_left_(root->right);
    root = rotate_with_right_(root);
    return root;
}

/* Assume the given tree is balanced or have an imbalance of 2.
 * We allow a maximum imbalance of 1, and we maintain it as long as the
 * tree exists. Therefore, a valid tree generated by us will either be balanced,
 * or imbalanced by 2 because of a recent insertion (or deletion).
 * If the latter is the case, this function restores the balance.
 */
static struct bstree_node *balance_(struct bstree_node *root)
{
    if (!root) {
        return NULL;
    }
    if (height_(root->left) - height_(root->right) > MAX_IMBALANCE) {
        if (height_(root->left->left) > height_(root->left->right)) {
            root = rotate_with_left_(root);
        } else {
            root = double_with_left_(root);
        }
    } else if (height_(root->right) - height_(root->left) > MAX_IMBALANCE) {
        if (height_(root->right->right) > height_(root->right->left)) {
            root = rotate_with_right_(root);
        } else {
            root = double_with_right_(root);
        }
    }
    root->height = int_max_(height_(root->left), height_(root->right)) + 1;
    return root;
}

static struct bstree_node *get_min_(struct bstree_node *root)
{
    while (root && root->left) {
        root = root->left;
    }
    return root;
}

static struct bstree_node *insert_(struct bstree_node *root,
        const struct bstree_ops *ops, void *object)
{
    if (!root) {
        return mknode_(object);
    }
    if (ops->compare_object(object, root->object) < 0) {
        root->left = insert_(root->left, ops, object);
        return balance_(root);
    }
    if (ops->compare_object(object, root->object) > 0) {
        root->right = insert_(root->right, ops, object);
        return balance_(root);
    }
    /* Inserting equal key. We are not going to hold the given pointer.
     * If it is us who manages the lifetime (the ops->free_object != NULL),
     * we should free it.
     */
    root->count++;
    if (ops->free_object) {
        ops->free_object(object);
    }
    return balance_(root);
}

static struct bstree_node *replace_(struct bstree_node *root,
        const struct bstree_ops *ops, void *object)
{
    if (!root) {
        return mknode_(object);
    }
    if (ops->compare_object(object, root->object) < 0) {
        root->left = replace_(root->left, ops, object);
        return balance_(root);
    }
    if (ops->compare_object(object, root->object) > 0) {
        root->right = replace_(root->right, ops, object);
        return balance_(root);
    }
    /* Inserting equal key. We are going to replace the existing object with
     * the new one. We shall free the object if we have to (ops->free_object != NULL),
     * then replace the pointer in the node.
     */
    if (ops->free_object) {
        ops->free_object(root->object);
    }
    root->object = object;
    return balance_(root);
}

static void destroy_(struct bstree_node *root, const struct bstree_ops *ops)
{
    if (!root) {
        return;
    }
    destroy_(root->left, ops);
    destroy_(root->right, ops);
    if (ops->free_object) {
        ops->free_object(root->object);
    }
    free(root);
}

static int traverse_inorder_(const struct bstree_node *root, void *it_data,
        int (*operation)(void *object, void *it_data))
{
    return
        root &&
        (traverse_inorder_(root->left, it_data, operation) ||
        operation(root->object, it_data) ||
        traverse_inorder_(root->right, it_data, operation));
}

static int traverse_inorder_cnt_(const struct bstree_node *root,
        void *it_data,
        int (*operation)(void *object, void *it_data))
{
    int i;
    if (!root) {
        return 0;
    }
    if (traverse_inorder_cnt_(root->left, it_data, operation)) {
        return 1;
    }
    for (i = 0; i < root->count; i++) {
        if (operation(root->object, it_data)) {
            return 1;
        }
    }
    if (traverse_inorder_cnt_(root->right, it_data, operation)) {
        return 1;
    }
    return 0;
}

static int count_(const struct bstree_node *root,
        const struct bstree_ops *ops, const void *object)
{
    if (!root) {
        return 0;
    }
    if (ops->compare_object(object, root->object) < 0) {
        return count_(root->left, ops, object);
    }
    if (ops->compare_object(object, root->object) > 0) {
        return count_(root->right, ops, object);
    }
    return root->count;
}

static void *search_(const struct bstree_node *root,
        const struct bstree_ops *ops, const void *key)
{
    if (!root) {
        return NULL;
    }
    if (ops->compare_object(key, root->object) < 0) {
        return search_(root->left, ops, key);
    }
    if (ops->compare_object(key, root->object) > 0) {
        return search_(root->right, ops, key);
    }
    return root->object;
}

static struct bstree_node *remove_(struct bstree_node *root,
        const struct bstree_ops *ops, const void *key)
{
    if (!root) {
        return NULL;
    }
    if (ops->compare_object(key, root->object) < 0) {
         root->left = remove_(root->left, ops, key);
         return balance_(root);
    }
    if (ops->compare_object(key, root->object) > 0) {
        root->right = remove_(root->right, ops, key);
        return balance_(root);
    }
    /* Found the node to be deleted */
    if (!root->left || !root->right) {
        struct bstree_node *tmp = root->left ? root->left : root->right;
        if (ops->free_object) {
            ops->free_object(root->object);
        }
        free(root);
        return tmp;
    }
    /* Node to be deleted has two children */
    struct bstree_node *right_min = get_min_(root->right);
    /* As we get the pointer held at right_min and put it inside root, we shall
     * remove right_min without free'ing the object it holds.
     * For this reason, we call the remove function with a NULL free_object.
     */
    struct bstree_ops tmp_ops = *ops;
    tmp_ops.free_object = NULL;
    if (ops->free_object) {
        ops->free_object(root->object);
    }
    root->object = right_min->object;
    root->right = remove_(root->right, &tmp_ops, right_min->object);
    return balance_(root);
}

static int size_(struct bstree_node *root)
{
    if (!root) {
        return 0;
    }
    return size_(root->left) + size_(root->right) + 1;
}

/* Interface functions
 */

struct bstree *bstree_new(
        int (*compare_object)(const void *lhs, const void *rhs),
        void (*free_object)(void *object))
{
    struct bstree *tree;
    tree = malloc(sizeof(*tree));
    tree->root = NULL;
    tree->ops = malloc(sizeof(*tree->ops));
    tree->ops->compare_object = compare_object;
    tree->ops->free_object = free_object;
    return tree;
}

void bstree_destroy(struct bstree *tree)
{
    destroy_(tree->root, tree->ops);
    free(tree->ops);
    free(tree);
}

void bstree_insert(struct bstree *tree, void *object)
{
    tree->root = insert_(tree->root, tree->ops, object);
}

void bstree_replace(struct bstree *tree, void *object)
{
    tree->root = replace_(tree->root, tree->ops, object);
}

int bstree_traverse_inorder(const struct bstree *tree, void *it_data,
        int (*operation)(void *object, void *it_data))
{
    return traverse_inorder_(tree->root, it_data, operation);
}

int bstree_traverse_inorder_cnt(const struct bstree *tree, void *it_data,
        int (*operation)(void *object, void *it_data))
{
    return traverse_inorder_cnt_(tree->root, it_data, operation);
}

int bstree_count(const struct bstree *tree, const void *key)
{
    return count_(tree->root, tree->ops, key);
}

void *bstree_search(const struct bstree *tree, const void *key)
{
    return search_(tree->root, tree->ops, key);
}

void bstree_remove(struct bstree *tree, const void *key)
{
    tree->root = remove_(tree->root, tree->ops, key);
}

int bstree_size(struct bstree *tree)
{
    return size_(tree->root);
}

int bstree_height(struct bstree *tree)
{
    return height_(tree->root);
}
