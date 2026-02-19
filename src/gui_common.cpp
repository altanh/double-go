#include "gui_common.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

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

// ── Star point positions for 9x9 ───────────────────────────────────────────

static const double_go::Point HOSHI_9[] = {
    {2, 2}, {2, 6}, {4, 4}, {6, 2}, {6, 6},
};
static constexpr int HOSHI_9_COUNT = 5;

// ── Text rendering ──────────────────────────────────────────────────────────

void draw_text(SDL_Renderer *renderer, int x, int y, const char *text,
               int scale) {
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

int text_width(const char *text, int scale) {
  int len = static_cast<int>(std::strlen(text));
  if (len == 0)
    return 0;
  return len * (5 * scale + scale) - scale;
}

// ── Drawing helpers ─────────────────────────────────────────────────────────

void draw_filled_circle(SDL_Renderer *renderer, int cx, int cy, int r) {
  for (int dy = -r; dy <= r; ++dy) {
    int dx = static_cast<int>(std::sqrt(r * r - dy * dy));
    SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
  }
}

void draw_circle_outline(SDL_Renderer *renderer, int cx, int cy, int r) {
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

// ── Coordinate conversion ───────────────────────────────────────────────────

int board_x(int col) { return MARGIN + col * CELL_SIZE; }
int board_y(int row) { return MARGIN + row * CELL_SIZE; }

std::optional<double_go::Point> pixel_to_point(int px, int py) {
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

// ── Score formatting ────────────────────────────────────────────────────────

std::string format_score(double v) {
  if (v == static_cast<int>(v))
    return std::to_string(static_cast<int>(v));
  char buf[16];
  std::snprintf(buf, sizeof(buf), "%.1f", v);
  return buf;
}

// ── Board rendering ─────────────────────────────────────────────────────────

void render_board(SDL_Renderer *renderer, const double_go::Board &board,
                  std::optional<double_go::Point> last_move,
                  std::optional<double_go::Point> hover, double komi) {
  // Background
  SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, 255);
  SDL_Rect board_rect{0, 0, WIN_W, WIN_H - STATUS_HEIGHT};
  SDL_RenderFillRect(renderer, &board_rect);

  // Grid lines
  SDL_SetRenderDrawColor(renderer, LINE_COLOR.r, LINE_COLOR.g, LINE_COLOR.b,
                         255);
  for (int i = 0; i < BOARD_SIZE; ++i) {
    SDL_RenderDrawLine(renderer, board_x(0), board_y(i),
                       board_x(BOARD_SIZE - 1), board_y(i));
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
  if (!board.game_over() && hover &&
      board.at(*hover) == double_go::Color::Empty && !board.must_pass() &&
      board.is_legal(*hover)) {
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

  auto sr = board.score(komi);
  int line1_y = WIN_H - STATUS_HEIGHT + 8;
  int line2_y = line1_y + 20;

  if (board.game_over()) {
    std::string winner;
    if (sr.black_score > sr.white_score)
      winner = "Black wins";
    else if (sr.white_score > sr.black_score)
      winner = "White wins";
    else
      winner = "Draw";

    std::string line1 = "GAME OVER | B:" + format_score(sr.black_score) +
                        " W:" + format_score(sr.white_score) + " | " + winner;

    SDL_SetRenderDrawColor(renderer, STATUS_TEXT.r, STATUS_TEXT.g,
                           STATUS_TEXT.b, 255);
    draw_text(renderer, 8, line1_y, line1.c_str());

    std::string line2 = "R:reset  Q:quit";
    draw_text(renderer, 8, line2_y, line2.c_str());
  } else {
    // Player indicator circle
    int indicator_cx = 24;
    int indicator_cy = line1_y + 7;
    int indicator_r = 7;
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

    std::string status;
    const char *player =
        board.to_play() == double_go::Color::Black ? "BLACK" : "WHITE";

    if (board.must_pass()) {
      status = std::string(player) + " MUST PASS (P)";
    } else if (board.phase() == double_go::Phase::Second) {
      status = std::string(player) + " place 2nd stone or P:end turn";
    } else {
      status = std::string(player) + " to play";
    }

    std::string caps =
        " | B:" + std::to_string(board.captures(double_go::Color::Black)) +
        " W:" + std::to_string(board.captures(double_go::Color::White));
    status += caps;

    SDL_SetRenderDrawColor(renderer, STATUS_TEXT.r, STATUS_TEXT.g,
                           STATUS_TEXT.b, 255);
    draw_text(renderer, 44, line1_y, status.c_str());

    std::string score_line =
        "B:" + std::to_string(sr.black_stones + sr.black_territory) +
        " W:" + std::to_string(sr.white_stones + sr.white_territory) + "+" +
        format_score(komi) + "=" + format_score(sr.white_score) +
        "  Komi:" + format_score(komi) + " [+/-]";
    draw_text(renderer, 8, line2_y, score_line.c_str());
  }
}
