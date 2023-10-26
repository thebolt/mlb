// SPDX-License-Identifier: MIT

#ifndef LITE_BROKER_SUBTREE_H
#define LITE_BROKER_SUBTREE_H

#include <stdint.h>

struct subtree_node;

/**
 * List of nodes allocated from free store
 */
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

struct subtree_travesal_entry;

/**
 *
 */
struct subscription_tree_traversal_ctx {
  uint16_t stack_allocated;
  uint16_t stack_used;
  struct subtree_travesal_entry *stack;
};

/**
 * The main subscription tree
 *
 * @TODO: More docs!
 */
struct subscription_tree {
  struct subtree_node root;

  uint16_t max_tree_depth;

  struct subscription_tree_traversal_ctx traverse_ctx;
};

int subscription_tree_init(struct subscription_tree *tree);
void subscription_tree_free(struct subscription_tree *tree);
int subscription_tree_subscribe(struct subscription_tree *tree, const char *path, uint8_t subscriber);
int subscription_tree_unsubscribe(struct subscription_tree *tree, const char *path, uint8_t subscriber);
int subscription_tree_purge(struct subscription_tree *tree, uint8_t subscriber);
int subscription_tree_collect_subscribers(struct subscription_tree *tree, const char *path, uint64_t *subscribers);

#endif  // LITE_BROKER_SUBTREE_H