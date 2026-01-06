#ifndef PLAYER_TABLE_HPP
#define PLAYER_TABLE_HPP

#include <FL/Fl_Table.H>
#include <FL/fl_draw.H>
#include <string>
#include <vector>
#include <functional>

struct PlayerInfo {
    int index;
    std::string name;
    int ping;
    int health;
    int frags;
    std::string steamId;
};

class PlayerTable : public Fl_Table {
public:
    PlayerTable(int x, int y, int w, int h, const char* label = nullptr);

    void setPlayers(const std::vector<PlayerInfo>& players);
    void setKickCallback(std::function<void(int)> callback);
    void setBanCallback(std::function<void(int)> callback);
    void updateColumnWidths();

protected:
    void draw_cell(TableContext context, int row, int col,
                   int x, int y, int w, int h) override;
    int handle(int event) override;

private:
    std::vector<PlayerInfo> m_players;
    std::function<void(int)> m_kickCallback;
    std::function<void(int)> m_banCallback;
    int m_hoverRow;
    int m_hoverCol;

    static constexpr int COL_INDEX = 0;
    static constexpr int COL_NAME = 1;
    static constexpr int COL_PING = 2;
    static constexpr int COL_HEALTH = 3;
    static constexpr int COL_FRAGS = 4;
    static constexpr int COL_KICK = 5;
    static constexpr int COL_BAN = 6;
    static constexpr int NUM_COLS = 7;

    void drawButton(int x, int y, int w, int h, const char* label, bool hover);
    bool isButtonCol(int col) const { return col == COL_KICK || col == COL_BAN; }
};

#endif // PLAYER_TABLE_HPP
