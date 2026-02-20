#include "gui_common.h"

int main(int /*argc*/, char * /*argv*/[]) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  SDL_Window *window =
      SDL_CreateWindow("Double Go", SDL_WINDOWPOS_CENTERED,
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

  double_go::Board board(BOARD_SIZE);
  std::optional<double_go::Point> last_move;
  std::optional<double_go::Point> hover_point;
  double komi = 6.5;

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;

      case SDL_MOUSEMOTION: {
        hover_point = pixel_to_point(event.motion.x, event.motion.y);
        break;
      }

      case SDL_MOUSEBUTTONDOWN: {
        if (board.game_over())
          break;

        auto pt = pixel_to_point(event.button.x, event.button.y);
        if (!pt)
          break;

        if (event.button.button == SDL_BUTTON_LEFT) {
          if (board.apply(double_go::Action::place(*pt)))
            last_move = *pt;
        }
        break;
      }

      case SDL_KEYDOWN: {
        switch (event.key.keysym.sym) {
        case SDLK_q:
        case SDLK_ESCAPE:
          running = false;
          break;
        case SDLK_p:
          if (!board.game_over()) {
            board.pass();
            last_move = std::nullopt;
          }
          break;
        case SDLK_r:
          board = double_go::Board(BOARD_SIZE);
          last_move = std::nullopt;
          break;
        case SDLK_EQUALS:
        case SDLK_PLUS:
          komi += 0.5;
          break;
        case SDLK_MINUS:
        case SDLK_UNDERSCORE:
          if (komi >= 0.5)
            komi -= 0.5;
          break;
        default:
          break;
        }
        break;
      }

      default:
        break;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    render_board(renderer, board, last_move, hover_point, komi);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
