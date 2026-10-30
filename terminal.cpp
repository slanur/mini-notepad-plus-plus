// terminal.cpp

#include "terminal.h"
#include <cstdio>
#include <iostream>

#ifdef PLATFORM_WINDOWS
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  static HANDLE hStdin  = INVALID_HANDLE_VALUE;
  static HANDLE hStdout = INVALID_HANDLE_VALUE;
  static DWORD  oldStdinMode  = 0;
  static DWORD  oldStdoutMode = 0;
#else
  #include <termios.h>
  #include <unistd.h>
  #include <sys/ioctl.h>
  static struct termios origTermios;
#endif

namespace Terminal {

void setUTF8() {
#ifdef PLATFORM_WINDOWS
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif
}

void enableRawMode() {
#ifdef PLATFORM_WINDOWS
    hStdin  = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(hStdin,  &oldStdinMode);
    GetConsoleMode(hStdout, &oldStdoutMode);
    SetConsoleMode(hStdin, ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS);
    DWORD newOut = oldStdoutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hStdout, newOut);
#else
    tcgetattr(STDIN_FILENO, &origTermios);
    struct termios raw = origTermios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

void disableRawMode() {
#ifdef PLATFORM_WINDOWS
    if (hStdin  != INVALID_HANDLE_VALUE) SetConsoleMode(hStdin,  oldStdinMode);
    if (hStdout != INVALID_HANDLE_VALUE) SetConsoleMode(hStdout, oldStdoutMode);
#else
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
#endif
}

int readKey() {
#ifdef PLATFORM_WINDOWS
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD ir;
    DWORD read;

    while (true) {
        ReadConsoleInputA(hIn, &ir, 1, &read);
        if (ir.EventType != KEY_EVENT) continue;
        if (!ir.Event.KeyEvent.bKeyDown) continue;

        WORD  vk   = ir.Event.KeyEvent.wVirtualKeyCode;
        DWORD ctrl = ir.Event.KeyEvent.dwControlKeyState;
        char  ch   = ir.Event.KeyEvent.uChar.AsciiChar;

        bool isCtrl  = (ctrl & (LEFT_CTRL_PRESSED  | RIGHT_CTRL_PRESSED)) != 0;
        bool isShift = (ctrl & SHIFT_PRESSED) != 0;

        // Yon tuslari
        if (vk == VK_LEFT) {
            if (isCtrl && isShift) return KEY_CTRL_SHIFT_LEFT;
            if (isCtrl)            return KEY_CTRL_LEFT;
            if (isShift)           return KEY_SHIFT_LEFT;
            return KEY_LEFT;
        }
        if (vk == VK_RIGHT) {
            if (isCtrl && isShift) return KEY_CTRL_SHIFT_RIGHT;
            if (isCtrl)            return KEY_CTRL_RIGHT;
            if (isShift)           return KEY_SHIFT_RIGHT;
            return KEY_RIGHT;
        }
        if (vk == VK_UP)   return isCtrl ? KEY_CTRL_UP   : KEY_UP;
        if (vk == VK_DOWN) return isCtrl ? KEY_CTRL_DOWN : KEY_DOWN;

        // Backspace / Ctrl+Backspace ayrimi
        // Normal Backspace : VK=8, isCtrl=0  -> KEY_BACKSPACE
        // Ctrl+Backspace   : VK=8, isCtrl=1  -> KEY_CTRL_BACKSPACE
        // (Ctrl+H Windows'ta Backspace VK'si ile catisir, bu yuzden Ctrl+R kullaniyoruz)
        if (vk == VK_BACK) {
            if (isCtrl) return KEY_CTRL_BACKSPACE;
            return KEY_BACKSPACE;
        }
        if (vk == VK_DELETE) return KEY_DELETE;
        if (vk == VK_RETURN) return KEY_ENTER;
        if (vk == VK_ESCAPE) return KEY_ESC;
        if (vk == VK_TAB)    return KEY_TAB;
        if (vk == VK_HOME)   return KEY_HOME;
        if (vk == VK_END)    return KEY_END;
        if (vk == VK_PRIOR)  return KEY_PAGE_UP;
        if (vk == VK_NEXT)   return KEY_PAGE_DOWN;

        // Ctrl + harf kombinasyonlari
        if (isCtrl) {
            if ((unsigned char)ch >= 32) return (unsigned char)ch;
            switch (vk) {
                case 'Z': return KEY_CTRL_Z;
                case 'F': return KEY_CTRL_F;
                case 'G': return KEY_CTRL_G;
                case 'C': return KEY_CTRL_C;
                case 'V': return KEY_CTRL_V;
                case 'X': return KEY_CTRL_X;
                case 'S': return KEY_CTRL_S;
                case 'O': return KEY_CTRL_O;
                case 'W': return KEY_CTRL_W;
                case 'R': return KEY_CTRL_H_KEY; // Ctrl+R = Degistir
                default: continue;
            }
        }

        // Normal karakter
        if ((unsigned char)ch >= 32) return (unsigned char)ch;
    }
#else
    // ── Unix / Mac ────────────────────────────────────────────────────────
    unsigned char buf[8] = {};
    int n = read(STDIN_FILENO, buf, 1);
    if (n <= 0) return KEY_NONE;

    if (buf[0] == 27) { // ESC dizisi
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_cc[VMIN]  = 0;
        t.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        n = read(STDIN_FILENO, buf + 1, 1);
        t.c_cc[VMIN]  = 1;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);

        if (n == 0) return KEY_ESC;

        if (buf[1] == '[') {
            n = read(STDIN_FILENO, buf + 2, 1);
            if (n == 0) return KEY_ESC;

            if (buf[2] == '1') {
                unsigned char extra[4] = {};
                read(STDIN_FILENO, extra, 3);
                if (extra[0] == ';') {
                    char mod = extra[1], dir = extra[2];
                    if (mod == '5') {
                        if (dir == 'C') return KEY_CTRL_RIGHT;
                        if (dir == 'D') return KEY_CTRL_LEFT;
                    }
                    if (mod == '2') {
                        if (dir == 'C') return KEY_SHIFT_RIGHT;
                        if (dir == 'D') return KEY_SHIFT_LEFT;
                    }
                    if (mod == '6') {
                        if (dir == 'C') return KEY_CTRL_SHIFT_RIGHT;
                        if (dir == 'D') return KEY_CTRL_SHIFT_LEFT;
                    }
                }
                return KEY_HOME;
            }
            switch (buf[2]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                case '3': { read(STDIN_FILENO, buf, 1); return KEY_DELETE; }
                case '5': { read(STDIN_FILENO, buf, 1); return KEY_PAGE_UP; }
                case '6': { read(STDIN_FILENO, buf, 1); return KEY_PAGE_DOWN; }
                default:  return KEY_NONE;
            }
        }
        return KEY_ESC;
    }

