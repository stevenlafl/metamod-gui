#include "player_table.hpp"
#include <FL/Fl.H>
#include <cstdio>

PlayerTable::PlayerTable(int x, int y, int w, int h, const char* label)
    : Fl_Table(x, y, w, h, label)
    , m_hoverRow(-1)
    , m_hoverCol(-1)
{
    cols(NUM_COLS);
    col_header(1);
    col_resize(0);
    row_header(0);
    row_height_all(22);

    updateColumnWidths();

    end();
}

void PlayerTable::updateColumnWidths() {
    int scrollbarW = Fl::scrollbar_size();
    int availableW = w() - scrollbarW - 4;  // Account for scrollbar and borders

    // Column widths as percentages (total = 100%)
    // Index: 5%, Name: 40%, Ping: 8%, HP: 8%, Frags: 9%, Kick: 15%, Ban: 15%
    col_width(COL_INDEX, availableW * 5 / 100);
    col_width(COL_NAME, availableW * 40 / 100);
    col_width(COL_PING, availableW * 8 / 100);
    col_width(COL_HEALTH, availableW * 8 / 100);
    col_width(COL_FRAGS, availableW * 9 / 100);
    col_width(COL_KICK, availableW * 15 / 100);
    col_width(COL_BAN, availableW * 15 / 100);
}

void PlayerTable::setPlayers(const std::vector<PlayerInfo>& players) {
    m_players = players;
    rows(static_cast<int>(players.size()));
    redraw();
}

void PlayerTable::setKickCallback(std::function<void(int)> callback) {
    m_kickCallback = callback;
}

void PlayerTable::setBanCallback(std::function<void(int)> callback) {
    m_banCallback = callback;
}

void PlayerTable::draw_cell(TableContext context, int row, int col,
                            int x, int y, int w, int h) {
    switch (context) {
        case CONTEXT_COL_HEADER: {
            fl_push_clip(x, y, w, h);
            fl_draw_box(FL_THIN_UP_BOX, x, y, w, h, col_header_color());
            fl_color(FL_BLACK);
            fl_font(FL_HELVETICA_BOLD, 12);

            const char* headers[] = {"#", "Name", "Ping", "HP", "Frags", "Kick", "Ban"};
            fl_draw(headers[col], x + 4, y, w - 8, h, FL_ALIGN_LEFT);
            fl_pop_clip();
            break;
        }

        case CONTEXT_CELL: {
            if (row < 0 || row >= static_cast<int>(m_players.size())) break;

            fl_push_clip(x, y, w, h);

            // Alternating row colors
            Fl_Color bgColor = (row % 2 == 0) ? FL_WHITE : fl_rgb_color(248, 248, 248);
            fl_draw_box(FL_FLAT_BOX, x, y, w, h, bgColor);

            const PlayerInfo& player = m_players[row];
            char buf[64];

            if (isButtonCol(col)) {
                bool hover = (row == m_hoverRow && col == m_hoverCol);
                const char* label = (col == COL_KICK) ? "Kick" : "Ban";
                drawButton(x + 2, y + 2, w - 4, h - 4, label, hover);
            } else {
                fl_color(FL_BLACK);
                fl_font(FL_HELVETICA, 12);

                switch (col) {
                    case COL_INDEX:
                        snprintf(buf, sizeof(buf), "%d", player.index);
                        fl_draw(buf, x + 4, y, w - 8, h, FL_ALIGN_LEFT);
                        break;
                    case COL_NAME:
                        fl_draw(player.name.c_str(), x + 4, y, w - 8, h, FL_ALIGN_LEFT);
                        break;
                    case COL_PING:
                        snprintf(buf, sizeof(buf), "%d", player.ping);
                        fl_draw(buf, x + 4, y, w - 8, h, FL_ALIGN_RIGHT);
                        break;
                    case COL_HEALTH:
                        snprintf(buf, sizeof(buf), "%d", player.health);
                        fl_draw(buf, x + 4, y, w - 8, h, FL_ALIGN_RIGHT);
                        break;
                    case COL_FRAGS:
                        snprintf(buf, sizeof(buf), "%d", player.frags);
                        fl_draw(buf, x + 4, y, w - 8, h, FL_ALIGN_RIGHT);
                        break;
                }
            }

            fl_pop_clip();
            break;
        }

        default:
            break;
    }
}

void PlayerTable::drawButton(int x, int y, int w, int h, const char* label, bool hover) {
    Fl_Color bgColor = hover ? fl_rgb_color(184, 212, 240) : fl_rgb_color(221, 221, 221);
    fl_draw_box(FL_UP_BOX, x, y, w, h, bgColor);
    fl_color(FL_BLACK);
    fl_font(FL_HELVETICA, 11);
    fl_draw(label, x, y, w, h, FL_ALIGN_CENTER);
}

int PlayerTable::handle(int event) {
    int row, col;
    ResizeFlag resizeFlag;

    // Check if event is inside this widget
    bool mouseInside = Fl::event_inside(this);

    switch (event) {
        case FL_MOVE: {
            cursor2rowcol(row, col, resizeFlag);

            if (row != m_hoverRow || col != m_hoverCol) {
                m_hoverRow = row;
                m_hoverCol = col;
                // Show hand cursor over kick/ban buttons
                if (isButtonCol(col) && row >= 0 && row < static_cast<int>(m_players.size())) {
                    fl_cursor(FL_CURSOR_HAND);
                } else {
                    fl_cursor(FL_CURSOR_DEFAULT);
                }
                redraw();
            }
            return 1;
        }

        case FL_LEAVE:
            if (m_hoverRow >= 0 || m_hoverCol >= 0) {
                m_hoverRow = -1;
                m_hoverCol = -1;
                fl_cursor(FL_CURSOR_DEFAULT);
                redraw();
            }
            return 1;

        case FL_MOUSEWHEEL:
            // Handle mouse wheel scrolling when mouse is over the table
            if (mouseInside) {
                return Fl_Table::handle(event);
            }
            return 0;  // Don't consume event if mouse not over us

        case FL_PUSH: {
            Fl::focus(nullptr);  // Clear focus from any input
            cursor2rowcol(row, col, resizeFlag);

            if (row >= 0 && row < static_cast<int>(m_players.size())) {
                int playerIndex = m_players[row].index;

                if (col == COL_KICK && m_kickCallback) {
                    m_kickCallback(playerIndex);
                    return 1;
                } else if (col == COL_BAN && m_banCallback) {
                    m_banCallback(playerIndex);
                    return 1;
                }
            }
            break;
        }
    }

    return Fl_Table::handle(event);
}
