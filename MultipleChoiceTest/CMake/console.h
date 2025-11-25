// console.h
#ifndef CONSOLE_H
#define CONSOLE_H

#include <windows.h>
#include <conio.h>

// Mã phím thường dùng
#define KEY_NONE   -1
#define KEY_ESC    27
#define KEY_ENTER  13
#define KEY_SPACE  32
#define KEY_DEL    83   // getch: 224, 83 (DEL)
#define KEY_INS    82   // getch: 224, 82
#define KEY_LEFT   75   // getch: 224, 75
#define KEY_RIGHT  77   // getch: 224, 77
#define KEY_UP     72   // getch: 224, 72
#define KEY_DOWN   80   // getch: 224, 80
#define KEY_F2     60   // getch: 0, 60
#define KEY_F5     63   // getch: 0, 63
#define KEY_F9     67   // getch: 0, 67

void gotoxy(short x, short y);
void setColor(WORD attr);
void clearScreen();
void hideCursor();
void showCursor();

// Trả key: phím ASCII hoặc mã mở rộng chuẩn ở trên
int readKey();

// In đồng hồ thời gian còn lại mm:ss tại (x,y)
void print_mmss_at(int x, int y, int seconds);

// Hộp thoại xác nhận Yes/No (Y/N). Trả true nếu Yes.
bool confirm_dialog(const char* message);

#endif // CONSOLE_H
