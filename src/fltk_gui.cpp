#include "fltk_gui.hpp"
#include "icon.xpm"
#include <extdll.h>
#include <meta_api.h>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Scrollbar.H>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <fcntl.h>
#include <poll.h>
#include <dirent.h>

extern enginefuncs_t g_engfuncs;
extern globalvars_t* gpGlobals;

// HoverButton implementation
HoverButton::HoverButton(int x, int y, int w, int h, const char* label)
    : Fl_Button(x, y, w, h, label)
    , m_hover(false)
{
}

int HoverButton::handle(int event) {
    switch (event) {
        case FL_ENTER:
            m_hover = true;
            fl_cursor(FL_CURSOR_HAND);
            redraw();
            return 1;
        case FL_LEAVE:
            m_hover = false;
            fl_cursor(FL_CURSOR_DEFAULT);
            redraw();
            return 1;
        case FL_PUSH:
            Fl::focus(nullptr);  // Clear focus from any input
            break;
    }
    return Fl_Button::handle(event);
}

void HoverButton::draw() {
    if (m_hover) {
        // Temporarily lighten color, draw with theme, then restore
        Fl_Color orig = color();
        color(fl_lighter(orig));
        Fl_Button::draw();
        color(orig);
    } else {
        Fl_Button::draw();
    }
}

// HoverToggleButton implementation
HoverToggleButton::HoverToggleButton(int x, int y, int w, int h, const char* label)
    : Fl_Toggle_Button(x, y, w, h, label)
    , m_hover(false)
{
}

int HoverToggleButton::handle(int event) {
    switch (event) {
        case FL_ENTER:
            m_hover = true;
            fl_cursor(FL_CURSOR_HAND);
            redraw();
            return 1;
        case FL_LEAVE:
            m_hover = false;
            fl_cursor(FL_CURSOR_DEFAULT);
            redraw();
            return 1;
        case FL_PUSH:
            Fl::focus(nullptr);  // Clear focus from any input
            break;
    }
    return Fl_Toggle_Button::handle(event);
}

void HoverToggleButton::draw() {
    if (m_hover) {
        // Temporarily lighten color, draw with theme, then restore
        Fl_Color orig = color();
        color(fl_lighter(orig));
        Fl_Toggle_Button::draw();
        color(orig);
    } else {
        Fl_Toggle_Button::draw();
    }
}

// ScrollChoice implementation - a choice widget with scrollable popup
ScrollChoice::ScrollChoice(int x, int y, int w, int h, const char* label)
    : Fl_Widget(x, y, w, h, label)
{
}

void ScrollChoice::add(const char* text) {
    m_items.push_back(text ? text : "");
}

void ScrollChoice::clear() {
    m_items.clear();
    m_value = 0;
}

void ScrollChoice::value(int v) {
    if (v >= 0 && v < (int)m_items.size()) {
        m_value = v;
        redraw();
    }
}

const char* ScrollChoice::text() const {
    if (m_value >= 0 && m_value < (int)m_items.size()) {
        return m_items[m_value].c_str();
    }
    return "";
}

int ScrollChoice::handle(int event) {
    switch (event) {
        case FL_ENTER:
            m_hover = true;
            fl_cursor(FL_CURSOR_HAND);
            redraw();
            return 1;
        case FL_LEAVE:
            m_hover = false;
            fl_cursor(FL_CURSOR_DEFAULT);
            redraw();
            return 1;
        case FL_PUSH:
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                showPopup();
                return 1;
            }
            break;
    }
    return Fl_Widget::handle(event);
}

void ScrollChoice::draw() {
    // Draw button-like appearance
    Fl_Color col = m_hover ? fl_lighter(FL_BACKGROUND_COLOR) : FL_BACKGROUND_COLOR;
    fl_draw_box(FL_UP_BOX, x(), y(), w(), h(), col);

    // Draw text
    fl_color(FL_FOREGROUND_COLOR);
    fl_font(FL_HELVETICA, m_textsize);
    int tx = x() + 6;
    int tw = w() - 20;
    if (!m_items.empty() && m_value >= 0 && m_value < (int)m_items.size()) {
        fl_draw(m_items[m_value].c_str(), tx, y(), tw, h(), FL_ALIGN_LEFT);
    }

    // Draw dropdown arrow
    int ax = x() + w() - 14;
    int ay = y() + h() / 2;
    fl_color(FL_FOREGROUND_COLOR);
    fl_polygon(ax, ay - 3, ax + 6, ay - 3, ax + 3, ay + 3);
}

