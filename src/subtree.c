// SPDX-License-Identifier: MIT

#include "lite_broker/subtree.h"

#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#define MLB_MAX_WILDCARDS 8

static int strcmpend(const char *s1, const char *s2, const char *s2end) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  const unsigned char *p2end = (const unsigned char *)s2end;

  while (*p1 && *p1 == *p2 && p2 != p2end) {
    p1++;
    p2++;
  }

  if (p2 == p2end) {
    return 0;
  }

  return (*p1 > *p2) - (*p2 > *p1);
}

static bool check_valid_wildcards(const char *path, bool allow_wildcards) {
  uint32_t count_onelevel = 0;

  while (*path) {
    if (*path == '+') {
      count_onelevel++;
      if (!allow_wildcards || count_onelevel > MLB_MAX_WILDCARDS) {
        return false;
      }
    }

    if (*path == '#') {
      if (!allow_wildcards || *(path + 1) != '\0') {
        return false;
      }
    }
    path++;
  }

  return true;
}

static int subtree_node_init(struct subtree_node *node, char *path_component) {
  memset(node, 0, sizeof(struct subtree_node));

  node->path_component = path_component;

  return 0;
}

static void subtree_node_free(struct subtree_node *node) {
  free(node->path_component);

  for (uint32_t i = 0; i < node->children.used; i++) {
    subtree_node_free(&node->children.nodes[i]);
  }

  free(node->children.nodes);
}

static struct subtree_node *subtree_node_find_by_path(struct subtree_node *node, const char *path_component,
                                                      const char *delim) {
  for (uint32_t i = 0; i < node->children.used; i++) {
    struct subtree_node *child = &node->children.nodes[i];
    if (strcmpend(child->path_component, path_component, delim) == 0) {
      return child;
    }
  }

  return NULL;
}

static int subtree_node_list_add_node(struct subtree_node_list *list, const char *path_component, const char *delim,
                                      struct subtree_node **new_node) {
  uint16_t new_allocated;
  struct subtree_node *new_list;

  if (list->used == list->allocated) {
    /* Need allocation */
    new_allocated = list->allocated + 4;
    new_list = realloc(list->nodes, new_allocated * sizeof(struct subtree_node));
    if (!new_list) {
      return -ENOMEM;
    }

    list->allocated = new_allocated;
    list->nodes = new_list;
  }

  struct subtree_node *node = &list->nodes[list->used];
  list->used++;

  size_t n = delim ? delim - path_component : strlen(path_component);

  subtree_node_init(node, strndup(path_component, n));

  if (new_node) {
    *new_node = node;
  }

  return 0;
}

struct subtree_travesal_entry {
  struct subtree_node *node;
  const char *saveptr;
  uint16_t index;
};

static int subtree_ctx_prepare(struct subscription_tree *tree, size_t num_entries) {
  if (tree->traverse_ctx.stack_allocated < num_entries) {
    errno = 0;
    tree->traverse_ctx.stack = realloc(tree->traverse_ctx.stack, num_entries * sizeof(struct subtree_travesal_entry));
    if (errno != 0) {
      return -errno;
    }

    tree->traverse_ctx.stack_allocated = num_entries;
  }

  tree->traverse_ctx.stack_used = 0;

  return 0;
}

static int subtree_ctx_push(struct subscription_tree *tree, struct subtree_node *node, const char *saveptr,
                            uint16_t index) {
  struct subscription_tree_traversal_ctx *ctx = &tree->traverse_ctx;

  if (ctx->stack_used >= ctx->stack_allocated) {
    return -ENOMEM;
  }

  size_t idx = ctx->stack_used;
  ctx->stack[idx].node = node;
  ctx->stack[idx].saveptr = saveptr;
  ctx->stack[idx].index = index;
  ctx->stack_used++;

  return 0;
}