    // Ctrl + karakter (ASCII kontrol kodlari)
    switch (buf[0]) {
        case 3:   return KEY_CTRL_C;
        case 6:   return KEY_CTRL_F;
        case 7:   return KEY_CTRL_G;
        case 8:   return KEY_CTRL_BACKSPACE; // Ctrl+Backspace (bazi terminaller)
        case '\t': return KEY_TAB;
        case '\n':
        case '\r': return KEY_ENTER;
        case 15:  return KEY_CTRL_O;
        case 18:  return KEY_CTRL_H_KEY;  // Ctrl+R (ASCII 18) = Degistir
        case 19:  return KEY_CTRL_S;
        case 22:  return KEY_CTRL_V;
        case 23:  return KEY_CTRL_W;
        case 24:  return KEY_CTRL_X;
        case 26:  return KEY_CTRL_Z;
        case 127: return KEY_BACKSPACE;   // Normal Backspace (DEL)
    }

    return (int)(unsigned char)buf[0];
#endif
}

void moveCursor(int row, int col) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[%d;%dH", row, col);
    fputs(buf, stdout); fflush(stdout);
}

void clearScreen() { fputs("\033[2J\033[H", stdout); fflush(stdout); }
void clearLine()   { fputs("\033[2K", stdout);        fflush(stdout); }
void hideCursor()  { fputs("\033[?25l", stdout);      fflush(stdout); }
void showCursor()  { fputs("\033[?25h", stdout);      fflush(stdout); }
void setCursorBar()   { fputs("\033[5 q", stdout); fflush(stdout); }
void setCursorBlock() { fputs("\033[2 q", stdout); fflush(stdout); }

void setColor(Color fg, Color bg) {
    char buf[64];
    if (fg == COLOR_DEFAULT && bg == COLOR_DEFAULT) {
        fputs("\033[0m", stdout);
    } else if (bg == COLOR_DEFAULT) {
        // Sadece fg: once sifirla sonra fg'yi uygula
        int fgCode = (fg >= COLOR_BRIGHT_BLACK) ? (90 + fg - COLOR_BRIGHT_BLACK) : (30 + fg);
        snprintf(buf, sizeof(buf), "\033[0;%dm", fgCode);
        fputs(buf, stdout);
    } else {
        int fgCode = (fg >= COLOR_BRIGHT_BLACK) ? (90 + fg - COLOR_BRIGHT_BLACK) : (30 + fg);
        int bgCode = (bg >= COLOR_BRIGHT_BLACK) ? (100 + bg - COLOR_BRIGHT_BLACK) : (40 + bg);
        snprintf(buf, sizeof(buf), "\033[%d;%dm", fgCode, bgCode);
        fputs(buf, stdout);
    }
    fflush(stdout);
}

void resetColor() { fputs("\033[0m", stdout); fflush(stdout); }

void getSize(int &rows, int &cols) {
#ifdef PLATFORM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    cols = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) { rows = 24; cols = 80; }
    else { rows = ws.ws_row; cols = ws.ws_col; }
#endif
}

void print(const std::string &s) { fputs(s.c_str(), stdout); fflush(stdout); }
void printChar(char c)           { fputc(c, stdout);          fflush(stdout); }

} // namespace Terminal