// Static callback for popup browser selection
void ScrollChoice::popupCallback(Fl_Widget* w, void* data) {
    Fl_Hold_Browser* browser = static_cast<Fl_Hold_Browser*>(w);
    Fl_Window* popup = browser->window();
    // Store selection in popup's user_data
    popup->user_data(reinterpret_cast<void*>(static_cast<intptr_t>(browser->value())));
    popup->hide();
}

void ScrollChoice::showPopup() {
    if (m_items.empty()) return;

    // Calculate popup height
    int itemH = m_textsize + 6;
    int popupH = (int)m_items.size() * itemH + 4;
    if (popupH > m_maxHeight) popupH = m_maxHeight;

    // Convert widget coordinates to screen coordinates
    int screenX = 0, screenY = 0;
    Fl_Window* win = window();
    if (win) {
        screenX = win->x() + x();
        screenY = win->y() + y();
    }

    // Create popup window over the widget (replacing it visually)
    int popupY = screenY;
    int screenH = Fl::h();
    if (popupY + popupH > screenH) {
        popupY = screenH - popupH;
        if (popupY < 0) popupY = 0;
    }

    Fl_Menu_Window* popup = new Fl_Menu_Window(screenX, popupY, w(), popupH);
    popup->set_modal();
    popup->set_override();  // Bypass window manager - no animations
    popup->clear_border();
    popup->user_data(nullptr);  // Will store selection

    Fl_Hold_Browser* browser = new Fl_Hold_Browser(0, 0, w(), popupH);
    browser->textsize(m_textsize);
    browser->has_scrollbar(Fl_Browser_::VERTICAL);

    for (const auto& item : m_items) {
        browser->add(item.c_str());
    }

    if (m_value >= 0 && m_value < (int)m_items.size()) {
        browser->value(m_value + 1);
        browser->middleline(m_value + 1);
    }

    // Use callback for selection - only trigger on release (click complete)
    browser->when(FL_WHEN_RELEASE_ALWAYS);
    browser->callback(popupCallback);

    popup->end();
    popup->show();

    // Wait for initial mouse release before grabbing (avoids immediate close)
    while (popup->shown() && Fl::event() != FL_RELEASE) {
        Fl::wait(0.01);
    }
    Fl::wait(0.01);  // Process the release

    // Now grab - any click outside will close the popup
    if (popup->shown()) {
        Fl::grab(popup);
    }

    // Run modal loop
    while (popup->shown()) {
        Fl::wait();

        // Check if clicked outside popup (grab sends events to popup)
        if (Fl::event() == FL_PUSH) {
            int ex = Fl::event_x_root();
            int ey = Fl::event_y_root();
            if (ex < popup->x() || ex >= popup->x() + popup->w() ||
                ey < popup->y() || ey >= popup->y() + popup->h()) {
                popup->hide();
            }
        }
    }

    Fl::grab(nullptr);

    // Get selection from user_data
    int sel = static_cast<int>(reinterpret_cast<intptr_t>(popup->user_data()));

    if (sel > 0) {
        m_value = sel - 1;
        redraw();
        if (m_callback) {
            m_callback(this, m_userdata);
        }
    }

    delete popup;

    // Reset cursor and hover state to avoid brief hand cursor flash
    m_hover = false;
    fl_cursor(FL_CURSOR_DEFAULT);
}

// LogDisplay implementation
LogDisplay::LogDisplay(int x, int y, int w, int h)
    : Fl_Text_Display(x, y, w, h)
    , m_gui(nullptr)
{
}

void LogDisplay::checkScrollPosition() {
    if (m_gui) {
        if (isAtBottom()) {
            m_gui->enableAutoScroll();
        } else {
            m_gui->disableAutoScroll();
        }
    }
}

int LogDisplay::handle(int event) {
    // Check if event is inside this widget
    bool mouseInside = Fl::event_inside(this);

    if (event == FL_MOUSEWHEEL) {
        if (mouseInside) {
            int result = Fl_Text_Display::handle(event);
            checkScrollPosition();
            return result;
        }
        return 0;  // Not inside - don't handle, let other widgets get it
    }

    // Let parent handle the event first
    int result = Fl_Text_Display::handle(event);

    // Check scroll position after any mouse event that could affect scrolling
    if (event == FL_PUSH || event == FL_RELEASE || event == FL_DRAG) {
        checkScrollPosition();
    }

    return result;
}

void LogDisplay::scrollToBottom() {
    if (buffer()) {
        // Move cursor to end of buffer and scroll to show it
        insert_position(buffer()->length());
        show_insert_position();
    }
}

