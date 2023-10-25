#include <nala.h>

#include "lite_broker/subtree.h"

TEST(subtree_list_add_node1) {
  struct subtree_node top_node;
  struct subtree_node *new1 = NULL;
  struct subtree_node *new2 = NULL;

  subtree_node_init(&top_node, NULL);

  ASSERT_EQ(subtree_node_list_add_node(&top_node.children, "new1", &new1), 0);
  ASSERT_NE(new1, NULL);

  ASSERT_EQ(subtree_node_list_add_node(&top_node.children, "new2", &new2), 0);
  ASSERT_NE(new2, NULL);

  ASSERT_NE(new1, new2);

  subtree_node_free(&top_node);
}

TEST(subtree_init1) {
  struct subscription_tree tree;

  ASSERT_EQ(subscription_tree_init(&tree), 0);
}

TEST(subtree_subscribe1) {
  struct subscription_tree tree;

  ASSERT_EQ(subscription_tree_init(&tree), 0);

  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/a/path", 1), 0);

  uint64_t subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is/a/path", &subscribers), 0);
  ASSERT_EQ(subscribers, 1 << 1);

  subscription_tree_free(&tree);
}

TEST(subtree_subscribe_wildcard1) {
  struct subscription_tree tree;

  ASSERT_EQ(subscription_tree_init(&tree), 0);

  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/#", 2), 0);

  uint64_t subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is/a/path", &subscribers), 0);
  ASSERT_EQ(subscribers, 1 << 2);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is/a/path/very/deep/under", &subscribers), 0);
  ASSERT_EQ(subscribers, 1 << 2);

  subscription_tree_free(&tree);
}

TEST(subtree_subscribe_wildcard2) {
  struct subscription_tree tree;

  ASSERT_EQ(subscription_tree_init(&tree), 0);

  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/-/path", 3), 0);

  uint64_t subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is/a/path", &subscribers), 0);
  ASSERT_EQ(subscribers, 1 << 3);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is/a/path/very/deep/under", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscription_tree_free(&tree);
}