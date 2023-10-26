#include <nala.h>

#include "lite_broker/subtree.h"

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

TEST(subtree_subscribe_invalid) {
  struct subscription_tree tree;

  ASSERT_EQ(subscription_tree_init(&tree), 0);

  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/#", 1), 0);
  ASSERT_LT(subscription_tree_subscribe(&tree, "this/#/blah", 1), 0);

  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/+/blah/+/hej", 1), 0);
  ASSERT_LT(subscription_tree_subscribe(&tree, "this/+/+/+/+/+/+/+/+/+/+/+/+", 1), 0);

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

  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/+/path", 3), 0);

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

TEST(subtree_subscribe_wildcard3) {
  struct subscription_tree tree;

  ASSERT_EQ(subscription_tree_init(&tree), 0);

  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/+/path", 3), 0);
  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/+/a/path", 4), 0);
  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/+/a/path/#", 5), 0);

  uint64_t subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is", &subscribers), 0);
  ASSERT_EQ(subscribers, 0);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is/a/path", &subscribers), 0);
  ASSERT_EQ(subscribers, 1 << 3 | 1 << 4);

  subscribers = 0;
  ASSERT_EQ(subscription_tree_collect_subscribers(&tree, "this/is/a/path/very/deep/under", &subscribers), 0);
  ASSERT_EQ(subscribers, 1 << 5);

  subscription_tree_free(&tree);
}

TEST(subtree_flush_subscriber) {
  struct subscription_tree tree;

  ASSERT_EQ(subscription_tree_init(&tree), 0);

  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/a/path", 1), 0);
  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/a/path2", 1), 0);
  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/b/path", 1), 0);
  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/b/path2", 1), 0);
  ASSERT_EQ(subscription_tree_subscribe(&tree, "this/is/b/path3", 1), 0);

  ASSERT_EQ(subscription_tree_purge(&tree, 1), 0);

  subscription_tree_free(&tree);
}