bool LogDisplay::isAtBottom() const {
    if (!buffer()) return true;

    // Check if scrollbar is at or near the bottom
    const Fl_Scrollbar* vsb = mVScrollBar;
    if (vsb && vsb->visible()) {
        // value() is current position, maximum() is max scroll,
        // slider_size() is the visible portion
        double maxScroll = vsb->maximum() - vsb->slider_size();
        if (maxScroll <= 0) return true;  // Content fits, always "at bottom"
        return (vsb->value() >= maxScroll - 1);  // Within 1 line of bottom
    }
    return true;  // No scrollbar means content fits
}

// PlaceholderInput implementation
PlaceholderInput::PlaceholderInput(int x, int y, int w, int h, const char* label)
    : Fl_Input(x, y, w, h, label)
{
}

int PlaceholderInput::handle(int event) {
    if (event == FL_KEYDOWN && m_gui) {
        int key = Fl::event_key();
        if (key == FL_Up) {
            m_gui->historyUp();
            return 1;
        } else if (key == FL_Down) {
            m_gui->historyDown();
            return 1;
        }
    }
    return Fl_Input::handle(event);
}

void PlaceholderInput::draw() {
    Fl_Input::draw();
    // Draw placeholder if empty and not focused
    if (size() == 0 && !m_placeholder.empty() && Fl::focus() != this) {
        fl_color(fl_rgb_color(160, 160, 160));
        fl_font(textfont(), textsize());
        int tx = x() + Fl::box_dx(box()) + 3;
        int ty = y() + Fl::box_dy(box());
        int tw = w() - Fl::box_dw(box()) - 6;
        int th = h() - Fl::box_dh(box());
        fl_draw(m_placeholder.c_str(), tx, ty, tw, th, FL_ALIGN_LEFT);
    }
}

FltkGUI& FltkGUI::getInstance() {
    static FltkGUI instance;
    return instance;
}

FltkGUI::FltkGUI()
    : m_window(nullptr)
    , m_titleBar(nullptr)
    , m_statusGroup(nullptr)
    , m_hostnameLabel(nullptr)
    , m_mapLabel(nullptr)
    , m_playersLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_entitiesLabel(nullptr)
    , m_controlsGroup(nullptr)
    , m_mapChoice(nullptr)
    , m_changeMapBtn(nullptr)
    , m_restartBtn(nullptr)
    , m_playersGroup(nullptr)
    , m_playerTable(nullptr)
    , m_logsGroup(nullptr)
    , m_logDisplay(nullptr)
    , m_logBuffer(nullptr)
    , m_autoScrollBtn(nullptr)
    , m_commandInput(nullptr)
    , m_initialized(false)
    , m_visible(false)
    , m_playerCount(0)
    , m_maxPlayers(0)
    , m_entityCount(0)
    , m_serverTime(0)
    , m_timeLimit(0)
    , m_selectedMapIndex(0)
    , m_autoScroll(true)
    , m_historyIndex(-1)
    , m_origStdout(-1)
    , m_origStderr(-1)
    , m_captureActive(false)
{
    m_stdoutPipe[0] = m_stdoutPipe[1] = -1;
    m_stderrPipe[0] = m_stderrPipe[1] = -1;
}

FltkGUI::~FltkGUI() {
    shutdown();
}

bool FltkGUI::initialize() {
    if (m_initialized) {
        return true;
    }

    Fl::scheme("gleam");

    setupOutputCapture();

    m_initialized = true;
    return true;
}

void FltkGUI::shutdown() {
    if (!m_initialized) {
        return;
    }

    hide();
    cleanupOutputCapture();

    m_initialized = false;
}

