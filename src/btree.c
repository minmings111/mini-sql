#include "btree.h"

#include <stdlib.h>

static BTreeNode *btree_create_node(int is_leaf);
static void btree_free_node(BTreeNode *node);
static int btree_search_node(const BTreeNode *node, int key, long *value_out);
static int btree_split_child(BTreeNode *parent, int index);
static int btree_insert_nonfull(BTreeNode *node, int key, long value);

void btree_init(BTreeIndex *tree) {
    if (tree == NULL) {
        return;
    }

    tree->root = NULL;
}

void btree_free(BTreeIndex *tree) {
    if (tree == NULL) {
        return;
    }

    btree_free_node(tree->root);
    tree->root = NULL;
}

int btree_search(const BTreeIndex *tree, int key, long *value_out) {
    if (tree == NULL || tree->root == NULL) {
        return 0;
    }

    return btree_search_node(tree->root, key, value_out);
}

int btree_insert(BTreeIndex *tree, int key, long value) {
    BTreeNode *root;
    BTreeNode *new_root;
    int status;

    if (tree == NULL) {
        return -1;
    }

    if (tree->root == NULL) {
        tree->root = btree_create_node(1);
        if (tree->root == NULL) {
            return -1;
        }
        tree->root->keys[0] = key;
        tree->root->values[0] = value;
        tree->root->key_count = 1;
        return 0;
    }

    if (btree_search(tree, key, NULL)) {
        return 1;
    }

    root = tree->root;
    if (root->key_count == BTREE_MAX_KEYS) {
        new_root = btree_create_node(0);
        if (new_root == NULL) {
            return -1;
        }

        new_root->children[0] = root;
        tree->root = new_root;

        status = btree_split_child(new_root, 0);
        if (status != 0) {
            btree_free_node(new_root);
            tree->root = root;
            return -1;
        }

        return btree_insert_nonfull(new_root, key, value);
    }

    return btree_insert_nonfull(root, key, value);
}

static BTreeNode *btree_create_node(int is_leaf) {
    BTreeNode *node;

    node = (BTreeNode *) calloc(1, sizeof(BTreeNode));
    if (node == NULL) {
        return NULL;
    }

    node->is_leaf = is_leaf;
    return node;
}

static void btree_free_node(BTreeNode *node) {
    int i;

    if (node == NULL) {
        return;
    }

    if (!node->is_leaf) {
        for (i = 0; i <= node->key_count; i++) {
            btree_free_node(node->children[i]);
        }
    }

    free(node);
}

static int btree_search_node(const BTreeNode *node, int key, long *value_out) {
    int i = 0;

    while (i < node->key_count && key > node->keys[i]) {
        i++;
    }

    if (i < node->key_count && key == node->keys[i]) {
        if (value_out != NULL) {
            *value_out = node->values[i];
        }
        return 1;
    }

    if (node->is_leaf) {
        return 0;
    }

    return btree_search_node(node->children[i], key, value_out);
}

static int btree_split_child(BTreeNode *parent, int index) {
    BTreeNode *child = parent->children[index];
    BTreeNode *sibling;
    int i;

    sibling = btree_create_node(child->is_leaf);
    if (sibling == NULL) {
        return -1;
    }

    sibling->key_count = BTREE_MIN_DEGREE - 1;

    for (i = 0; i < BTREE_MIN_DEGREE - 1; i++) {
        sibling->keys[i] = child->keys[i + BTREE_MIN_DEGREE];
        sibling->values[i] = child->values[i + BTREE_MIN_DEGREE];
    }

    if (!child->is_leaf) {
        for (i = 0; i < BTREE_MIN_DEGREE; i++) {
            sibling->children[i] = child->children[i + BTREE_MIN_DEGREE];
            child->children[i + BTREE_MIN_DEGREE] = NULL;
        }
    }

    child->key_count = BTREE_MIN_DEGREE - 1;

    for (i = parent->key_count; i >= index + 1; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = sibling;

    for (i = parent->key_count - 1; i >= index; i--) {
        parent->keys[i + 1] = parent->keys[i];
        parent->values[i + 1] = parent->values[i];
    }

    parent->keys[index] = child->keys[BTREE_MIN_DEGREE - 1];
    parent->values[index] = child->values[BTREE_MIN_DEGREE - 1];
    parent->key_count++;

    return 0;
}

static int btree_insert_nonfull(BTreeNode *node, int key, long value) {
    int i = node->key_count - 1;
    int status;

    if (node->is_leaf) {
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }

        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->key_count++;
        return 0;
    }

    while (i >= 0 && key < node->keys[i]) {
        i--;
    }
    i++;

    if (node->children[i]->key_count == BTREE_MAX_KEYS) {
        status = btree_split_child(node, i);
        if (status != 0) {
            return -1;
        }

        if (key > node->keys[i]) {
            i++;
        }
    }

    return btree_insert_nonfull(node->children[i], key, value);
}
