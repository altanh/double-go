#include "double-go/double-go.h"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

// ── Bitmap font (5x7, ASCII 32–126) ────────────────────────────────────────
// Each glyph is 7 bytes (rows top→bottom), each byte's bits 4..0 = columns
// left→right.  Rendered scaled 2× → 10×14 pixels per character.

static const uint8_t FONT_GLYPHS[][7] = {
    // 32 ' '
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 33 '!'
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04},
    // 34 '"'
    {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 35 '#'
    {0x0A, 0x1F, 0x0A, 0x0A, 0x1F, 0x0A, 0x00},
    // 36 '$'
    {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04},
    // 37 '%'
    {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03},
    // 38 '&'
    {0x08, 0x14, 0x14, 0x08, 0x15, 0x12, 0x0D},
    // 39 '''
    {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 40 '('
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02},
    // 41 ')'
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08},
    // 42 '*'
    {0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00},
    // 43 '+'
    {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00},
    // 44 ','
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08},
    // 45 '-'
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},
    // 46 '.'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04},
    // 47 '/'
    {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10},
    // 48 '0'
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    // 49 '1'
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // 50 '2'
    {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F},
    // 51 '3'
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},
    // 52 '4'
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    // 53 '5'
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    // 54 '6'
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    // 55 '7'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    // 56 '8'
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    // 57 '9'
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
    // 58 ':'
    {0x00, 0x00, 0x04, 0x00, 0x00, 0x04, 0x00},
    // 59 ';'
    {0x00, 0x00, 0x04, 0x00, 0x00, 0x04, 0x08},
    // 60 '<'
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02},
    // 61 '='
    {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00},
    // 62 '>'
    {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08},
    // 63 '?'
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04},
    // 64 '@'
    {0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E},
    // 65 'A'
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    // 66 'B'
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    // 67 'C'
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    // 68 'D'
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},
    // 69 'E'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    // 70 'F'
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    // 71 'G'
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E},
    // 72 'H'
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    // 73 'I'
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // 74 'J'
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},
    // 75 'K'
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    // 76 'L'
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    // 77 'M'
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    // 78 'N'
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    // 79 'O'
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    // 80 'P'
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    // 81 'Q'
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    // 82 'R'
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    // 83 'S'
    {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E},
    // 84 'T'
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    // 85 'U'
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    // 86 'V'
    {0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04},
    // 87 'W'
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11},
    // 88 'X'
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    // 89 'Y'
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    // 90 'Z'
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
    // 91 '['
    {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E},
    // 92 '\'
    {0x10, 0x10, 0x08, 0x04, 0x02, 0x01, 0x01},
    // 93 ']'
    {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E},
    // 94 '^'
    {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00},
    // 95 '_'
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F},
    // 96 '`'
    {0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
    // 97 'a'
    {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F},
    // 98 'b'
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E},
    // 99 'c'
    {0x00, 0x00, 0x0E, 0x11, 0x10, 0x11, 0x0E},
    // 100 'd'
    {0x01, 0x01, 0x0F, 0x11, 0x11, 0x11, 0x0F},
    // 101 'e'
    {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E},
    // 102 'f'
    {0x06, 0x08, 0x1E, 0x08, 0x08, 0x08, 0x08},
    // 103 'g'
    {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01},
    // 104 'h'
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x11},
    // 105 'i'
    {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E},
    // 106 'j'
    {0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C},
    // 107 'k'
    {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12},
    // 108 'l'
    {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    // 109 'm'
    {0x00, 0x00, 0x1A, 0x15, 0x15, 0x15, 0x15},
    // 110 'n'
    {0x00, 0x00, 0x1E, 0x11, 0x11, 0x11, 0x11},
    // 111 'o'
    {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E},
    // 112 'p'
    {0x00, 0x00, 0x1E, 0x11, 0x11, 0x1E, 0x10},
    // 113 'q'
    {0x00, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01},
    // 114 'r'
    {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10},
    // 115 's'
    {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E},
    // 116 't'
    {0x08, 0x08, 0x1E, 0x08, 0x08, 0x09, 0x06},
    // 117 'u'
    {0x00, 0x00, 0x11, 0x11, 0x11, 0x11, 0x0F},
    // 118 'v'
    {0x00, 0x00, 0x11, 0x11, 0x0A, 0x0A, 0x04},
    // 119 'w'
    {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A},
    // 120 'x'
    {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11},
    // 121 'y'
    {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E},
    // 122 'z'
    {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F},
    // 123 '{'
    {0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02},
    // 124 '|'
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    // 125 '}'
    {0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08},
    // 126 '~'
    {0x00, 0x00, 0x08, 0x15, 0x02, 0x00, 0x00},
};