void FltkGUI::createWindow() {
    if (m_window) {
        return;
    }

    int winW = 750;
    int winH = 600;
    int margin = 10;
    int paneSpacing = 8;

    m_window = new Fl_Double_Window(winW, winH, "HLDS Command Center");
    m_window->callback(onWindowClose, this);

    // Set window icon
    Fl_Pixmap* iconPixmap = new Fl_Pixmap(icon);
    Fl_RGB_Image* iconImage = new Fl_RGB_Image(iconPixmap);
    m_window->icon(iconImage);

    // Title bar
    m_titleBar = new Fl_Box(0, 0, winW, 28, "HLDS Command Center");
    m_titleBar->box(FL_FLAT_BOX);
    m_titleBar->color(fl_rgb_color(74, 144, 217));
    m_titleBar->labelcolor(FL_WHITE);
    m_titleBar->labelfont(FL_HELVETICA_BOLD);
    m_titleBar->labelsize(14);
    m_titleBar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    // Top row heights and positions
    int topRowY = 35;
    int topRowHeight = 110;

    // Server Status pane
    int statusW = 280;
    m_statusGroup = new Fl_Group(margin, topRowY, statusW, topRowHeight, "Server Status");
    m_statusGroup->box(FL_BORDER_BOX);
    m_statusGroup->align(FL_ALIGN_TOP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    m_statusGroup->labelsize(12);
    m_statusGroup->labelfont(FL_HELVETICA_BOLD);
    {
        int labelX = margin + 8;
        int labelY = topRowY + 22;
        int labelH = 16;
        int labelW = statusW - 16;

        m_hostnameLabel = new Fl_Box(labelX, labelY, labelW, labelH, "Host: Unknown");
        m_hostnameLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        m_hostnameLabel->labelsize(11);
        labelY += labelH;

        m_mapLabel = new Fl_Box(labelX, labelY, labelW, labelH, "Map: Unknown");
        m_mapLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        m_mapLabel->labelsize(11);
        labelY += labelH;

        m_playersLabel = new Fl_Box(labelX, labelY, labelW, labelH, "Players: 0 / 0");
        m_playersLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        m_playersLabel->labelsize(11);
        labelY += labelH;

        m_timeLabel = new Fl_Box(labelX, labelY, labelW, labelH, "Time: 0:00");
        m_timeLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        m_timeLabel->labelsize(11);
        labelY += labelH;

        m_entitiesLabel = new Fl_Box(labelX, labelY, labelW, labelH, "Entities: 0");
        m_entitiesLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        m_entitiesLabel->labelsize(11);
    }
    m_statusGroup->end();

    // Controls pane
    int controlsX = margin + statusW + paneSpacing;
    int controlsW = winW - margin * 2 - statusW - paneSpacing;
    m_controlsGroup = new Fl_Group(controlsX, topRowY, controlsW, topRowHeight, "Controls");
    m_controlsGroup->box(FL_BORDER_BOX);
    m_controlsGroup->align(FL_ALIGN_TOP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    m_controlsGroup->labelsize(12);
    m_controlsGroup->labelfont(FL_HELVETICA_BOLD);
    {
        int btnY = topRowY + 25;
        int btnH = 25;

        // Map dropdown with scrollable popup
        m_mapChoice = new ScrollChoice(controlsX + 8, btnY, 200, btnH, "");
        m_mapChoice->callback(onMapChoice, this);
        m_mapChoice->textsize(11);
        m_mapChoice->maxHeight(300);

        m_changeMapBtn = new HoverButton(controlsX + 215, btnY, 95, btnH, "Change Map");
        m_changeMapBtn->callback(onChangeMapBtn, this);
        m_changeMapBtn->labelsize(11);
        m_changeMapBtn->visible_focus(0);

        btnY += btnH + 10;

        m_restartBtn = new HoverButton(controlsX + 8, btnY, 75, btnH, "Restart");
        m_restartBtn->callback(onRestartBtn, this);
        m_restartBtn->labelsize(11);
        m_restartBtn->visible_focus(0);
    }
    m_controlsGroup->end();

    // Players pane
    int playerPaneY = topRowY + topRowHeight + paneSpacing;
    int playerPaneH = 200;
    int playerPaneW = winW - margin * 2;
    m_playersGroup = new Fl_Group(margin, playerPaneY, playerPaneW, playerPaneH, "Players");
    m_playersGroup->box(FL_BORDER_BOX);
    m_playersGroup->align(FL_ALIGN_TOP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    m_playersGroup->labelsize(12);
    m_playersGroup->labelfont(FL_HELVETICA_BOLD);
    {
        m_playerTable = new PlayerTable(margin + 4, playerPaneY + 18, playerPaneW - 8, playerPaneH - 22);
        m_playerTable->setKickCallback([this](int idx) { doKickPlayer(idx); });
        m_playerTable->setBanCallback([this](int idx) { doBanPlayer(idx); });
    }
    m_playersGroup->end();

    // Logs pane
    int logPaneY = playerPaneY + playerPaneH + paneSpacing;
    int commandInputH = 30;
    int logPaneH = winH - logPaneY - margin - commandInputH - paneSpacing;
    int logPaneW = winW - margin * 2;
    m_logsGroup = new Fl_Group(margin, logPaneY, logPaneW, logPaneH, "Server Logs");
    m_logsGroup->box(FL_BORDER_BOX);
    m_logsGroup->align(FL_ALIGN_TOP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    m_logsGroup->labelsize(12);
    m_logsGroup->labelfont(FL_HELVETICA_BOLD);
    {
        // Create text buffer and display for selectable/copyable text
        m_logBuffer = new Fl_Text_Buffer();
        int scrollbarW = Fl::scrollbar_size();
        int btnSize = scrollbarW + 2;  // Button is 2px larger than scrollbar
        int displayW = logPaneW - 8;
        int displayH = logPaneH - 22 - btnSize;  // Reduce height to make room for button row
        m_logDisplay = new LogDisplay(margin + 4, logPaneY + 18, displayW, displayH);
        m_logDisplay->setGui(this);
        m_logDisplay->buffer(m_logBuffer);
        m_logDisplay->textsize(11);
        m_logDisplay->textfont(FL_COURIER);
        m_logDisplay->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);

        // Auto-scroll toggle button in lower right corner, below the display
        int btnX = margin + 4 + displayW - btnSize;
        int btnY = logPaneY + 18 + displayH;
        m_autoScrollBtn = new HoverToggleButton(btnX, btnY, btnSize, btnSize, "A");
        m_autoScrollBtn->value(1);  // Start with auto-scroll enabled
        m_autoScrollBtn->callback(onAutoScrollToggle, this);
        m_autoScrollBtn->labelsize(10);
        m_autoScrollBtn->tooltip("Auto-scroll");
        m_autoScrollBtn->visible_focus(0);
    }
    m_logsGroup->end();

    // Command input
    int cmdY = logPaneY + logPaneH + paneSpacing;
    m_commandInput = new PlaceholderInput(margin + 12, cmdY, winW - margin * 2 - 12, commandInputH - 4, ">");
    m_commandInput->setPlaceholder("Enter command...");
    m_commandInput->setGui(this);
    m_commandInput->textsize(12);
    m_commandInput->box(FL_BORDER_BOX);
    m_commandInput->when(FL_WHEN_ENTER_KEY_ALWAYS);
    m_commandInput->callback(onCommandInput, this);

    m_window->end();
    m_window->resizable(m_logsGroup);

    // Load data
    loadMapCycle();
    refresh();

    // Start refresh timer
    Fl::add_timeout(1.0, onRefreshTimer, this);
}

void FltkGUI::destroyWindow() {
    if (!m_window) {
        return;
    }

    Fl::remove_timeout(onRefreshTimer, this);

    delete m_window;
    m_window = nullptr;
    m_titleBar = nullptr;
    m_statusGroup = nullptr;
    m_hostnameLabel = nullptr;
    m_mapLabel = nullptr;
    m_playersLabel = nullptr;
    m_timeLabel = nullptr;
    m_entitiesLabel = nullptr;
    m_controlsGroup = nullptr;
    m_mapChoice = nullptr;
    m_changeMapBtn = nullptr;
    m_restartBtn = nullptr;
    m_playersGroup = nullptr;
    m_playerTable = nullptr;
    m_logsGroup = nullptr;
    m_logDisplay = nullptr;
    m_autoScrollBtn = nullptr;
    // Note: m_logBuffer is owned by the text display and deleted with it
    m_logBuffer = nullptr;
    m_commandInput = nullptr;
}

void FltkGUI::show() {
    if (!m_initialized || m_visible) {
        return;
    }

    createWindow();
    m_window->show();
    m_visible = true;
}

void FltkGUI::hide() {
    if (!m_visible) {
        return;
    }

    destroyWindow();
    m_visible = false;
}

void FltkGUI::processEvents() {
    if (!m_initialized) {
        return;
    }

    readCapturedOutput();

    if (!m_visible || !m_window) {
        return;
    }

    Fl::check();  // Process pending events
}

void FltkGUI::onRefreshTimer(void* data) {
    FltkGUI* gui = static_cast<FltkGUI*>(data);
    if (gui->m_visible && gui->m_window) {
        gui->refresh();
        Fl::repeat_timeout(1.0, onRefreshTimer, data);
    }
}

void FltkGUI::refresh() {
    refreshServerStatus();
    refreshPlayerList();
}

void FltkGUI::refreshServerStatus() {
    const char* hostname = g_engfuncs.pfnCVarGetString("hostname");
    m_hostname = hostname ? hostname : "Unknown";
    const char* mapname = gpGlobals ? STRING(gpGlobals->mapname) : nullptr;
    m_currentMap = (mapname && mapname[0]) ? mapname : "Unknown";
    m_maxPlayers = gpGlobals->maxClients;
    m_serverTime = gpGlobals->time;
    m_timeLimit = g_engfuncs.pfnCVarGetFloat("mp_timelimit");
    m_entityCount = g_engfuncs.pfnNumberOfEntities();

    m_playerCount = 0;
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(i);
        if (pEdict && !pEdict->free && pEdict->pvPrivateData) {
            const char* name = STRING(pEdict->v.netname);
            if (name && name[0]) m_playerCount++;
        }
    }

    // Update labels
    if (m_hostnameLabel) {
        static char buf[128];
        std::string host = m_hostname.length() > 28 ? m_hostname.substr(0, 25) + "..." : m_hostname;
        snprintf(buf, sizeof(buf), "Host: %s", host.c_str());
        m_hostnameLabel->copy_label(buf);
    }
    if (m_mapLabel) {
        static char buf[64];
        snprintf(buf, sizeof(buf), "Map: %s", m_currentMap.c_str());
        m_mapLabel->copy_label(buf);
    }
    if (m_playersLabel) {
        static char buf[64];
        snprintf(buf, sizeof(buf), "Players: %d / %d", m_playerCount, m_maxPlayers);
        m_playersLabel->copy_label(buf);
    }
    if (m_timeLabel) {
        static char buf[64];
        int minutes = (int)(m_serverTime / 60);
        int seconds = (int)m_serverTime % 60;
        if (m_timeLimit > 0) {
            snprintf(buf, sizeof(buf), "Time: %d:%02d / %.0f:00", minutes, seconds, m_timeLimit);
        } else {
            snprintf(buf, sizeof(buf), "Time: %d:%02d", minutes, seconds);
        }
        m_timeLabel->copy_label(buf);
    }
    if (m_entitiesLabel) {
        static char buf[64];
        snprintf(buf, sizeof(buf), "Entities: %d", m_entityCount);
        m_entitiesLabel->copy_label(buf);
    }

    if (m_window) {
        m_window->redraw();
    }
}

void FltkGUI::refreshPlayerList() {
    if (!m_playerTable) return;

    std::vector<PlayerInfo> players;

    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(i);
        if (pEdict && !pEdict->free && pEdict->pvPrivateData) {
            const char* name = STRING(pEdict->v.netname);
            if (name && name[0]) {
                PlayerInfo info;
                info.index = i;
                info.name = name;

                int ping = 0, loss = 0;
                g_engfuncs.pfnGetPlayerStats(pEdict, &ping, &loss);
                info.ping = ping;
                info.health = (int)pEdict->v.health;
                info.frags = (int)pEdict->v.frags;

                const char* authid = g_engfuncs.pfnGetPlayerAuthId(pEdict);
                info.steamId = authid ? authid : "N/A";

                players.push_back(info);
            }
        }
    }

    m_playerTable->setPlayers(players);
}

void FltkGUI::loadMapCycle() {
    m_maps.clear();
    m_selectedMapIndex = 0;

    // Get game directory and build maps path
    char gameDir[256];
    g_engfuncs.pfnGetGameDir(gameDir);
    char mapsPath[512];
    snprintf(mapsPath, sizeof(mapsPath), "%s/maps", gameDir);

    // Scan for .bsp files
    DIR* dir = opendir(mapsPath);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            const char* name = entry->d_name;
            size_t len = strlen(name);

            // Check for .bsp extension
            if (len > 4 && strcasecmp(name + len - 4, ".bsp") == 0) {
                // Extract map name without .bsp extension
                char mapname[64];
                size_t mapLen = len - 4;
                if (mapLen >= sizeof(mapname)) mapLen = sizeof(mapname) - 1;
                strncpy(mapname, name, mapLen);
                mapname[mapLen] = '\0';

                m_maps.push_back(mapname);
            }
        }
        closedir(dir);
    }

    // Sort maps alphabetically
    std::sort(m_maps.begin(), m_maps.end());

    const char* currentMapName = gpGlobals ? STRING(gpGlobals->mapname) : nullptr;
    if (currentMapName && !currentMapName[0]) {
        currentMapName = nullptr;
    }

    if (m_maps.empty() && currentMapName) {
        m_maps.push_back(currentMapName);
    }

    // Populate dropdown
    if (m_mapChoice) {
        m_mapChoice->clear();
        for (size_t i = 0; i < m_maps.size(); i++) {
            m_mapChoice->add(m_maps[i].c_str());
        }
        m_mapChoice->value(m_selectedMapIndex);
    }
}

