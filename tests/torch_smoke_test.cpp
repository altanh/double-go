#include <gtest/gtest.h>
#include <torch/torch.h>

TEST(Torch, SmokeTest) {
  auto t = torch::rand({2, 3});
  EXPECT_EQ(t.size(0), 2);
  EXPECT_EQ(t.size(1), 3);
  auto sum = t.sum().item<float>();
  EXPECT_GT(sum, 0.0f);
  EXPECT_LT(sum, 6.0f);
}
