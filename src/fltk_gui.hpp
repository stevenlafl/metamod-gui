#ifndef FLTK_GUI_HPP
#define FLTK_GUI_HPP

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <string>
#include <vector>
#include <deque>
#include <ctime>
#include <unistd.h>

#include "player_table.hpp"

// Custom button with hover state
class HoverButton : public Fl_Button {
public:
    HoverButton(int x, int y, int w, int h, const char* label = nullptr);
    int handle(int event) override;
    void draw() override;
private:
    bool m_hover;
};

// Custom toggle button with hover state
class HoverToggleButton : public Fl_Toggle_Button {
public:
    HoverToggleButton(int x, int y, int w, int h, const char* label = nullptr);
    int handle(int event) override;
    void draw() override;
private:
    bool m_hover;
};

// Custom choice with scrollable popup
class ScrollChoice : public Fl_Widget {
public:
    ScrollChoice(int x, int y, int w, int h, const char* label = nullptr);
    void add(const char* text);
    void clear();
    int value() const { return m_value; }
    void value(int v);
    int size() const { return (int)m_items.size(); }
    const char* text() const;
    void textsize(int s) { m_textsize = s; }
    void callback(Fl_Callback* cb, void* data) { m_callback = cb; m_userdata = data; }
    int handle(int event) override;
    void draw() override;
    void maxHeight(int h) { m_maxHeight = h; }
private:
    void showPopup();
    static void popupCallback(Fl_Widget* w, void* data);

    std::vector<std::string> m_items;
    int m_value = 0;
    int m_textsize = 11;
    int m_maxHeight = 300;
    Fl_Callback* m_callback = nullptr;
    void* m_userdata = nullptr;
    bool m_hover = false;
};

// Forward declaration
class FltkGUI;

// Custom input with placeholder text and command history
class PlaceholderInput : public Fl_Input {
public:
    PlaceholderInput(int x, int y, int w, int h, const char* label = nullptr);
    void setPlaceholder(const char* text) { m_placeholder = text ? text : ""; }
    void setGui(FltkGUI* gui) { m_gui = gui; }
    int handle(int event) override;
    void draw() override;
private:
    std::string m_placeholder;
    FltkGUI* m_gui = nullptr;
};

// Custom text display that detects manual scrolling
class LogDisplay : public Fl_Text_Display {
public:
    LogDisplay(int x, int y, int w, int h);
    int handle(int event) override;
    void setGui(FltkGUI* gui) { m_gui = gui; }
    void scrollToBottom();
    bool isAtBottom() const;
    void checkScrollPosition();
private:
    FltkGUI* m_gui;
};

class FltkGUI {
public:
    static FltkGUI& getInstance();

    bool initialize();
    void shutdown();
    void show();
    void hide();
    void processEvents();
    bool isVisible() const { return m_visible; }

    void appendLog(const char* text);
    void disableAutoScroll();
    void enableAutoScroll();
    void selectCurrentMap();

    // Command history navigation
    void historyUp();
    void historyDown();

private:
    FltkGUI();
    ~FltkGUI();
    FltkGUI(const FltkGUI&) = delete;
    FltkGUI& operator=(const FltkGUI&) = delete;

    void createWindow();
    void destroyWindow();

    // Refresh callbacks
    static void onRefreshTimer(void* data);
    void refresh();
    void refreshServerStatus();
    void refreshPlayerList();

    // Widget callbacks
    static void onChangeMapBtn(Fl_Widget*, void* data);
    static void onRestartBtn(Fl_Widget*, void* data);
    static void onMapChoice(Fl_Widget*, void* data);
    static void onCommandInput(Fl_Widget*, void* data);
    static void onWindowClose(Fl_Widget*, void* data);
    static void onAutoScrollToggle(Fl_Widget*, void* data);

    // Actions
    void doChangeMap();
    void doRestart();
    void doKickPlayer(int playerIndex);
    void doBanPlayer(int playerIndex);
    void doMapSelect();
    void doCommand();

    // Map loading
    void loadMapCycle();

    // stdout/stderr capture
    void setupOutputCapture();
    void cleanupOutputCapture();
    void readCapturedOutput();

    // Window and widgets
    Fl_Double_Window* m_window;

    // Title bar
    Fl_Box* m_titleBar;

    // Server Status pane
    Fl_Group* m_statusGroup;
    Fl_Box* m_hostnameLabel;
    Fl_Box* m_mapLabel;
    Fl_Box* m_playersLabel;
    Fl_Box* m_timeLabel;
    Fl_Box* m_entitiesLabel;

    // Controls pane
    Fl_Group* m_controlsGroup;
    ScrollChoice* m_mapChoice;
    HoverButton* m_changeMapBtn;
    HoverButton* m_restartBtn;

    // Players pane
    Fl_Group* m_playersGroup;
    PlayerTable* m_playerTable;

    // Logs pane
    Fl_Group* m_logsGroup;
    LogDisplay* m_logDisplay;
    Fl_Text_Buffer* m_logBuffer;
    HoverToggleButton* m_autoScrollBtn;

    // Command input
    PlaceholderInput* m_commandInput;

    // State
    bool m_initialized;
    bool m_visible;

    // Server status data
    std::string m_hostname;
    std::string m_currentMap;
    int m_playerCount;
    int m_maxPlayers;
    int m_entityCount;
    float m_serverTime;
    float m_timeLimit;

    // Map list
    std::vector<std::string> m_maps;
    int m_selectedMapIndex;

    // Logs
    std::deque<std::string> m_logLines;
    static constexpr size_t MAX_LOG_LINES = 500;
    bool m_autoScroll;

    // Command history
    std::vector<std::string> m_commandHistory;
    static constexpr size_t MAX_HISTORY = 30;
    int m_historyIndex;
    std::string m_savedInput;

    // stdout/stderr capture
    int m_stdoutPipe[2];
    int m_stderrPipe[2];
    int m_origStdout;
    int m_origStderr;
    bool m_captureActive;
    std::string m_partialLine;
};

#endif // FLTK_GUI_HPP