void FltkGUI::selectCurrentMap() {
    if (!m_mapChoice || m_maps.empty()) return;

    const char* currentMapName = gpGlobals ? STRING(gpGlobals->mapname) : nullptr;
    if (!currentMapName || !currentMapName[0]) return;

    for (int i = 0; i < (int)m_maps.size(); i++) {
        if (strcasecmp(m_maps[i].c_str(), currentMapName) == 0) {
            m_selectedMapIndex = i;
            m_mapChoice->value(m_selectedMapIndex);
            break;
        }
    }
}

void FltkGUI::historyUp() {
    if (!m_commandInput || m_commandHistory.empty()) return;

    if (m_historyIndex == -1) {
        // Save current input before navigating
        m_savedInput = m_commandInput->value() ? m_commandInput->value() : "";
        m_historyIndex = (int)m_commandHistory.size() - 1;
    } else if (m_historyIndex > 0) {
        m_historyIndex--;
    }

    m_commandInput->value(m_commandHistory[m_historyIndex].c_str());
    m_commandInput->insert_position(m_commandInput->size());
}

void FltkGUI::historyDown() {
    if (!m_commandInput || m_historyIndex == -1) return;

    if (m_historyIndex < (int)m_commandHistory.size() - 1) {
        m_historyIndex++;
        m_commandInput->value(m_commandHistory[m_historyIndex].c_str());
    } else {
        // Restore saved input
        m_historyIndex = -1;
        m_commandInput->value(m_savedInput.c_str());
    }
    m_commandInput->insert_position(m_commandInput->size());
}