static void draw_text(SDL_Renderer *renderer, int x, int y, const char *text,
                      int scale = 2) {
  const int glyph_w = 5 * scale;
  const int glyph_h = 7 * scale;
  int cx = x;
  for (const char *p = text; *p; ++p) {
    int ch = static_cast<unsigned char>(*p);
    if (ch < 32 || ch > 126) {
      cx += glyph_w + scale;
      continue;
    }
    const uint8_t *glyph = FONT_GLYPHS[ch - 32];
    for (int row = 0; row < 7; ++row) {
      uint8_t bits = glyph[row];
      for (int col = 0; col < 5; ++col) {
        if (bits & (0x10 >> col)) {
          SDL_Rect r{cx + col * scale, y + row * scale, scale, scale};
          SDL_RenderFillRect(renderer, &r);
        }
      }
    }
    cx += glyph_w + scale;
  }
  (void)glyph_h;
}

static int text_width(const char *text, int scale = 2) {
  int len = static_cast<int>(std::strlen(text));
  if (len == 0)
    return 0;
  return len * (5 * scale + scale) - scale;
}

// ── Drawing helpers ─────────────────────────────────────────────────────────

static void draw_filled_circle(SDL_Renderer *renderer, int cx, int cy, int r) {
  for (int dy = -r; dy <= r; ++dy) {
    int dx = static_cast<int>(std::sqrt(r * r - dy * dy));
    SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
  }
}

static void draw_circle_outline(SDL_Renderer *renderer, int cx, int cy, int r) {
  int x = r, y = 0, err = 1 - r;
  while (x >= y) {
    SDL_RenderDrawPoint(renderer, cx + x, cy + y);
    SDL_RenderDrawPoint(renderer, cx - x, cy + y);
    SDL_RenderDrawPoint(renderer, cx + x, cy - y);
    SDL_RenderDrawPoint(renderer, cx - x, cy - y);
    SDL_RenderDrawPoint(renderer, cx + y, cy + x);
    SDL_RenderDrawPoint(renderer, cx - y, cy + x);
    SDL_RenderDrawPoint(renderer, cx + y, cy - x);
    SDL_RenderDrawPoint(renderer, cx - y, cy - x);
    ++y;
    if (err < 0) {
      err += 2 * y + 1;
    } else {
      --x;
      err += 2 * (y - x) + 1;
    }
  }
}

// ── Constants ───────────────────────────────────────────────────────────────

static constexpr int BOARD_SIZE = 9;
static constexpr int MARGIN = 40;
static constexpr int STATUS_HEIGHT = 60;
static constexpr int CELL_SIZE = 68;
static constexpr int BOARD_PX = CELL_SIZE * (BOARD_SIZE - 1);
static constexpr int WIN_W = BOARD_PX + 2 * MARGIN;
static constexpr int WIN_H = BOARD_PX + 2 * MARGIN + STATUS_HEIGHT;
static constexpr int STONE_RADIUS = CELL_SIZE / 2 - 3;

// Colors
static constexpr SDL_Color BG_COLOR{0xDC, 0xB3, 0x5C, 0xFF}; // wooden board
static constexpr SDL_Color LINE_COLOR{0x30, 0x30, 0x30, 0xFF};
static constexpr SDL_Color BLACK_STONE{0x20, 0x20, 0x20, 0xFF};
static constexpr SDL_Color WHITE_STONE{0xF0, 0xF0, 0xF0, 0xFF};
static constexpr SDL_Color STATUS_BG{0x30, 0x30, 0x30, 0xFF};
static constexpr SDL_Color STATUS_TEXT{0xE0, 0xE0, 0xE0, 0xFF};

// ── Coordinate conversion ───────────────────────────────────────────────────

static int board_x(int col) { return MARGIN + col * CELL_SIZE; }
static int board_y(int row) { return MARGIN + row * CELL_SIZE; }

