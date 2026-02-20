#pragma once

#include "double-go/double-go.h"

#include <SDL2/SDL.h>
#include <optional>
#include <string>

// ── Layout constants ────────────────────────────────────────────────────────

inline constexpr int BOARD_SIZE = 9;
inline constexpr int MARGIN = 40;
inline constexpr int STATUS_HEIGHT = 80;
inline constexpr int CELL_SIZE = 68;
inline constexpr int BOARD_PX = CELL_SIZE * (BOARD_SIZE - 1);
inline constexpr int WIN_W = BOARD_PX + 2 * MARGIN;
inline constexpr int WIN_H = BOARD_PX + 2 * MARGIN + STATUS_HEIGHT;
inline constexpr int STONE_RADIUS = CELL_SIZE / 2 - 3;

// ── Color constants ─────────────────────────────────────────────────────────

inline constexpr SDL_Color BG_COLOR{0xDC, 0xB3, 0x5C, 0xFF};
inline constexpr SDL_Color LINE_COLOR{0x30, 0x30, 0x30, 0xFF};
inline constexpr SDL_Color BLACK_STONE{0x20, 0x20, 0x20, 0xFF};
inline constexpr SDL_Color WHITE_STONE{0xF0, 0xF0, 0xF0, 0xFF};
inline constexpr SDL_Color STATUS_BG{0x30, 0x30, 0x30, 0xFF};
inline constexpr SDL_Color STATUS_TEXT{0xE0, 0xE0, 0xE0, 0xFF};

// ── Function declarations ───────────────────────────────────────────────────

void draw_text(SDL_Renderer *renderer, int x, int y, const char *text,
               int scale = 2);
int text_width(const char *text, int scale = 2);
void draw_filled_circle(SDL_Renderer *renderer, int cx, int cy, int r);
void draw_circle_outline(SDL_Renderer *renderer, int cx, int cy, int r);
int board_x(int col);
int board_y(int row);
std::optional<double_go::Point> pixel_to_point(int px, int py);
std::string format_score(double v);
void render_board(SDL_Renderer *renderer, const double_go::Board &board,
                  std::optional<double_go::Point> last_move,
                  std::optional<double_go::Point> hover, double komi);