// Callbacks
void FltkGUI::onWindowClose(Fl_Widget*, void* data) {
    FltkGUI* gui = static_cast<FltkGUI*>(data);
    gui->hide();
    g_engfuncs.pfnServerCommand("quit\n");
    g_engfuncs.pfnServerExecute();
}

void FltkGUI::onChangeMapBtn(Fl_Widget*, void* data) {
    static_cast<FltkGUI*>(data)->doChangeMap();
}

void FltkGUI::onRestartBtn(Fl_Widget*, void* data) {
    static_cast<FltkGUI*>(data)->doRestart();
}

void FltkGUI::onMapChoice(Fl_Widget*, void* data) {
    static_cast<FltkGUI*>(data)->doMapSelect();
}

void FltkGUI::onCommandInput(Fl_Widget*, void* data) {
    static_cast<FltkGUI*>(data)->doCommand();
}

void FltkGUI::onAutoScrollToggle(Fl_Widget* w, void* data) {
    FltkGUI* gui = static_cast<FltkGUI*>(data);
    Fl_Toggle_Button* btn = static_cast<Fl_Toggle_Button*>(w);
    gui->m_autoScroll = btn->value() != 0;
}

void FltkGUI::disableAutoScroll() {
    m_autoScroll = false;
    if (m_autoScrollBtn) {
        m_autoScrollBtn->value(0);
    }
}

