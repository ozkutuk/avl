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

#define ARR_SIZE 16

struct int_arr {
    int *arr;
    int last;
    int size;
};

int cmp_int(const void *lhs, const void *rhs)
{
    return *((const int *)lhs) - *((const int *)rhs);
}

void free_int(void *p)
{
    free(p);
}

int print_int(void *ptr, void *it_data)
{
    printf("%d\n", *(int *) ptr);
    return 0;
}

int sum_int(void *ptr, void *it_data)
{
    *(int *)it_data += *(int *)ptr;
    return 0;
}

int sum_int_lt_5(void *ptr, void *it_data)
{
    if (*(int *)ptr < 5) {
        *(int *)it_data += *(int *)ptr;
        return 0;
    } else {
        return 1;
    }
}

int mk_array(void *ptr, void *it_data)
{
    struct int_arr *elements = it_data;
    if (elements->last >= elements->size - 1) {
        elements->arr = realloc(elements->arr, elements->size * 2 * sizeof(int));
        elements->size = elements->size * 2;
    }
    elements->arr[++elements->last] = *(int *)ptr;
    return 0;
}

int main(void)
{
    struct bstree_node *root = NULL;
    struct bstree_ops ops = { cmp_int, free_int };
    int *arr[ARR_SIZE];
    int i, sum = 0;
    struct int_arr elements = { malloc(sizeof(int)), -1, 1 };
    for (i = 0; i < ARR_SIZE; i++) {
        arr[i] = malloc(sizeof(int));
        *arr[i] = rand() % 10;
        printf("*arr[%d] = %d\n", i, *arr[i]);
    }
    putchar('\n');
    for (i = 0; i < ARR_SIZE; i++) {
        root = bstree_insert(root, &ops, arr[i]);
        arr[i] = NULL;
    }

    /* inorder print */
    bstree_traverse_inorder(root, NULL, print_int);
    putchar('\n');

    /* preorder print */
    bstree_traverse_preorder(root, NULL, print_int);
    putchar('\n');

    /* postorder print */
    bstree_traverse_preorder(root, NULL, print_int);
    putchar('\n');

    bstree_traverse_inorder(root, &sum, sum_int);
    printf("\nheight = %d\n", root->height);
    printf("\nsum = %d\n", sum);
    sum = 0;
    bstree_traverse_inorder(root, &sum, sum_int_lt_5);
    printf("\nsum lt 10 = %d\n", sum);
    bstree_traverse_inorder(root, &elements, mk_array);
    int n = 3;
    printf("count of 3 = %d\n", bstree_count(root, &ops, &n));
    for (i = 0; i <= elements.last; i++) {
        printf("elements.arr[%d] = %d\n", i, elements.arr[i]);
    }
    elements.last = -1;
    bstree_traverse_inorder_cnt(root, &elements, mk_array);
    putchar('\n');
    for (i = 0; i <= elements.last; i++) {
        printf("elements.arr[%d] = %d\n", i, elements.arr[i]);
    }
    free(elements.arr);

    puts("\nTesting remove function");
    bstree_traverse_inorder(root, NULL, print_int);
    putchar('\n');

    n = 5;
    root = bstree_remove(root, &ops, &n);
    bstree_traverse_inorder(root, NULL, print_int);
    printf("Removed %d\n", n);

    n = 65536;
    root = bstree_remove(root, &ops, &n);
    bstree_traverse_inorder(root, NULL, print_int);
    printf("Removed %d\n", n);

    n = 3;
    root = bstree_remove(root, &ops, &n);
    bstree_traverse_inorder(root, NULL, print_int);
    printf("Removed %d\n", n);

    bstree_destroy(root, &ops);
    return 0;
}
