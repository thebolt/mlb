// SPDX-License-Identifier: MIT

#ifndef LITE_BROKER_SUBTREE_H
#define LITE_BROKER_SUBTREE_H

#include <stdint.h>

struct subtree_node;

struct subtree_node_list {
  struct subtree_node *nodes;
  uint16_t allocated;
  uint16_t used;
};

/**
 * A node in our subscription tree
 */
struct subtree_node {
  /** Path name component */
  char *path_component;

  /** All sub-nodes of this node */
  struct subtree_node_list children;

  /** Mask of all subscribed clients */
  uint64_t subscribed_mask;

  /** Mask of all wildcard subscribed clients */
  uint64_t wildcard_subscribed_mask;
};

struct subscription_tree {
  struct subtree_node root;
};

int subscription_tree_init(struct subscription_tree *tree);
void subscription_tree_free(struct subscription_tree *tree);
int subscription_tree_subscribe(struct subscription_tree *tree, const char *path, uint8_t subscriber);
int subscription_tree_collect_subscribers(struct subscription_tree *tree, const char *path, uint64_t *subscribers);

int subtree_node_init(struct subtree_node *node, const char *path_component);
void subtree_node_free(struct subtree_node *node);
struct subtree_node *subtree_node_find_by_path(struct subtree_node *node, const char *path_component);

int subtree_node_list_add_node(struct subtree_node_list *list, const char *path_component,
                               struct subtree_node **new_node);

#endif  // LITE_BROKER_SUBTREE_H