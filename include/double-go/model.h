#pragma once

#include "board.h"
#include <deque>
#include <memory>
#include <sstream>
#include <torch/torch.h>

namespace double_go {

using namespace torch;

struct ResidualBlock : nn::Module {
  nn::Conv2d conv1 = nullptr;
  nn::Conv2d conv2 = nullptr;
  nn::BatchNorm2d bn1 = nullptr;
  nn::BatchNorm2d bn2 = nullptr;

  ResidualBlock(int channels)
      : conv1(register_module(
            "conv1",
            nn::Conv2d(nn::Conv2dOptions(channels, channels, 3).padding(1)))),
        conv2(register_module(
            "conv2",
            nn::Conv2d(nn::Conv2dOptions(channels, channels, 3).padding(1)))),
        bn1(register_module("bn1", nn::BatchNorm2d(channels))),
        bn2(register_module("bn2", nn::BatchNorm2d(channels))) {}

  Tensor forward(Tensor x) {
    Tensor residual = bn2(conv2(relu(bn1(conv1(x)))));
    return relu(residual + x);
  }
};

struct PolicyHead : nn::Module {
  nn::Conv2d conv = nullptr;
  nn::BatchNorm2d bn = nullptr;
  nn::Linear fc = nullptr;

  PolicyHead(int board_size, int channels)
      : conv(register_module("conv",
                             nn::Conv2d(nn::Conv2dOptions(channels, 2, 1)))),
        bn(register_module("bn", nn::BatchNorm2d(2))),
        fc(register_module("fc", nn::Linear(2 * board_size * board_size,
                                            board_size * board_size + 1))) {}

  Tensor forward(Tensor x) { return fc(relu(bn(conv(x))).flatten(1)); }
};

struct ValueHead : nn::Module {
  nn::Conv2d conv = nullptr;
  nn::BatchNorm2d bn = nullptr;
  nn::Linear fc1 = nullptr;
  nn::Linear fc2 = nullptr;

  ValueHead(int board_size, int channels)
      : conv(register_module("conv",
                             nn::Conv2d(nn::Conv2dOptions(channels, 1, 1)))),
        bn(register_module("bn", nn::BatchNorm2d(1))),
        fc1(register_module("fc1", nn::Linear(board_size * board_size, 256))),
        fc2(register_module("fc2", nn::Linear(256, 1))) {}

  Tensor forward(Tensor x) {
    return tanh(fc2(relu(fc1(relu(bn(conv(x))).flatten(1)))));
  }
};

struct Model : nn::Module {
  static constexpr size_t HISTORY_LEN = 8;
  static constexpr size_t NUM_PLANES = HISTORY_LEN * 2 + 1 + 1 + 1 + 1;

  const int board_size;
  const int num_blocks;
  const int num_channels;

  nn::Conv2d conv = nullptr;
  nn::Sequential blocks = nullptr;
  std::shared_ptr<PolicyHead> policy_head = nullptr;
  std::shared_ptr<ValueHead> value_head = nullptr;

  Model(int board_size, int num_blocks = 10, int num_channels = 64)
      : board_size(board_size), num_blocks(num_blocks),
        num_channels(num_channels),
        conv(register_module(
            "conv",
            nn::Conv2d(
                nn::Conv2dOptions(NUM_PLANES, num_channels, 3).padding(1)))),
        blocks(register_module("blocks", nn::Sequential())),
        policy_head(register_module(
            "policy_head",
            std::make_shared<PolicyHead>(board_size, num_channels))),
        value_head(register_module(
            "value_head",
            std::make_shared<ValueHead>(board_size, num_channels))) {
    for (int i = 0; i < num_blocks; ++i) {
      blocks->push_back(ResidualBlock(num_channels));
    }
  }

  Tensor encode(const std::deque<Board> &boards);

  std::pair<Tensor, Tensor> forward(Tensor encoding) {
    Tensor features = blocks->forward(conv(encoding));
    Tensor policy = policy_head->forward(features);
    Tensor value = value_head->forward(features);
    return {policy, value};
  }
};

} // namespace double_go