static int subtree_ctx_pop(struct subscription_tree *tree, struct subtree_node **node, const char **saveptr,
                           uint16_t *index) {
  struct subscription_tree_traversal_ctx *ctx = &tree->traverse_ctx;

  if (ctx->stack_used == 0) {
    return 0;
  }

  ctx->stack_used--;
  size_t idx = ctx->stack_used;

  if (node) {
    *node = ctx->stack[idx].node;
  }
  if (saveptr) {
    *saveptr = ctx->stack[idx].saveptr;
  }
  if (index) {
    *index = ctx->stack[idx].index;
  }

  return ctx->stack_used + 1;
}

enum subtree_traversal_type {
  SUBTREE_TRAVERSAL_PREORDER = 0,
  SUBTREE_TRAVERSAL_POSTORDER,
};

typedef void (*subtree_dfs_cb)(struct subtree_node *node, void *cb_data);

static int subtree_traverse_df(struct subscription_tree *tree, subtree_dfs_cb cb, void *cb_data,
                               enum subtree_traversal_type type) {
  int rc = 0;

  rc = subtree_ctx_prepare(tree, tree->max_tree_depth);
  if (rc < 0) {
    return rc;
  }

  /* Perform a depth first traversal */
  struct subtree_node *current_node = &tree->root;
  uint16_t node_idx = 0;

  while (true) {
    if (node_idx == 0 && type == SUBTREE_TRAVERSAL_PREORDER) {
      cb(current_node, cb_data);
    }

    if (current_node->children.used > 0 && node_idx < current_node->children.used) {
      /* More children to process, push current and select node_idx child as next */
      rc = subtree_ctx_push(tree, current_node, NULL, node_idx + 1);
      if (rc < 0) {
        goto out;
      }

      current_node = &current_node->children.nodes[node_idx];
      node_idx = 0;
      continue;
    } else if (node_idx == current_node->children.used && type == SUBTREE_TRAVERSAL_POSTORDER) {
      cb(current_node, cb_data);
    }

    rc = subtree_ctx_pop(tree, &current_node, NULL, &node_idx);
    if (rc < 0) {
      goto out;
    } else if (rc == 0) {
      break;
    }
  }

out:
  return rc;
}

int subscription_tree_init(struct subscription_tree *tree) {
  int rc;

  if ((rc = subtree_node_init(&tree->root, NULL)) != 0) {
    return rc;
  }

  tree->max_tree_depth = 0;
  memset(&tree->traverse_ctx, 0, sizeof(tree->traverse_ctx));

  return 0;
}

void subscription_tree_free(struct subscription_tree *tree) {
  subtree_node_free(&tree->root);

  free(tree->traverse_ctx.stack);
}

int subscription_tree_subscribe(struct subscription_tree *tree, const char *path, uint8_t subscriber) {
  int rc = 0;
  uint64_t subscriber_mask = (1 << subscriber);

  if (subscriber >= 64 || !path || strlen(path) == 0) {
    return -EINVAL;
  }

  if (*path == '/') {
    path++;
  }

  if (!check_valid_wildcards(path, true)) {
    return -EINVAL;
  }

  const char *rest_path = path;
  const char *next_path_part;
  const char *delim_ptr;
  uint16_t depth = 0;

  struct subtree_node *current_node = &tree->root;

  while (rest_path != NULL && *rest_path != '\0') {
    delim_ptr = next_path_part = strchr(rest_path, '/');
    if (next_path_part) {
      next_path_part++;
    }

    if (strcmp("#", rest_path) == 0) {
      current_node->wildcard_subscribed_mask |= subscriber_mask;
      goto out;
    }

    struct subtree_node *child = subtree_node_find_by_path(current_node, rest_path, delim_ptr);
    if (!child) {
      rc = subtree_node_list_add_node(&current_node->children, rest_path, delim_ptr, &child);
      if (rc != 0) {
        goto out;
      }
    }

    current_node = child;
    rest_path = next_path_part;
    depth++;
  }

  if (depth > tree->max_tree_depth) {
    tree->max_tree_depth = depth;
  }

  if (current_node) {
    current_node->subscribed_mask |= subscriber_mask;
  }

out:
  return rc;
}

