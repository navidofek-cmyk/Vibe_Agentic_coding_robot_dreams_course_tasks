#include "terminal.h"
#include <unistd.h>

static void out(const char* s, size_t n) { if (write(STDOUT_FILENO, s, n) < 0) {} }
#include <sys/ioctl.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sstream>

static struct termios origTermios;
static bool rawEnabled = false;

namespace Term {

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &origTermios) == -1)
        throw std::runtime_error("tcgetattr failed");
    rawEnabled = true;

    struct termios raw = origTermios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |=  (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        throw std::runtime_error("tcsetattr failed");
}

void disableRawMode() {
    if (rawEnabled)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
}

void moveCursor(int row, int col) {
    // ANSI is 1-based
    std::string s = "\033[" + std::to_string(row + 1) + ";" + std::to_string(col + 1) + "H";
    out(s.c_str(), s.size());
}

void clearScreen() {
    out("\033[2J", 4);
    moveCursor(0, 0);
}

void clearLine() { out("\033[2K", 4); }
void hideCursor() { out("\033[?25l", 6); }
void showCursor() { out("\033[?25h", 6); }

void getSize(int& rows, int& cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        rows = 24; cols = 80;
    } else {
        rows = ws.ws_row;
        cols = ws.ws_col;
    }
}

std::string reset() { return "\033[0m"; }

std::string bold(const std::string& s)    { return "\033[1m" + s + "\033[0m"; }
std::string inverse(const std::string& s) { return "\033[7m" + s + "\033[0m"; }

std::string fgBlue(const std::string& s)   { return "\033[34m" + s + "\033[0m"; }
std::string fgCyan(const std::string& s)   { return "\033[36m" + s + "\033[0m"; }
std::string fgYellow(const std::string& s) { return "\033[33m" + s + "\033[0m"; }
std::string fgWhite(const std::string& s)  { return "\033[97m" + s + "\033[0m"; }

std::string bgBlue(const std::string& s)   { return "\033[44m" + s + "\033[0m"; }
std::string bgWhite(const std::string& s)  { return "\033[47m" + s + "\033[0m"; }
std::string bgCyan(const std::string& s)   { return "\033[46m" + s + "\033[0m"; }
std::string bgYellow(const std::string& s) { return "\033[43m" + s + "\033[0m"; }

std::string colorFg(int r, int g, int b, const std::string& s) {
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" +
           std::to_string(b) + "m" + s + "\033[0m";
}

std::string colorBg(int r, int g, int b, const std::string& s) {
    return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" +
           std::to_string(b) + "m" + s + "\033[0m";
}

int readKey() {
    char c;
    if (read(STDIN_FILENO, &c, 1) <= 0) return -1;

    if (c != '\033') return (unsigned char)c;

    // Escape sequence
    char seq[8] = {};
    if (read(STDIN_FILENO, &seq[0], 1) <= 0) return Key::ESC;
    if (read(STDIN_FILENO, &seq[1], 1) <= 0) return Key::ESC;

    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            char extra;
            if (read(STDIN_FILENO, &extra, 1) <= 0) return Key::ESC;
            if (extra == '~') {
                switch (seq[1]) {
                    case '1': return Key::HOME;
                    case '3': return Key::DEL;
                    case '4': return Key::END;
                    case '5': return Key::PAGE_UP;
                    case '6': return Key::PAGE_DOWN;
                    case '7': return Key::HOME;
                    case '8': return Key::END;
                }
            }
        } else {
            switch (seq[1]) {
                case 'A': return Key::ARROW_UP;
                case 'B': return Key::ARROW_DOWN;
                case 'C': return Key::ARROW_RIGHT;
                case 'D': return Key::ARROW_LEFT;
                case 'H': return Key::HOME;
                case 'F': return Key::END;
            }
        }
    } else if (seq[0] == 'O') {
        switch (seq[1]) {
            case 'H': return Key::HOME;
            case 'F': return Key::END;
        }
    }

    return Key::ESC;
}

} // namespace Term