void FltkGUI::enableAutoScroll() {
    m_autoScroll = true;
    if (m_autoScrollBtn) {
        m_autoScrollBtn->value(1);
    }
}

// Actions
void FltkGUI::doChangeMap() {
    if (m_maps.empty() || m_selectedMapIndex < 0 || m_selectedMapIndex >= (int)m_maps.size()) {
        return;
    }

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "changelevel %s\n", m_maps[m_selectedMapIndex].c_str());
    g_engfuncs.pfnServerCommand(cmd);
    g_engfuncs.pfnServerExecute();

    char log[128];
    snprintf(log, sizeof(log), "Changing map to: %s", m_maps[m_selectedMapIndex].c_str());
    appendLog(log);
}

void FltkGUI::doRestart() {
    g_engfuncs.pfnServerCommand("restart\n");
    g_engfuncs.pfnServerExecute();
    appendLog("Map restarted");
}

void FltkGUI::doMapSelect() {
    if (m_mapChoice) {
        m_selectedMapIndex = m_mapChoice->value();
    }
}

void FltkGUI::doCommand() {
    if (!m_commandInput) return;

    const char* text = m_commandInput->value();
    if (!text || !text[0]) return;

    std::string cmd = text;

    // Add to command history
    if (m_commandHistory.empty() || m_commandHistory.back() != cmd) {
        m_commandHistory.push_back(cmd);
        if (m_commandHistory.size() > MAX_HISTORY) {
            m_commandHistory.erase(m_commandHistory.begin());
        }
    }
    m_historyIndex = -1;
    m_savedInput.clear();

    // Log the command
    std::string logEntry = "> " + cmd + "\n";
    m_logLines.push_back(logEntry);
    while (m_logLines.size() > MAX_LOG_LINES) {
        m_logLines.pop_front();
    }

    if (m_logBuffer) {
        m_logBuffer->append(logEntry.c_str());
        if (m_autoScroll && m_logDisplay) {
            m_logDisplay->scrollToBottom();
        }
    }

    // Execute
    cmd += "\n";
    g_engfuncs.pfnServerCommand(cmd.c_str());
    g_engfuncs.pfnServerExecute();

    // Clear input
    m_commandInput->value("");
}

void FltkGUI::doKickPlayer(int playerIndex) {
    edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(playerIndex);
    if (!pEdict || pEdict->free || !pEdict->pvPrivateData) {
        appendLog("Player not found");
        return;
    }

    const char* name = STRING(pEdict->v.netname);
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "kick #%d\n", g_engfuncs.pfnGetPlayerUserId(pEdict));
    g_engfuncs.pfnServerCommand(cmd);
    g_engfuncs.pfnServerExecute();

    char log[128];
    snprintf(log, sizeof(log), "Kicked: %s", name ? name : "Unknown");
    appendLog(log);
    refresh();
}