// Returns the nearest board intersection to pixel (px, py), or nullopt if too
// far away.
static std::optional<double_go::Point> pixel_to_point(int px, int py) {
  int col = std::round(static_cast<double>(px - MARGIN) / CELL_SIZE);
  int row = std::round(static_cast<double>(py - MARGIN) / CELL_SIZE);
  if (col < 0 || col >= BOARD_SIZE || row < 0 || row >= BOARD_SIZE)
    return std::nullopt;
  int dx = px - board_x(col);
  int dy = py - board_y(row);
  if (dx * dx + dy * dy > STONE_RADIUS * STONE_RADIUS)
    return std::nullopt;
  return double_go::Point{row, col};
}

// ── Star point positions for 9x9 ───────────────────────────────────────────

static const double_go::Point HOSHI_9[] = {
    {2, 2}, {2, 6}, {4, 4}, {6, 2}, {6, 6},
};
static constexpr int HOSHI_9_COUNT = 5;

// ── Rendering ───────────────────────────────────────────────────────────────

static void render_board(SDL_Renderer *renderer, const double_go::Board &board,
                         std::optional<double_go::Point> last_move,
                         std::optional<double_go::Point> hover) {
  // Background
  SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, 255);
  SDL_Rect board_rect{0, 0, WIN_W, WIN_H - STATUS_HEIGHT};
  SDL_RenderFillRect(renderer, &board_rect);

  // Grid lines
  SDL_SetRenderDrawColor(renderer, LINE_COLOR.r, LINE_COLOR.g, LINE_COLOR.b,
                         255);
  for (int i = 0; i < BOARD_SIZE; ++i) {
    // Horizontal
    SDL_RenderDrawLine(renderer, board_x(0), board_y(i),
                       board_x(BOARD_SIZE - 1), board_y(i));
    // Vertical
    SDL_RenderDrawLine(renderer, board_x(i), board_y(0), board_x(i),
                       board_y(BOARD_SIZE - 1));
  }

  // Star points (hoshi)
  for (int i = 0; i < HOSHI_9_COUNT; ++i) {
    int cx = board_x(HOSHI_9[i].col);
    int cy = board_y(HOSHI_9[i].row);
    draw_filled_circle(renderer, cx, cy, 4);
  }

  // Ko point marker
  auto ko = board.ko_point();
  if (ko) {
    SDL_SetRenderDrawColor(renderer, 0xCC, 0x22, 0x22, 255);
    int cx = board_x(ko->col);
    int cy = board_y(ko->row);
    SDL_Rect kr{cx - 4, cy - 4, 9, 9};
    SDL_RenderFillRect(renderer, &kr);
  }

  // Stones
  for (int r = 0; r < BOARD_SIZE; ++r) {
    for (int c = 0; c < BOARD_SIZE; ++c) {
      auto color = board.at({r, c});
      if (color == double_go::Color::Empty)
        continue;
      int cx = board_x(c);
      int cy = board_y(r);
      if (color == double_go::Color::Black) {
        SDL_SetRenderDrawColor(renderer, BLACK_STONE.r, BLACK_STONE.g,
                               BLACK_STONE.b, 255);
        draw_filled_circle(renderer, cx, cy, STONE_RADIUS);
      } else {
        SDL_SetRenderDrawColor(renderer, WHITE_STONE.r, WHITE_STONE.g,
                               WHITE_STONE.b, 255);
        draw_filled_circle(renderer, cx, cy, STONE_RADIUS);
        SDL_SetRenderDrawColor(renderer, BLACK_STONE.r, BLACK_STONE.g,
                               BLACK_STONE.b, 255);
        draw_circle_outline(renderer, cx, cy, STONE_RADIUS);
      }
    }
  }

  // Last move marker
  if (last_move) {
    auto color = board.at(*last_move);
    int cx = board_x(last_move->col);
    int cy = board_y(last_move->row);
    if (color == double_go::Color::Black) {
      SDL_SetRenderDrawColor(renderer, WHITE_STONE.r, WHITE_STONE.g,
                             WHITE_STONE.b, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, BLACK_STONE.r, BLACK_STONE.g,
                             BLACK_STONE.b, 255);
    }
    draw_filled_circle(renderer, cx, cy, 5);
  }

  // Hover preview (semi-transparent stone)
  if (hover && board.at(*hover) == double_go::Color::Empty &&
      !board.must_pass() && board.is_legal(*hover)) {
    int cx = board_x(hover->col);
    int cy = board_y(hover->row);
    if (board.to_play() == double_go::Color::Black) {
      SDL_SetRenderDrawColor(renderer, BLACK_STONE.r, BLACK_STONE.g,
                             BLACK_STONE.b, 100);
    } else {
      SDL_SetRenderDrawColor(renderer, WHITE_STONE.r, WHITE_STONE.g,
                             WHITE_STONE.b, 100);
    }
    draw_filled_circle(renderer, cx, cy, STONE_RADIUS);
    if (board.to_play() == double_go::Color::White) {
      SDL_SetRenderDrawColor(renderer, BLACK_STONE.r, BLACK_STONE.g,
                             BLACK_STONE.b, 100);
      draw_circle_outline(renderer, cx, cy, STONE_RADIUS);
    }
  }

  // ── Status bar ──────────────────────────────────────────────────────────
  SDL_SetRenderDrawColor(renderer, STATUS_BG.r, STATUS_BG.g, STATUS_BG.b, 255);
  SDL_Rect status_rect{0, WIN_H - STATUS_HEIGHT, WIN_W, STATUS_HEIGHT};
  SDL_RenderFillRect(renderer, &status_rect);

  // Player indicator circle
  int indicator_cx = 24;
  int indicator_cy = WIN_H - STATUS_HEIGHT / 2;
  int indicator_r = 10;
  if (board.to_play() == double_go::Color::Black) {
    SDL_SetRenderDrawColor(renderer, BLACK_STONE.r, BLACK_STONE.g,
                           BLACK_STONE.b, 255);
    draw_filled_circle(renderer, indicator_cx, indicator_cy, indicator_r);
  } else {
    SDL_SetRenderDrawColor(renderer, WHITE_STONE.r, WHITE_STONE.g,
                           WHITE_STONE.b, 255);
    draw_filled_circle(renderer, indicator_cx, indicator_cy, indicator_r);
    SDL_SetRenderDrawColor(renderer, BLACK_STONE.r, BLACK_STONE.g,
                           BLACK_STONE.b, 255);
    draw_circle_outline(renderer, indicator_cx, indicator_cy, indicator_r);
  }

  // Build status string
  std::string status;
  const char *player =
      board.to_play() == double_go::Color::Black ? "BLACK" : "WHITE";

  if (board.must_pass()) {
    status = std::string(player) + " MUST PASS (P)";
  } else if (board.phase() == double_go::Phase::DoubleMove) {
    status = std::string(player) + " place 2nd stone";
  } else {
    status = std::string(player) + " to play";
  }

  // Capture counts
  std::string caps =
      " | B:" + std::to_string(board.captures(double_go::Color::Black)) +
      " W:" + std::to_string(board.captures(double_go::Color::White));
  status += caps;

  // Draw status text
  SDL_SetRenderDrawColor(renderer, STATUS_TEXT.r, STATUS_TEXT.g, STATUS_TEXT.b,
                         255);
  draw_text(renderer, 44, WIN_H - STATUS_HEIGHT / 2 - 7, status.c_str());
}

// ── Main ────────────────────────────────────────────────────────────────────

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
        auto pt = pixel_to_point(event.button.x, event.button.y);
        if (!pt)
          break;

        if (board.must_pass())
          break;

        if (event.button.button == SDL_BUTTON_LEFT) {
          // Normal phase: Move; DoubleMove phase: DoubleSecond
          if (board.phase() == double_go::Phase::DoubleMove) {
            if (board.apply(double_go::Action::double_second(*pt)))
              last_move = *pt;
          } else {
            if (board.apply(double_go::Action::move(*pt)))
              last_move = *pt;
          }
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
          // Start double move
          if (board.phase() == double_go::Phase::Normal) {
            if (board.apply(double_go::Action::double_first(*pt)))
              last_move = *pt;
          }
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
          if (board.phase() == double_go::Phase::Normal) {
            board.pass();
            last_move = std::nullopt;
          }
          break;
        case SDLK_r:
          board = double_go::Board(BOARD_SIZE);
          last_move = std::nullopt;
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
    render_board(renderer, board, last_move, hover_point);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
