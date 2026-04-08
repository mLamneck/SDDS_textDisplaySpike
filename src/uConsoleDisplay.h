#ifndef UCONSOLEDISPLAY_H
#define UCONSOLEDISPLAY_H

#include "uAbstractTextDisplay.h"
#include <iostream>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
#endif

namespace sdds {
namespace textDisplaySpike {

template <int nRows, int nColumns>
class TconsoleDisplay : public TabstractTextDisplay<nRows, nColumns> {
public:
    constexpr static int SDDS_TDS_KEY_LEFT  = 75;
    constexpr static int SDDS_TDS_KEY_RIGHT = 77;
    constexpr static int SDDS_TDS_KEY_UP    = 72;
    constexpr static int SDDS_TDS_KEY_DOWN  = 80;
    constexpr static int SDDS_TDS_KEY_ESC   = 27;
    constexpr static int SDDS_TDS_KEY_ENTER = 13;

    int readKey() {
#ifdef _WIN32
        if (!_kbhit()) return 0;
        return _getch();
#else
        return readKeyUnix();
#endif
    }

private:

/* ============================================================
   Cursor & Screen handling (ANSI works on Linux + Windows)
   ============================================================ */

    void clearScreen() {
        std::cout << "\033[2J\033[H";
        std::cout.flush();
    }

    void gotoxy(int x, int y) {
        std::cout << "\033[" << y + 1 << ";" << x + 1 << "H";
        std::cout.flush();
    }

    void writeToConsole(int x, int y, const char* text, int length) {
#ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD position = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
        DWORD written;
        WriteConsoleOutputCharacterA(hConsole, text, length, position, &written);
#else
        gotoxy(x, y);
        std::cout.write(text, length);
        std::cout.flush();
        auto c = this->getCursor();
        gotoxy(c.x,c.y);
#endif
    }

/* ============================================================
   Linux / Unix keyboard input
   ============================================================ */
   
#ifndef _WIN32

class TerminalRawGuard {
    termios oldt;
    int oldf;
public:
    TerminalRawGuard() {
        tcgetattr(STDIN_FILENO, &oldt);

        termios raw = oldt;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    }

    ~TerminalRawGuard() {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
    }
} terminal;

int readKeyUnix() {
    unsigned char buf[8];
    int n = read(STDIN_FILENO, buf, sizeof(buf));

    if (n <= 0)
        return 0;

    // ENTER
    if (buf[0] == '\n')
        return SDDS_TDS_KEY_ENTER;

    // ESC + [
    if (buf[0] == 27 && n >= 3 && buf[1] == '[') {
        switch (buf[2]) {
            case 'A': return SDDS_TDS_KEY_UP;
            case 'B': return SDDS_TDS_KEY_DOWN;
            case 'C': return SDDS_TDS_KEY_RIGHT;
            case 'D': return SDDS_TDS_KEY_LEFT;
        }
        return SDDS_TDS_KEY_ESC;
    }

    //ESC
    if (buf[0] == 27)
        return SDDS_TDS_KEY_ESC;

    return buf[0];
}

#endif


/* ============================================================
   Abstract interface implementations
   ============================================================ */

    void doSetCursor(const TcursorInterface cursor) override {
        gotoxy(cursor.x, cursor.y);
        this->onTaskDone();
    }

    void doClear() override {
        clearScreen();
        gotoxy(this->getCursor().x, this->getCursor().y);
        this->onTaskDone();
    }

    void doUpdateRow(TrowChanges changes) override {
        writeToConsole(
            changes.firstChangedIdx,
            changes.row,
            changes._buffer,
            changes.n
        );
        this->onTaskDone();
    }
};

} // namespace textDisplaySpike
} // namespace sdds

#endif // UCONSOLEDISPLAY_H
