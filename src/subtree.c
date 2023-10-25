// SPDX-License-Identifier: MIT

#include "lite_broker/subtree.h"

#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>

static int strcmpdelim(const char *s1, const char *s2, char delim) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  const unsigned char d = (unsigned char)delim;

  while (*p1) {
    if (*p1 != *p2) {
      break;
    }
    p1++;
    p2++;
  }

  if (*p2 == d) {
    return 0;
  }

  return (*p1 > *p2) - (*p2 > *p1);
}

int subscription_tree_init(struct subscription_tree *tree) {
  int rc;

  if ((rc = subtree_node_init(&tree->root, NULL)) != 0) {
    return rc;
  }

  return 0;
}

void subscription_tree_free(struct subscription_tree *tree) { subtree_node_free(&tree->root); }

int subscription_tree_subscribe(struct subscription_tree *tree, const char *path, uint8_t subscriber) {
  int rc = 0;
  uint64_t subscriber_mask = (1 << subscriber);

  if (subscriber >= 64 || !path || strlen(path) == 0) {
    return -EINVAL;
  }

  /*@TODO remove any leading / */
  /*@TODO ensure # is only at the end */

  const char *rest_path = path;
  const char *next_path_part;

  struct subtree_node *current_node = &tree->root;

  while (rest_path != NULL && *rest_path != '\0') {
    next_path_part = strchr(rest_path, '/');
    if (next_path_part) {
      next_path_part++;
    }

    if (strcmpdelim("#", rest_path, '/') == 0) {
      current_node->wildcard_subscribed_mask |= subscriber_mask;
      current_node = NULL;
      break;
    }

    struct subtree_node *child = subtree_node_find_by_path(current_node, rest_path);
    if (!child) {
      rc = subtree_node_list_add_node(&current_node->children, rest_path, &child);
      if (rc != 0) {
        goto out;
      }
    }

    current_node = child;
    rest_path = next_path_part;
  }

  if (current_node) {
    current_node->subscribed_mask |= subscriber_mask;
  }

out:
  return rc;
}

int subscription_tree_collect_subscribers(struct subscription_tree *tree, const char *path, uint64_t *subscribers) {
  int rc = 0;

  struct path_save {
    struct subtree_node *node;
    const char *saveptr;
  } wildcard_restarts[8];
  uint32_t num_restarts = 0;
  uint64_t submask = 0;

  /* @TODO ensure path does not contain any wildcards */

  const char *rest_path = path;
  const char *next_path_part;

  struct subtree_node *current_node = &tree->root;

  while (true) {
    while (rest_path != NULL && *rest_path != '\0') {
      next_path_part = strchr(rest_path, '/');
      if (next_path_part) {
        next_path_part++;
      }

      submask |= current_node->wildcard_subscribed_mask;

      struct subtree_node *wildcard_node = subtree_node_find_by_path(current_node, "-");
      if (wildcard_node) {
        if (num_restarts >= 8) {
          rc = -ENOMEM;
          goto out;
        }

        wildcard_restarts[num_restarts].node = wildcard_node;
        wildcard_restarts[num_restarts].saveptr = next_path_part;
        num_restarts++;
      }

      struct subtree_node *child_node = subtree_node_find_by_path(current_node, rest_path);
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

    if (num_restarts == 0) {
      break;
    }

    num_restarts--;
    current_node = wildcard_restarts[num_restarts].node;
    rest_path = wildcard_restarts[num_restarts].saveptr;
  }

  if (subscribers) {
    *subscribers = submask;
  }

out:
  return rc;
}

int subtree_node_init(struct subtree_node *node, char *path_component) {
  memset(node, 0, sizeof(struct subtree_node));

  node->path_component = path_component;

  return 0;
}

void subtree_node_free(struct subtree_node *node) {
  free(node->path_component);

  for (uint32_t i = 0; i < node->children.used; i++) {
    subtree_node_free(&node->children.nodes[i]);
  }

  free(node->children.nodes);
}



struct subtree_node *subtree_node_find_by_path(struct subtree_node *node, const char *path_component) {
  for (uint32_t i = 0; i < node->children.used; i++) {
    struct subtree_node *child = &node->children.nodes[i];
    if (strcmpdelim(child->path_component, path_component, '/') == 0) {
      return child;
    }
  }

  return NULL;
}

int subtree_node_list_add_node(struct subtree_node_list *list, const char *path_component,
                               struct subtree_node **new_node) {
  uint16_t new_allocated;
  struct subtree_node *new_list;

  if (list->used == list->allocated) {
    /* Need allocation */
    new_allocated = list->allocated * 2 + 2;
    new_list = realloc(list->nodes, new_allocated * sizeof(struct subtree_node));
    if (!new_list) {
      return -ENOMEM;
    }

    list->allocated = new_allocated;
    list->nodes = new_list;
  }

  struct subtree_node *node = &list->nodes[list->used];
  list->used++;

  char *delim = strchr(path_component, '/');
  size_t n = delim ? delim - path_component : strlen(path_component);

  subtree_node_init(node, strndup(path_component, n));

  if (new_node) {
    *new_node = node;
  }

  return 0;
}