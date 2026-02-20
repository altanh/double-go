#include "gui_common.h"

#include <algorithm>
#include <random>

int main(int /*argc*/, char * /*argv*/[]) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  SDL_Window *window =
      SDL_CreateWindow("Double Go - Bot vs Bot", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_SHOWN);
  if (!window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  std::random_device rd;
  double_go::RandomBot black_bot(rd());
  double_go::RandomBot white_bot(rd());

  double_go::Board board(BOARD_SIZE);
  std::optional<double_go::Point> last_move;
  double komi = 6.5;

  bool paused = false;
  Uint32 move_delay_ms = 200;
  Uint32 last_move_time = SDL_GetTicks();

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_q:
        case SDLK_ESCAPE:
          running = false;
          break;
        case SDLK_r:
          black_bot = double_go::RandomBot(rd());
          white_bot = double_go::RandomBot(rd());
          board = double_go::Board(BOARD_SIZE);
          last_move = std::nullopt;
          last_move_time = SDL_GetTicks();
          break;
        case SDLK_SPACE:
          paused = !paused;
          break;
        case SDLK_UP:
          move_delay_ms = std::max(20u, move_delay_ms / 2);
          break;
        case SDLK_DOWN:
          move_delay_ms = std::min(2000u, move_delay_ms * 2);
          break;
        default:
          break;
        }
        break;

      default:
        break;
      }
    }

    // Auto-play on timer
    if (!paused && !board.game_over()) {
      Uint32 now = SDL_GetTicks();
      if (now - last_move_time >= move_delay_ms) {
        auto &bot = (board.to_play() == double_go::Color::Black) ? black_bot
                                                                 : white_bot;
        auto action = bot.pick_action(board);
        board.apply(action);
        if (action.type == double_go::ActionType::Place) {
          last_move = action.point;
          last_move_time = now;
        } else {
          last_move = std::nullopt;
          last_move_time = now + move_delay_ms / 2;
        }
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    render_board(renderer, board, last_move, std::nullopt, komi);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