void FltkGUI::doBanPlayer(int playerIndex) {
    edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(playerIndex);
    if (!pEdict || pEdict->free || !pEdict->pvPrivateData) {
        appendLog("Player not found");
        return;
    }

    const char* name = STRING(pEdict->v.netname);
    int userId = g_engfuncs.pfnGetPlayerUserId(pEdict);

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "banid 0 #%d kick\n", userId);
    g_engfuncs.pfnServerCommand(cmd);
    g_engfuncs.pfnServerExecute();

    g_engfuncs.pfnServerCommand("writeid\n");
    g_engfuncs.pfnServerExecute();

    char log[128];
    snprintf(log, sizeof(log), "Banned: %s", name ? name : "Unknown");
    appendLog(log);
    refresh();
}

void FltkGUI::appendLog(const char* text) {
    if (!text || !text[0]) return;

    char timestamped[512];
    int minutes = (int)(m_serverTime / 60);
    int seconds = (int)m_serverTime % 60;
    snprintf(timestamped, sizeof(timestamped), "[%d:%02d] %s\n", minutes, seconds, text);

    m_logLines.push_back(timestamped);
    while (m_logLines.size() > MAX_LOG_LINES) {
        m_logLines.pop_front();
    }

    if (m_logBuffer && m_visible) {
        m_logBuffer->append(timestamped);
        if (m_autoScroll && m_logDisplay) {
            m_logDisplay->scrollToBottom();
        }
    }
}

void FltkGUI::setupOutputCapture() {
    if (m_captureActive) return;

    if (pipe(m_stdoutPipe) == -1) {
        return;
    }
    if (pipe(m_stderrPipe) == -1) {
        close(m_stdoutPipe[0]);
        close(m_stdoutPipe[1]);
        m_stdoutPipe[0] = m_stdoutPipe[1] = -1;
        return;
    }

    m_origStdout = dup(STDOUT_FILENO);
    m_origStderr = dup(STDERR_FILENO);

    dup2(m_stdoutPipe[1], STDOUT_FILENO);
    dup2(m_stderrPipe[1], STDERR_FILENO);

    int flags = fcntl(m_stdoutPipe[0], F_GETFL, 0);
    fcntl(m_stdoutPipe[0], F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(m_stderrPipe[0], F_GETFL, 0);
    fcntl(m_stderrPipe[0], F_SETFL, flags | O_NONBLOCK);

    m_captureActive = true;
}

void FltkGUI::cleanupOutputCapture() {
    if (!m_captureActive) return;

    if (m_origStdout != -1) {
        dup2(m_origStdout, STDOUT_FILENO);
        close(m_origStdout);
        m_origStdout = -1;
    }
    if (m_origStderr != -1) {
        dup2(m_origStderr, STDERR_FILENO);
        close(m_origStderr);
        m_origStderr = -1;
    }

    if (m_stdoutPipe[0] != -1) { close(m_stdoutPipe[0]); m_stdoutPipe[0] = -1; }
    if (m_stdoutPipe[1] != -1) { close(m_stdoutPipe[1]); m_stdoutPipe[1] = -1; }
    if (m_stderrPipe[0] != -1) { close(m_stderrPipe[0]); m_stderrPipe[0] = -1; }
    if (m_stderrPipe[1] != -1) { close(m_stderrPipe[1]); m_stderrPipe[1] = -1; }

    m_captureActive = false;
}

void FltkGUI::readCapturedOutput() {
    if (!m_captureActive) return;

    char buffer[4096];
    ssize_t bytesRead;
    bool gotOutput = false;

    while ((bytesRead = read(m_stdoutPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        m_partialLine += buffer;
        gotOutput = true;
        // Pass through to original stdout
        if (m_origStdout != -1) {
            write(m_origStdout, buffer, bytesRead);
        }
    }

    while ((bytesRead = read(m_stderrPipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        m_partialLine += buffer;
        gotOutput = true;
        // Pass through to original stderr
        if (m_origStderr != -1) {
            write(m_origStderr, buffer, bytesRead);
        }
    }

    if (gotOutput) {
        size_t pos;
        while ((pos = m_partialLine.find('\n')) != std::string::npos) {
            std::string line = m_partialLine.substr(0, pos);
            m_partialLine = m_partialLine.substr(pos + 1);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) continue;

            line += "\n";
            m_logLines.push_back(line);
            while (m_logLines.size() > MAX_LOG_LINES) {
                m_logLines.pop_front();
            }

            if (m_logBuffer && m_visible) {
                m_logBuffer->append(line.c_str());
                if (m_autoScroll && m_logDisplay) {
                    m_logDisplay->scrollToBottom();
                }
            }
        }
    }
}