int subscription_tree_unsubscribe(struct subscription_tree *tree, const char *path, uint8_t subscriber) {
  int rc = 0;
  uint64_t unsubscriber_mask = ~(1 << subscriber);

  if (subscriber >= 64 || !path || strlen(path) == 0) {
    return -EINVAL;
  }

  if (*path == '/') {
    path++;
  }

  if (!check_valid_wildcards(path, true)) {
    return -EINVAL;
  }

  /* @TODO: Save the traversal path so we can do GC on nodes "backwards" */

  const char *rest_path = path;
  const char *next_path_part;
  const char *delim_ptr;

  struct subtree_node *current_node = &tree->root;

  while (rest_path != NULL && *rest_path != '\0') {
    delim_ptr = next_path_part = strchr(rest_path, '/');
    if (next_path_part) {
      next_path_part++;
    }

    if (strcmp("#", rest_path) == 0) {
      current_node->wildcard_subscribed_mask &= unsubscriber_mask;
      goto out;
    }

    struct subtree_node *child = subtree_node_find_by_path(current_node, rest_path, delim_ptr);
    if (!child) {
      /* Didn't find the path, so cannot be subscribed */
      rc = -ENOENT;
      goto out;
    }

    current_node = child;
    rest_path = next_path_part;
  }

  if (current_node) {
    current_node->subscribed_mask &= unsubscriber_mask;
  }

out:
  return rc;
}

static void subtree_purge_cb(struct subtree_node *node, void *cb_data) {
  uint64_t unsubscriber_mask = *(uint64_t *)cb_data;

  node->subscribed_mask &= unsubscriber_mask;
  node->wildcard_subscribed_mask &= unsubscriber_mask;
}

int subscription_tree_purge(struct subscription_tree *tree, uint8_t subscriber) {
  uint64_t unsubscriber_mask = ~(1 << subscriber);

  if (subscriber >= 64) {
    return -EINVAL;
  }

  return subtree_traverse_df(tree, subtree_purge_cb, &unsubscriber_mask, SUBTREE_TRAVERSAL_POSTORDER);
}

int subscription_tree_collect_subscribers(struct subscription_tree *tree, const char *path, uint64_t *subscribers) {
  int rc = 0;

  uint64_t submask = 0;

  const char *rest_path = path;
  const char *next_path_part;
  const char *delim_ptr;

  if (!check_valid_wildcards(path, false)) {
    return -EINVAL;
  }

  /* For collection traversal the branch factor is max 2 so stack never need more than the max depth*/
  rc = subtree_ctx_prepare(tree, tree->max_tree_depth);
  if (rc < 0) {
    return rc;
  }

  struct subtree_node *current_node = &tree->root;

  while (true) {
    while (rest_path != NULL && *rest_path != '\0') {
      delim_ptr = next_path_part = strchr(rest_path, '/');
      if (next_path_part) {
        next_path_part++;
      }

      submask |= current_node->wildcard_subscribed_mask;

      struct subtree_node *wildcard_node = subtree_node_find_by_path(current_node, "+", NULL);
      if (wildcard_node) {
        rc = subtree_ctx_push(tree, wildcard_node, next_path_part, 0);
        if (rc < 0) {
          goto out;
        }
      }

      struct subtree_node *child_node = subtree_node_find_by_path(current_node, rest_path, delim_ptr);
      if (!child_node) {
        current_node = NULL;
        break;
      }

      current_node = child_node;
      rest_path = next_path_part;
    }

    if (current_node) {
      submask |= current_node->subscribed_mask;
    }

    rc = subtree_ctx_pop(tree, &current_node, &rest_path, NULL);
    if (rc < 0) {
      goto out;
    } else if (rc == 0) {
      break;
    }
  }

  if (subscribers) {
    *subscribers = submask;
  }

out:
  return rc;
}
