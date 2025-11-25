// console.cpp
#include "console.h"
#include <cstdio>

void gotoxy(short x, short y) {
    COORD c; c.X = x; c.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void setColor(WORD attr) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attr);
}

void clearScreen() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    DWORD count = csbi.dwSize.X * csbi.dwSize.Y;
    COORD home = { 0, 0 };
    DWORD written;
    FillConsoleOutputCharacter(hOut, ' ', count, home, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, count, home, &written);
    SetConsoleCursorPosition(hOut, home);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 20;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}
void showCursor() {
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 20;
    info.bVisible = TRUE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

int readKey() {
    if (!_kbhit()) return KEY_NONE;
    int c = _getch();
    if (c == 0 || c == 224) {
        int c2 = _getch();
        return c2;
    }
    return c;
}

void print_mmss_at(int x, int y, int seconds) {
    if (seconds < 0) seconds = 0;
    int mm = seconds / 60;
    int ss = seconds % 60;
    gotoxy((short)x, (short)y);
    printf("%02d:%02d", mm, ss);
}

bool confirm_dialog(const char* message) {
    printf("\n%s (Y/N)? ", message);
    int ch;
    do {
        ch = _getch();
        if (ch == 'Y' || ch == 'y') return true;
        if (ch == 'N' || ch == 'n' || ch == KEY_ESC) return false;
    } while (1);
}
