#pragma once
#include <string>
#include <termios.h>

namespace Term {

void enableRawMode();
void disableRawMode();

// Cursor & screen
void moveCursor(int row, int col);   // 0-based
void clearScreen();
void clearLine();
void hideCursor();
void showCursor();
void getSize(int& rows, int& cols);

// Styling — resets after each call unless combined manually
std::string bold(const std::string& s);
std::string colorFg(int r, int g, int b, const std::string& s);
std::string colorBg(int r, int g, int b, const std::string& s);
std::string reset();

// 256-color helpers for common palette
std::string fgBlue(const std::string& s);
std::string fgCyan(const std::string& s);
std::string fgYellow(const std::string& s);
std::string fgWhite(const std::string& s);
std::string bgBlue(const std::string& s);
std::string bgWhite(const std::string& s);
std::string bgCyan(const std::string& s);
std::string bgYellow(const std::string& s);
std::string inverse(const std::string& s);

// Input — returns key code (see Key::*)
int readKey();

} // namespace Term

namespace Key {
    constexpr int ARROW_UP    = 1000;
    constexpr int ARROW_DOWN  = 1001;
    constexpr int ARROW_LEFT  = 1002;
    constexpr int ARROW_RIGHT = 1003;
    constexpr int HOME        = 1004;
    constexpr int END         = 1005;
    constexpr int PAGE_UP     = 1006;
    constexpr int PAGE_DOWN   = 1007;
    constexpr int DEL         = 1008;
    constexpr int RESIZE      = 1009;
    constexpr int BACKSPACE   = 127;
    constexpr int ENTER       = '\r';
    constexpr int ESC         = 27;
    constexpr int CTRL_S      = 19;
    constexpr int CTRL_X      = 24;
    constexpr int CTRL_O      = 15;
    constexpr int CTRL_N      = 14;
    constexpr int CTRL_F      =  6;
    constexpr int CTRL_R      = 18;
    constexpr int CTRL_T      = 20;
    constexpr int CTRL_A      =  1;
} // namespace Key
